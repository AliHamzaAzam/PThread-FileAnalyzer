#include <pthread.h>
#include <iostream>
#include <mach/mach.h>
#include <mach/thread_policy.h>
#include <fstream>
#include <string>
#include <chrono>
#include <vector>
#include <unordered_set>
#include <mutex>

#define NUM_THREADS 30

using namespace std;

// Shared data structure to collect graph information
struct GraphData {
    unordered_set<int> nodes;
    int num_edges = 0;
    mutex mtx;
};

// Thread arguments struct to pass multiple arguments
struct ThreadArgs {
    int core_id;
    int start_line;
    int num_lines;
    string filepath;
    GraphData* graph_data;
};

void* threadFunction(void* arg) {
    ThreadArgs* args = reinterpret_cast<ThreadArgs*>(arg);

    // Open file and skip to starting line
    ifstream file(args->filepath);
    string line;
    for (int i = 0; i < args->start_line; ++i) {
        getline(file, line);
    }

    int local_edges = 0;
    unordered_set<int> local_nodes;

    // Process assigned lines
    for (int i = 0; i < args->num_lines; ++i) {
        if (!getline(file, line)) break;

        // Skip comment lines
        if (line[0] == '#') {
            i--;
            continue;
        }

        // Parse edge
        int space = line.find(' ');
        int node1 = stoi(line.substr(0, space));
        int node2 = stoi(line.substr(space + 1));

        local_edges++;
        local_nodes.insert(node1);
        local_nodes.insert(node2);
    }

    // Thread-safe update of shared graph data
    {
        lock_guard<mutex> lock(args->graph_data->mtx);
        args->graph_data->num_edges += local_edges;
        args->graph_data->nodes.insert(local_nodes.begin(), local_nodes.end());
    }

    cout << "Thread " << args->core_id << " processed " << local_edges
         << " edges and " << local_nodes.size() << " nodes" << endl;

    return nullptr;
}

void set_thread_affinity(pthread_t thread, int core_id) {
    thread_port_t mach_thread = pthread_mach_thread_np(thread);
    thread_affinity_policy_data_t policy = {core_id};
    kern_return_t ret = thread_policy_set(
        mach_thread,
        THREAD_AFFINITY_POLICY,
        reinterpret_cast<thread_policy_t>(&policy),
        THREAD_AFFINITY_POLICY_COUNT
    );
    if (ret != KERN_SUCCESS) {
        cerr << "Failed to set thread affinity for core " << core_id
             << ": " << mach_error_string(ret) << endl;
    }
}

int main() {
    string path_to_file = "/Users/azaleas/Downloads/com-orkut.ungraph.txt";

    // Count total lines
    ifstream file(path_to_file);
    int num_lines = 0;
    string line;
    while (getline(file, line)) {
        num_lines++;
    }
    file.clear();
    file.seekg(0, ios::beg);

    int num_lines_per_thread = num_lines / NUM_THREADS;
    int num_lines_last_thread = num_lines - (NUM_THREADS - 1) * num_lines_per_thread;

    cout << "Number of lines: " << num_lines << endl;
    cout << "Number of lines per thread: " << num_lines_per_thread << endl;
    cout << "Number of lines in last thread: " << num_lines_last_thread << endl;

    // Shared graph data
    GraphData graph_data;

    // Start measuring time
    auto start = chrono::high_resolution_clock::now();

    pthread_t threads[NUM_THREADS];
    ThreadArgs thread_args[NUM_THREADS];

    // Create threads
    for (int i = 0; i < NUM_THREADS; ++i) {
        int start_line = i * num_lines_per_thread;
        int thread_lines = (i == NUM_THREADS - 1) ? num_lines_last_thread : num_lines_per_thread;

        thread_args[i] = {
            i,                   // core_id
            start_line,          // start_line
            thread_lines,        // num_lines
            path_to_file,        // filepath
            &graph_data          // shared graph data
        };

        if (pthread_create(&threads[i], nullptr, threadFunction, &thread_args[i]) != 0) {
            cerr << "Failed to create thread" << endl;
            return 1;
        }

        // Set thread affinity
        // setThreadAffinity(threads[i], i % NUM_THREADS);
    }

    // Join threads
    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_join(threads[i], nullptr);
    }

    // Stop measuring time
    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> duration = end - start;

    cout << "Time taken: " << duration.count() << " seconds" << endl;
    cout << "Total unique nodes: " << graph_data.nodes.size() << endl;
    cout << "Total edges: " << graph_data.num_edges << endl;

    return 0;
}