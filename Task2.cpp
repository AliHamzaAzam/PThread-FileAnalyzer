//
// Created by Ali Hamza Azam on 27/01/2025.
//
#include <iostream>
#include <unordered_map>
#include <fstream>
#include <mutex>
#include <pthread.h>
#include <sched.h>
#include <chrono>
#include <utility>
#include <queue>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <algorithm>


#define CORE_COUNT 11

using namespace std;

mutex mtx;

struct GraphData {
    unordered_map<string, int> word_count; //words with frequency
    int num_words = 0;
    int num_vowel_words = 0;
};

struct ThreadArgs {
    int thread_id{};
    int core_id{};
    const char* data;
    long start{};
    long end{};
    GraphData* text_data{};

    ThreadArgs(int thread_id, int core_id, const char* data, long start, long end, GraphData* text_data)
            : thread_id(thread_id), core_id(core_id), data(data), start(start), end(end), text_data(text_data) {}
    ThreadArgs() = default;
};

void* mmap_file(const std::string& filepath, size_t& length) {
    int fd = open(filepath.c_str(), O_RDONLY);
    length = lseek(fd, 0, SEEK_END);
    void* data = mmap(nullptr, length, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    return data;
}

long* calculate_chunk_offsets(const string& file_path, int num_threads) {
    FILE *file = fopen(file_path.c_str(), "rb");
    if (!file) {
        perror("Failed to open file");
        return nullptr;
    }

    fseek(file, 0, SEEK_END);
    long total_size = ftell(file);
    rewind(file);

    long* offsets = static_cast<long*>(malloc((num_threads + 1) * sizeof(long)));
    offsets[0] = 0;
    offsets[num_threads] = total_size;

    for (int i = 1; i < num_threads; i++) {
        long nominal_offset = i * (total_size / num_threads);
        fseek(file, nominal_offset, SEEK_SET);

        int c;
        while ((c = fgetc(file)) != EOF && c != '\n' && c != ' ') {}

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

    int local_word_count = 0;
    int local_vowel_word_count = 0;
    unordered_map<string, int> local_words;
    const char* data = args->data + args->start;
    const char* end = args->data + args->end;

    while (data < end) {
        const char* line_end = static_cast<const char*>(memchr(data, '\n', end - data));
        if (!line_end) {
            line_end = end;
        }

        string line(data, line_end - data);
        data = line_end + 1;

        if (line.empty()) {
            continue;
        }

        // Split line into words
        size_t start = 0;
        size_t space = line.find(' ');
        while (space != string::npos) {
            string word = line.substr(start, space - start);
            start = space + 1;
            space = line.find(' ', start);

            if (word.empty() || !isalpha(word[0])) {
                continue;
            }

            // Convert to lowercase
            transform(word.begin(), word.end(), word.begin(), ::tolower);

            if (word.find_first_of("aeiou") != string::npos) {
                local_vowel_word_count++;
            }
            local_word_count++;
            local_words[word]++;
        }
    }

    {
        lock_guard<mutex> lock(mtx);
        args->text_data->num_words += local_word_count;
        args->text_data->num_vowel_words += local_vowel_word_count;
        args->text_data->word_count.insert(local_words.begin(), local_words.end());
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

    // Calculate chunk offsets
    long* offsets = calculate_chunk_offsets(file_path, num_threads);
    if (!offsets) {
        return 1;
    }

    // Memory map file
    size_t length;
    void* mapped_data = mmap_file(file_path, length);
    if (!mapped_data) {
        free (offsets);
        return 1;
    }

    // Create threads
    pthread_t threads[num_threads];
    ThreadArgs thread_args[num_threads];
    GraphData word_data;
    priority_queue<pair<int, string>, vector<pair<int, string>>, greater<>> top_words; // Min-heap for top 10
    for (int i = 0; i < num_threads; i++) {
        if (core_affinity) {
            thread_args[i] = ThreadArgs(i, i % CORE_COUNT, static_cast<const char*>(mapped_data), offsets[i], offsets[i + 1], &word_data);
        } else {
            thread_args[i] = ThreadArgs(i, -1, static_cast<const char*>(mapped_data), offsets[i], offsets[i + 1], &word_data);
        }
    }


    auto start_time = chrono::high_resolution_clock::now();

    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], nullptr, process_chunk, &thread_args[i]);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], nullptr);
    }

    auto join_time = chrono::high_resolution_clock::now();

    // Determine top-10 words populate min-heap
    for (auto& [word, count] : word_data.word_count) {
        top_words.push({count, word});
        if (top_words.size() > 10) {
            top_words.pop();
        } else if (count > top_words.top().first) {
            top_words.pop();
            top_words.emplace(count, word);
        }
    }

    auto end_time = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = end_time - start_time;
    chrono::duration<double> join_elapsed = join_time - start_time;

    cout << "Execution Time: " << elapsed.count() << " seconds\n";
    cout << "Join time: "<< join_elapsed.count() << " seconds\n";

    // Output the results
    cout << "Total unique words: " << word_data.word_count.size() << endl;
    cout << "Total words starting with vowels: " << word_data.num_vowel_words << endl;
    cout << "Total words: " << word_data.num_words << endl;
    // Top 10 words
    cout << "Top 10 words with highest frequency:" << endl;
    while (!top_words.empty()) {
        cout << top_words.top().second << ": " << top_words.top().first << " occurrences" << endl;
        top_words.pop();
    }


    // Free memory
    free(offsets);
    return 0;
}