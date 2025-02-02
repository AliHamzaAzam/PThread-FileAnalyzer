//
// Created by Ali Hamza Azam on 29/01/2025.
// ID : 22I-2126
// Parallel and Distributed Computing - Assignment : 1
//
#include "utility.h"
#include <sstream>

mutex mtx;                                                                 // Mutex for critical sections

struct LogEntry {
    std::string date;                  // Date of the log entry
    std::string time;                  // Time of the log entry
    std::string number;                // Log entry number
    std::string level;                 // Log level (INFO, WARN, ERROR)
    std::string logClass;              // Log class (if present)
    std::string blockId;               // Block ID (if present)
    std::string src;                   // Source (if present)
    std::string dest;                  // Destination (if present)
};

struct LogData {
    int total_entries = 0;                                                // Total log entries
    unordered_map<string, int> logs_per_level;                            // Log entries per level
    unordered_map<string, unordered_map<string, int>> errors_per_level;   // Errors per level per block
    unordered_map<string, int> unique_logs;                               // Unique log entries per level
};

struct ThreadArgs {
    int thread_id{};
    int core_id{};
    const char* data;
    long start{};
    long end{};
    LogData* log_data{};

    ThreadArgs(int thread_id, int core_id, const char* data, long start, long end, LogData* log_data)
            : thread_id(thread_id), core_id(core_id), data(data), start(start), end(end), log_data(log_data) {}
    ThreadArgs() = default;
};


// Helper function to parse a log line into a LogEntry struct
LogEntry parseLogLine(const std::string& line) {
    LogEntry entry;
    std::istringstream iss(line);

    // Extract date, time, number, level, and class
    iss >> entry.date >> entry.time >> entry.number >> entry.level;

    std::string classWithColon;
    iss >> classWithColon;
    if (!classWithColon.empty() && classWithColon.back() == ':') {
        entry.logClass = classWithColon.substr(0, classWithColon.size() - 1);
    }

    // Extract the full message
    std::string message;
    std::getline(iss, message);

    // Trim leading whitespace from the message
    size_t firstNonSpace = message.find_first_not_of(" \t");
    if (firstNonSpace != std::string::npos) {
        message = message.substr(firstNonSpace);
    }

    // Parse message components (block, src, dest)
    size_t blockPos = message.find("block ");
    if (blockPos != std::string::npos) {
        size_t blockStart = blockPos + 6; // Skip "block "
        size_t srcPos = message.find(" src: ", blockStart);
        if (srcPos != std::string::npos) {
            entry.blockId = message.substr(blockStart, srcPos - blockStart);

            size_t srcStart = srcPos + 6; // Skip " src: "
            size_t destPos = message.find(" dest: ", srcStart);
            if (destPos != std::string::npos) {
                entry.src = message.substr(srcStart, destPos - srcStart);
                entry.dest = message.substr(destPos + 7); // Skip " dest: "
            }
        }
    }

    return entry;
}

// Thread function to process a chunk of data
void* process_chunk(void* arg) {
    auto* args = static_cast<ThreadArgs*>(arg);

    if (args->core_id != -1) {
        set_thread_affinity(pthread_self(), args->core_id);
    }

    int local_entries = 0;
    unordered_map<string, int> local_logs_per_level;
    unordered_map<string, unordered_map<string, int>> local_errors_per_level;
    unordered_map<string, int> local_unique_logs;

    auto data = args->data + args->start;
    const auto end = args->data + args->end;

    while (data < args->data + args->end) {
        auto line_end = static_cast<const char*>(memchr(data, '\n', end - data));
        if (!line_end) {
            line_end = args->data + args->end;
        }

        string line(data, line_end - data);
        data = line_end + 1;

        if (line.empty()) {
            continue;
        }

        LogEntry entry = parseLogLine(line);
        if (entry.level.empty()) {
            continue;
        }

        ++local_entries;
        ++local_logs_per_level[entry.level];
        ++local_unique_logs[entry.level];

        if (!entry.blockId.empty() && entry.level == "ERROR") {
            ++local_errors_per_level[entry.level][entry.blockId];
        }
    }


    // Update global data after processing the chunk
    {
        lock_guard<mutex> lock(mtx);
        args->log_data->total_entries += local_entries;
        for (const auto& [level, count] : local_logs_per_level) {
            args->log_data->logs_per_level[level] += count;
        }
        for (const auto& [level, errors] : local_errors_per_level) {
            for (const auto& [error, count] : errors) {
                args->log_data->errors_per_level[level][error] += count;
            }
        }
        for (const auto& [log, count] : local_unique_logs) {
            args->log_data->unique_logs[log] += count;
        }
    }

    return nullptr;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <file_path> <num_threads> [core_affinity] [save_to_file]\n";
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
    // Memory map file
    size_t length;
    void* mapped_data = mmap_file(file_path, length);
    if (!mapped_data) {
        return 1;
    }

    // Calculate chunk offsets
    auto offsets = calculate_chunk_offsets_mapped(static_cast<const char*>(mapped_data), length, num_threads);

    // Create threads
    pthread_t threads[num_threads];
    ThreadArgs thread_args[num_threads];
    LogData log_data;
    priority_queue<pair<int, int>, vector<pair<int, int>>, greater<>> top_nodes; // Min-heap for top 10
    for (int i = 0; i < num_threads; i++) {
        if (core_affinity) {
            thread_args[i] = ThreadArgs(i, i % CORE_COUNT, static_cast<const char*>(mapped_data), offsets[i], offsets[i + 1], &log_data);
        } else {
            thread_args[i] = ThreadArgs(i, -1, static_cast<const char*>(mapped_data), offsets[i], offsets[i + 1], &log_data);
        }
    }
    // Pre-processing - END

    // Main processing - START
    // Start threads
    auto start_time = chrono::high_resolution_clock::now();
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], nullptr, process_chunk, &thread_args[i]);
    }

    // Join threads
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], nullptr);
    }
    auto end_time = chrono::high_resolution_clock::now();
    // Main processing - END

    // Post-processing - START
    chrono::duration<double> elapsed = end_time - start_time;
    // Output the results
    cout << "Execution Time: " << elapsed.count() << " seconds\n";
    cout << "Total log entries: " << log_data.total_entries << endl;
    cout << "Log entries per level:\n";
    for (const auto& [level, count] : log_data.logs_per_level) {
        cout << level << ": " << count << " entries\n";
    }
    cout << "Errors per level:\n";
    if (log_data.errors_per_level.empty()) {
        cout << "No errors found\n";
    } else {
        for (const auto& [level, errors] : log_data.errors_per_level) {
            cout << level << ":\n";
            for (const auto& [error, count] : errors) {
                cout << "  " << error << ": " << count << " occurrences\n";
            }
        }
    }

    if (!save_to_file) {
        munmap(mapped_data, length);
        return 0;
    }
    // Save results to ../output/task4_output.txt
    ofstream output_file("../output/task4_output.txt");
    output_file << "Total log entries: " << log_data.total_entries << endl;
    output_file << "Log entries per level:\n";
    for (const auto& [level, count] : log_data.logs_per_level) {
        output_file << level << ": " << count << " entries\n";
    }
    output_file << "Errors per level:\n";
    if (log_data.errors_per_level.empty()) {
        output_file << "No errors found\n";
    } else {
        for (const auto& [level, errors] : log_data.errors_per_level) {
            output_file << level << ":\n";
            for (const auto& [error, count] : errors) {
                output_file << "  " << error << ": " << count << " occurrences\n";
            }
        }
    }
    output_file.close();
    // Post-processing - END

    // Free memory
    munmap(mapped_data, length);
    return 0;
}