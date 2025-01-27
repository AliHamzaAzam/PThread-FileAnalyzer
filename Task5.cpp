//
// Created by Ali Hamza Azam on 28/01/2025.
//
#include <iostream>
#include <unordered_map>
#include <fstream>
#include <mutex>
#include <pthread.h>
#include <sched.h>
#include <chrono>
#include <sys/mman.h>
#include <fcntl.h>
#include <float.h>
#include <numeric>
#include <unistd.h>

#define CORE_COUNT 11

using namespace std;

mutex mtx;

struct MatrixData {
    double sum = 0;
    float max = -FLT_MAX;
    float min = FLT_MAX;
    int count = 0;
    unordered_map<int, double> row_sum;
    unordered_map<int, double> col_sum;
};

struct ThreadArgs {
    int thread_id{};
    int core_id{};
    const char* data;
    long start{};
    long end{};
    MatrixData* matrix_data{};

    ThreadArgs(int thread_id, int core_id, const char* data, long start, long end, MatrixData* matrix_data)
        : thread_id(thread_id), core_id(core_id), data(data), start(start), end(end), matrix_data(matrix_data) {
    }

    ThreadArgs() = default;
};

void* mmap_file(const string& filepath, size_t& length) {
    int fd = open(filepath.c_str(), O_RDONLY);
    if (fd == -1) {
        perror("Failed to open file");
        return nullptr;
    }
    length = lseek(fd, 0, SEEK_END);
    void* data = mmap(nullptr, length, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    if (data == MAP_FAILED) {
        perror("Failed to mmap file");
        return nullptr;
    }
    return data;
}

long* calculate_chunk_offsets(const string& file_path, int num_threads) {
    FILE* file = fopen(file_path.c_str(), "rb");
    if (!file) {
        perror("Failed to open file");
        return nullptr;
    }

    fseek(file, 0, SEEK_END);
    long total_size = ftell(file);
    rewind(file);

    long* offsets = static_cast<long*>(malloc((num_threads + 1) * sizeof(long)));
    if (!offsets) {
        perror("Failed to allocate memory for offsets");
        fclose(file);
        return nullptr;
    }
    offsets[0] = 0;
    offsets[num_threads] = total_size;

    for (int i = 1; i < num_threads; i++) {
        long nominal_offset = i * (total_size / num_threads);
        fseek(file, nominal_offset, SEEK_SET);

        int c;
        while ((c = fgetc(file)) != EOF && c != '\n') {
        }

        if (c == EOF) {
            for (int j = i; j < num_threads; j++) offsets[j] = total_size;
            break;
        }
        offsets[i] = ftell(file);
    }

    fclose(file);
    return offsets;
}

#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/thread_policy.h>

void set_thread_affinity(pthread_t pthread, int core_id) {
    thread_affinity_policy_data_t policy = {core_id};
    thread_port_t mach_thread = pthread_mach_thread_np(pthread);
    thread_policy_set(mach_thread, THREAD_AFFINITY_POLICY, (thread_policy_t)&policy, 1);
}
#endif

#ifdef __linux__
void set_thread_affinity(pthread_t thread, int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
}
#endif

void* process_chunk(void* arg) {
    auto* args = static_cast<ThreadArgs*>(arg);

    if (args->core_id != -1) {
        set_thread_affinity(pthread_self(), args->core_id);
    }

    double local_sum = 0;
    float local_max = -FLT_MAX;
    float local_min = FLT_MAX;
    int local_count = 0;
    unordered_map<int, double> local_row_sum;
    unordered_map<int, double> local_col_sum;
    const char* data = args->data + args->start;
    const char* end = args->data + args->end;

    while (data < end) {
        const char* line_end = static_cast<const char*>(memchr(data, '\n', end - data));
        if (!line_end) {
            line_end = end;
        }

        int row = 0;
        const char* num_start = data;
        while (num_start < line_end) {
            const char* num_end = static_cast<const char*>(memchr(num_start, ' ', line_end - num_start));
            if (!num_end) {
                num_end = line_end;
            }

            float num = strtof(num_start, nullptr);
            local_sum += num;
            local_max = max(local_max, num);
            local_min = min(local_min, num);
            local_count++;
            local_row_sum[row] += num;
            local_col_sum[row] += num;

            num_start = num_end + 1;
            row++;
        }

        data = line_end + 1;
    }

    {
        lock_guard<mutex> lock(mtx);
        args->matrix_data->sum += local_sum;
        args->matrix_data->max = max(args->matrix_data->max, local_max);
        args->matrix_data->min = min(args->matrix_data->min, local_min);
        args->matrix_data->count += local_count;

        for (const auto& [row, sum] : local_row_sum) {
            args->matrix_data->row_sum[row] += sum;
        }

        for (const auto& [col, sum] : local_col_sum) {
            args->matrix_data->col_sum[col] += sum;
        }
    }

    return nullptr;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <file_path> <num_threads> [core_affinity]\n";
        return 1;
    }

    string file_path = argv[1];
    int num_threads = stoi(argv[2]);
    if (num_threads <= 0) {
        cerr << "Invalid number of threads\n";
        return 1;
    }
    bool core_affinity = (argc > 3) ? (string(argv[3]) == "true") : false;

    long* offsets = calculate_chunk_offsets(file_path, num_threads);
    if (!offsets) {
        return 1;
    }

    size_t length;
    void* mapped_data = mmap_file(file_path, length);
    if (!mapped_data) {
        free(offsets);
        return 1;
    }

    pthread_t threads[num_threads];
    ThreadArgs thread_args[num_threads];
    MatrixData matrix_data;
    for (int i = 0; i < num_threads; i++) {
        thread_args[i] = ThreadArgs(i, core_affinity ? i % CORE_COUNT : -1,
                                    static_cast<const char*>(mapped_data), offsets[i], offsets[i + 1], &matrix_data);
    }

    auto start_time = chrono::high_resolution_clock::now();

    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], nullptr, process_chunk, &thread_args[i]);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], nullptr);
    }

    auto end_time = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = end_time - start_time;

    cout << "Execution Time: " << elapsed.count() << " seconds\n";

    cout << "Sum: " << matrix_data.sum << endl;
    cout << "Max: " << matrix_data.max << endl;
    cout << "Min: " << matrix_data.min << endl;
    cout << "Count: " << matrix_data.count << endl;
    // cout << "Row Sums:\n";
    // for (const auto& [row, sum] : matrix_data.row_sum) {
    //     cout << "Row " << row << ": " << sum << endl;
    // }
    // cout << "Column Sums:\n";
    // for (const auto& [col, sum] : matrix_data.col_sum) {
    //     cout << "Column " << col << ": " << sum << endl;
    // }
    cout << "Sum of Row Sums: " << accumulate(matrix_data.row_sum.begin(), matrix_data.row_sum.end(), 0.0,
                                              [](double sum, const pair<int, double>& p) {
                                                  return sum + p.second;
                                              }) << endl;
    cout << "Sum of Column Sums: " << accumulate(matrix_data.col_sum.begin(), matrix_data.col_sum.end(), 0.0,
                                                 [](double sum, const pair<int, double>& p) {
                                                     return sum + p.second;
                                                 }) << endl;

    munmap(mapped_data, length);
    free(offsets);
    return 0;
}
