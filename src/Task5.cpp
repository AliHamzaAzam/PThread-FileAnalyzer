//
// Created by Ali Hamza Azam on 28/01/2025.
// ID : 22I-2126
// Parallel and Distributed Computing - Assignment : 1
//
#include "utility.h"
#include <cfloat>
#include <numeric>
#include <vector>
#include <algorithm>
#include <cstring>
#include <sstream>
#include <cstdlib>


// Structure for storing matrix statistics.
struct MatrixData {
    double sum = 0;
    double max = -DBL_MAX;
    double min = DBL_MAX;
    int count = 0;
    // Per-row and per-column sums.
    unordered_map<int, double> row_sum;
    unordered_map<int, double> col_sum;
};

// Thread arguments structure now includes the data type string.
struct ThreadArgs {
    int thread_id;
    int core_id;
    const char* file_data; // pointer to beginning of mapped file
    size_t data_offset;    // offset where the matrix binary data starts
    int start_row;         // global start row (inclusive)
    int end_row;           // global end row (exclusive)
    int num_cols;          // number of columns in the matrix
    string dtype;          // either "<f4" or "<f8"
    MatrixData local_result;

    ThreadArgs(int thread_id, int core_id, const char* file_data, size_t data_offset,
               int start_row, int end_row, int num_cols, const string& dtype)
        : thread_id(thread_id), core_id(core_id), file_data(file_data),
          data_offset(data_offset), start_row(start_row), end_row(end_row),
          num_cols(num_cols), dtype(dtype) {}

    ThreadArgs() = default;
};

// Parse the .npy header.
// This function now returns the data type string in 'dtype'.
// It assumes a header that looks like:
// "{'descr': '<f4' or '<f8', 'fortran_order': False, 'shape': (num_rows, num_cols), }"
bool parse_npy_header(const char* data, size_t file_length, size_t &header_length,
                      int &num_rows, int &num_cols, string &dtype) {
    // Check magic string: first 6 bytes: \x93NUMPY
    const char magic_string[] = "\x93NUMPY";
    if (memcmp(data, magic_string, 6) != 0) {
        cerr << "File is not a valid .npy file (bad magic string)." << endl;
        return false;
    }
    // Version numbers.
    unsigned char major = data[6];
    unsigned char minor = data[7];
    size_t header_len_field = 0;
    size_t header_offset = 8; // magic (6) + version (2)
    if (major == 1) {
        // Next 2 bytes is little-endian header length.
        header_len_field = *(reinterpret_cast<const uint16_t*>(data + header_offset));
        header_offset += 2;
    } else if (major == 2) {
        // Next 4 bytes for version 2.0.
        header_len_field = *(reinterpret_cast<const uint32_t*>(data + header_offset));
        header_offset += 4;
    } else {
        cerr << "Unsupported .npy version: " << (int)major << "." << (int)minor << endl;
        return false;
    }
    // Total header length.
    header_length = header_offset + header_len_field;
    if (header_length > file_length) {
        cerr << "Header length is greater than file length." << endl;
        return false;
    }
    // The header is an ASCII string.
    string header_str(data + header_offset, header_len_field);
    // Parse 'descr'
    size_t descr_pos = header_str.find("'descr':");
    if (descr_pos == string::npos) {
        cerr << "Header missing 'descr'." << endl;
        return false;
    }
    size_t first_quote = header_str.find("'", descr_pos + 7);
    size_t second_quote = header_str.find("'", first_quote + 1);
    if (first_quote == string::npos || second_quote == string::npos) {
        cerr << "Error parsing 'descr' in header." << endl;
        return false;
    }
    dtype = header_str.substr(first_quote + 1, second_quote - first_quote - 1);
    // Accept only <f4 or <f8.
    if (dtype != "<f4" && dtype != "<f8") {
        cerr << "Unsupported data type (descr = " << dtype << "). Only '<f4' and '<f8' are supported." << endl;
        return false;
    }

    // Parse 'fortran_order'
    size_t order_pos = header_str.find("'fortran_order':");
    if (order_pos == string::npos) {
        cerr << "Header missing 'fortran_order'." << endl;
        return false;
    }
    bool fortran_order = false;
    if (header_str.find("True", order_pos) != string::npos)
        fortran_order = true;
    if (fortran_order) {
        cerr << "Only C-order arrays are supported." << endl;
        return false;
    }

    // Parse 'shape'
    size_t shape_pos = header_str.find("'shape':");
    if (shape_pos == string::npos) {
        cerr << "Header missing 'shape'." << endl;
        return false;
    }
    size_t paren_start = header_str.find("(", shape_pos);
    size_t paren_end = header_str.find(")", paren_start);
    if (paren_start == string::npos || paren_end == string::npos) {
        cerr << "Error parsing 'shape' in header." << endl;
        return false;
    }
    string shape_str = header_str.substr(paren_start + 1, paren_end - paren_start - 1);
    // Remove spaces.
    shape_str.erase(remove_if(shape_str.begin(), shape_str.end(), ::isspace), shape_str.end());
    // Split by comma.
    size_t comma_pos = shape_str.find(",");
    if (comma_pos == string::npos) {
        cerr << "Expected a 2D array but found shape: " << shape_str << endl;
        return false;
    }
    string rows_str = shape_str.substr(0, comma_pos);
    string cols_str = shape_str.substr(comma_pos + 1);
    num_rows = stoi(rows_str);
    num_cols = stoi(cols_str);
    return true;
}

// Thread function to process a chunk of rows.
// This function branches based on the data type stored in args->dtype.
void* process_chunk(void* arg) {
    auto* args = static_cast<ThreadArgs*>(arg);
    if (args->core_id != -1) {
        set_thread_affinity(pthread_self(), args->core_id);
    }

    MatrixData local;
    int num_cols = args->num_cols;
    int start_row = args->start_row;
    int end_row = args->end_row;

    // Process based on the data type.
    if (args->dtype == "<f4") {
        const float* matrix = reinterpret_cast<const float*>(args->file_data + args->data_offset);
        for (int row = start_row; row < end_row; row++) {
            double row_sum = 0;
            for (int col = 0; col < num_cols; col++) {
                float val = matrix[row * num_cols + col];
                local.sum += val;
                local.max = std::max(local.max, static_cast<double>(val));
                local.min = std::min(local.min, static_cast<double>(val));
                local.count++;

                row_sum += val;
                local.col_sum[col] += val;
            }
            local.row_sum[row] = row_sum;
        }
    } else if (args->dtype == "<f8") {
        const double* matrix = reinterpret_cast<const double*>(args->file_data + args->data_offset);
        for (int row = start_row; row < end_row; row++) {
            double row_sum = 0;
            for (int col = 0; col < num_cols; col++) {
                double val = matrix[row * num_cols + col];
                local.sum += val;
                local.max = std::max(local.max, val);
                local.min = std::min(local.min, val);
                local.count++;

                row_sum += val;
                local.col_sum[col] += val;
            }
            local.row_sum[row] = row_sum;
        }
    }
    args->local_result = local;
    return nullptr;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <npy_file_path> <num_threads> [core_affinity] [save_to_file]\n";
        return 1;
    }
    string file_path = argv[1];
    int num_threads = stoi(argv[2]);
    if (num_threads <= 0) {
        cerr << "Invalid number of threads\n";
        return 1;
    }
    bool core_affinity = (argc > 3) ? (string(argv[3]) == "true") : false;
    bool save_to_file = (argc > 4) ? (string(argv[4]) == "true") : false;

    // Pre-processing - START
    // Memory map file.
    size_t file_length;
    void* mapped_file = mmap_file(file_path, file_length);
    if (!mapped_file) {
        return 1;
    }
    const char* file_data = static_cast<const char*>(mapped_file);

    // Parse the .npy header.
    size_t header_length;
    int num_rows, num_cols;
    string dtype;
    if (!parse_npy_header(file_data, file_length, header_length, num_rows, num_cols, dtype)) {
        munmap(mapped_file, file_length);
        return 1;
    }

    // Check that the file contains enough data.
    size_t element_size = (dtype == "<f4") ? sizeof(float) : sizeof(double);
    size_t expected_data_bytes = static_cast<size_t>(num_rows) * num_cols * element_size;
    if (header_length + expected_data_bytes > file_length) {
        cerr << "File does not contain enough data for the expected array." << endl;
        munmap(mapped_file, file_length);
        return 1;
    }

    // Partition rows among threads.
    vector<pthread_t> threads(num_threads);
    vector<ThreadArgs> thread_args;
    thread_args.reserve(num_threads);

    int rows_per_thread = num_rows / num_threads;
    int remainder = num_rows % num_threads;
    int current_row = 0;
    for (int i = 0; i < num_threads; i++) {
        int extra = (i < remainder) ? 1 : 0;
        int start_row = current_row;
        int end_row = current_row + rows_per_thread + extra;
        int core_id = core_affinity ? (i % CORE_COUNT) : -1;
        thread_args.emplace_back(i, core_id, file_data, header_length, start_row, end_row, num_cols, dtype);
        current_row = end_row;
    }
    // Pre-processing - END

    // Main processing - START
    auto start_time = chrono::high_resolution_clock::now();

    // Create threads.
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], nullptr, process_chunk, &thread_args[i]);
    }
    // Wait for threads.
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], nullptr);
    }
    // Main processing - END

    // Post-processing - START
    // Aggregate results.
    MatrixData global_data;
    global_data.max = -DBL_MAX;
    global_data.min = DBL_MAX;
    for (int i = 0; i < num_threads; i++) {
        global_data.sum += thread_args[i].local_result.sum;
        global_data.count += thread_args[i].local_result.count;
        global_data.max = std::max(global_data.max, thread_args[i].local_result.max);
        global_data.min = std::min(global_data.min, thread_args[i].local_result.min);
        for (const auto& [r, sum] : thread_args[i].local_result.row_sum) {
            global_data.row_sum[r] += sum;
        }
        for (const auto& [c, sum] : thread_args[i].local_result.col_sum) {
            global_data.col_sum[c] += sum;
        }
    }

    auto end_time = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = end_time - start_time;

    cout << "Execution Time: " << elapsed.count() << " seconds\n";
    cout << "Sum: " << global_data.sum << endl;
    cout << "Max: " << global_data.max << endl;
    cout << "Min: " << global_data.min << endl;
    cout << "Count: " << global_data.count << endl;

    double total_row_sum = 0;
    for (const auto& p : global_data.row_sum) {
        total_row_sum += p.second;
    }
    double total_col_sum = 0;
    for (const auto& p : global_data.col_sum) {
        total_col_sum += p.second;
    }
    cout << "Sum of Row Sums: " << total_row_sum << endl;
    cout << "Sum of Column Sums: " << total_col_sum << endl;

    if (!save_to_file) {
        munmap(mapped_file, file_length);
        return 0;
    }
    // Save results to ../output/task5_output.txt
    ofstream output_file("../output/task5_output.txt");
    output_file << "Execution Time: " << elapsed.count() << " seconds\n";
    output_file << "Sum: " << global_data.sum << endl;
    output_file << "Max: " << global_data.max << endl;
    output_file << "Min: " << global_data.min << endl;
    output_file << "Count: " << global_data.count << endl;
    output_file << "Sum of Row Sums: " << total_row_sum << endl;
    output_file << "Sum of Column Sums: " << total_col_sum << endl;
    output_file.close();

    // Post-processing - END
    munmap(mapped_file, file_length);
    return 0;
}
