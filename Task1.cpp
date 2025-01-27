//
// Created by Ali Hamza Azam on 26/01/2025.
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


#define CORE_COUNT 4

using namespace std;


struct GraphData {
    unordered_map<int, int> nodes; //nodes with degree
    priority_queue<pair<int, int>, vector<pair<int, int>>, greater<>> top_nodes; // Min-heap for top 10
    int num_edges = 0;
};

struct ThreadArgs {
    int thread_id{};
    int core_id{};
    string filepath;
    long start{};
    long end{};
    GraphData local_graph_data{};

    ThreadArgs(int thread_id, int core_id, string  filepath, long start, long end)
            : thread_id(thread_id), core_id(core_id), filepath(std::move(filepath)), start(start), end(end) {}
    ThreadArgs() = default;
};



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
        while ((c = fgetc(file)) != EOF && c != '\n') {}

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


void* process_chunk(void* arg) {
    auto* args = static_cast<ThreadArgs*>(arg);

    ifstream file(args->filepath, ios::in | ios::binary);
    file.seekg(args->start);

    if (args->core_id != -1) {
        set_thread_affinity(pthread_self(), args->core_id);
    }

    unordered_map<int, int> local_nodes;
    string buffer;
    buffer.reserve(4096); // buffer for faster reading
    string line;

    while (file.tellg() < args->end) {
        getline(file, line);

        if (line.empty() || line[0] == '#') {
            continue;
        }

        int space = line.find('\t');
        int node1 = stoi(line.substr(0, space));
        int node2 = stoi(line.substr(space + 1));


        args->local_graph_data.num_edges++;
        args->local_graph_data.nodes[node1]++;
        args->local_graph_data.nodes[node2]++;

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

    // Create threads
    pthread_t threads[num_threads];
    ThreadArgs thread_args[num_threads];
    for (int i = 0; i < num_threads; i++) {
        if (core_affinity) {
            thread_args[i] = ThreadArgs(i, i % CORE_COUNT, file_path, offsets[i], offsets[i + 1]);
        } else {
            thread_args[i] = ThreadArgs(i, -1, file_path, offsets[i], offsets[i + 1]);
        }
    }


    auto start_time = chrono::high_resolution_clock::now();

    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], nullptr, process_chunk, &thread_args[i]);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], nullptr);
    }

    // Populate global graph data
    GraphData graph_data;
    for (const auto& args : thread_args) {
        graph_data.num_edges += args.local_graph_data.num_edges;

        for (const auto& [node, degree] : args.local_graph_data.nodes) {
            graph_data.nodes[node] += degree;

            // Update top nodes
            if (graph_data.top_nodes.size() < 10) {
                graph_data.top_nodes.emplace(degree, node);
            } else if (degree > graph_data.top_nodes.top().first) {
                graph_data.top_nodes.pop();
                graph_data.top_nodes.emplace(degree, node);
            }
        }
    }

    auto end_time = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = end_time - start_time;

    cout << "Execution Time: " << elapsed.count() << " seconds\n";

    // Output the results
    cout << "Total unique nodes: " << graph_data.nodes.size() << endl;
    cout << "Total edges: " << graph_data.num_edges << endl;
    cout << "Top 10 nodes with highest degree:" << endl;
    while (!graph_data.top_nodes.empty()) {
        auto [degree, node] = graph_data.top_nodes.top();
        graph_data.top_nodes.pop();
        cout << "Node: " << node << ", Degree: " << degree << endl;
    }


    // Free memory
    free(offsets);
    return 0;
}

