//
// Created by Ali Hamza Azam on 27/01/2025.
// ID : 22I-2126
// Parallel and Distributed Computing - Assignment : 1
//
#include "utility.h"

mutex mtx;                             // Mutex for critical sections

struct GraphData {
    unordered_map<int, int> nodes;     // Nodes with degree
    int num_edges = 0;                 // Total edges
};

struct ThreadArgs {
    int thread_id;
    int core_id;
    const char* data;
    long start;
    long end;
    GraphData* text_data;

    ThreadArgs(int thread_id, int core_id, const char* data, long start, long end, GraphData* graph_data)
            : thread_id(thread_id), core_id(core_id), data(data), start(start), end(end), text_data(graph_data) {}
    ThreadArgs() = default;
};

// Thread function to process a chunk of data
void* process_chunk(void* arg) {
    auto* args = static_cast<ThreadArgs*>(arg);

    if (args->core_id != -1) {
        set_thread_affinity(pthread_self(), args->core_id);
    }

    int local_edges = 0;
    unordered_map<int, int> local_nodes;
    const char* data = args->data + args->start;
    const char* end = args->data + args->end;

    while (data < end) {
        const char* line_end = static_cast<const char*>(memchr(data, '\n', end - data));
        if (!line_end) {
            line_end = end;
        }

        string line(data, line_end - data);
        data = line_end + 1;

        if (line.empty() || line[0] == '#') {
            continue;
        }

        int space = line.find('\t');
        int node1 = stoi(line.substr(0, space));
        int node2 = stoi(line.substr(space + 1));

        local_edges++;
        local_nodes[node1]++;
        local_nodes[node2]++;
    }

    // Update global data after processing the chunk
    {
        lock_guard<mutex> lock(mtx);
        args->text_data->num_edges += local_edges;

        for (const auto& [node, degree] : local_nodes) {
            args->text_data->nodes[node] += degree;
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
    GraphData graph_data;
    priority_queue<pair<int, int>, vector<pair<int, int>>, greater<>> top_nodes; // Min-heap for top 10
    for (int i = 0; i < num_threads; i++) {
        if (core_affinity) {
            thread_args[i] = ThreadArgs(i, i % CORE_COUNT, static_cast<const char*>(mapped_data), offsets[i], offsets[i + 1], &graph_data);
        } else {
            thread_args[i] = ThreadArgs(i, -1, static_cast<const char*>(mapped_data), offsets[i], offsets[i + 1], &graph_data);
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
    auto join_time = chrono::high_resolution_clock::now();
    // Main processing - END

    // Post-processing - START
    // Determine top-10 nodes by degree using a min-heap
    for (const auto& [node, degree] : graph_data.nodes) {
        if (top_nodes.size() < 10) {
            top_nodes.emplace(degree, node);
        } else if (degree > top_nodes.top().first) {
            top_nodes.pop();
            top_nodes.emplace(degree, node);
        }
    }

    auto end_time = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = end_time - start_time;
    chrono::duration<double> join_elapsed = join_time - start_time;

    cout << "Execution Time: " << elapsed.count() << " seconds\n";
    cout << "Join time: "<< join_elapsed.count() << " seconds\n";

    // Output the results
    vector<pair<int, int>> temp_top_nodes; // Min-heap for top 10
    cout << "Total unique nodes: " << graph_data.nodes.size() << endl;
    cout << "Total edges: " << graph_data.num_edges << endl;
    cout << "Top 10 nodes with highest degree:" << endl;
    while (!top_nodes.empty()) {
        auto [degree, node] = top_nodes.top();
        top_nodes.pop();
        cout << "Node: " << node << ", Degree: " << degree << endl;
        temp_top_nodes.emplace_back(degree, node);
    }

    if (!save_to_file) {
        munmap(mapped_data, length);
        return 0;
    }
    // Save results to ../output/task1_output.txt
    ofstream output_file("../output/task1_output.txt");
    output_file << "Total unique nodes: " << graph_data.nodes.size() << endl;
    output_file << "Total edges: " << graph_data.num_edges << endl;
    output_file << "Top 10 nodes with highest degree:" << endl;
    while (!temp_top_nodes.empty()) {
        auto [degree, node] = temp_top_nodes.back();
        temp_top_nodes.pop_back();
        output_file << "Node: " << node << ", Degree: " << degree << endl;
    }
    output_file.close();
    // Post-processing - END

    // Free memory
    munmap(mapped_data, length);
    return 0;
}