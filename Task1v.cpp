//
// Created by Ali Hamza Azam on 27/01/2025.
//
#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>

using namespace std;

int total_edges = 0;
std::vector<pair<int, int>> verifyTopNodes(const string& filename) {
    ifstream file(filename, ios::in | ios::binary);
    if (!file) {
        cerr << "Failed to open file for verification\n";
        return {};
    }

    unordered_map<int, int> neighbor_counts;
    string line;

    while (getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        int space = line.find('\t');
        int node1 = stoi(line.substr(0, space));
        int node2 = stoi(line.substr(space + 1));

        total_edges++;
        neighbor_counts[node1]++;
        neighbor_counts[node2]++;
    }

    file.close();

    vector<pair<int, int>> nodes(neighbor_counts.begin(), neighbor_counts.end());

    sort(nodes.begin(), nodes.end(),
              [](const pair<int, int>& a, const pair<int, int>& b) {
                  return b.second < a.second;
              });

    if (nodes.size() > 10) {
        nodes.resize(10);
    }
    return nodes;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <filename> <num_threads>\n";
        return 1;
    }

    string filename = argv[1];
    int num_threads = stoi(argv[2]);
    if (num_threads <= 0) {
        cerr << "Invalid number of threads\n";
        return 1;
    }

    auto start_time = chrono::high_resolution_clock::now();
    vector<pair<int, int>> verified_top_nodes = verifyTopNodes(filename);
    auto end_time = chrono::high_resolution_clock::now();

    cout << "Verified top 10 nodes:\n";
    cout << "Execution Time: " << chrono::duration<double>(end_time - start_time).count() << " seconds\n";
    cout << "Total unique nodes: " << verified_top_nodes.size() << endl;
    cout << "Total edges: " << total_edges << endl;
    for (const auto& [node, count] : verified_top_nodes) {
        cout << "Node " << node << ": " << count << " neighbors\n";
    }

    return 0;
}