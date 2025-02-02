//
// Created by Ali Hamza Azam on 01/02/2025.
//

#ifndef UTILITY_H
#define UTILITY_H

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

#define CORE_COUNT 11

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

using namespace std;

// Helper function to memory map a file
void* mmap_file(const string& filepath, size_t& length) {
    int fd = open(filepath.c_str(), O_RDONLY);
    length = lseek(fd, 0, SEEK_END);
    void* data = mmap(nullptr, length, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    return data;
}

// Helper function to calculate chunk offsets for memory mapped data
std::vector<size_t> calculate_chunk_offsets_mapped(const char* data, size_t size, int num_threads) {
    std::vector<size_t> offsets(num_threads + 1);
    offsets[0] = 0;
    offsets[num_threads] = size;

    for (int i = 1; i < num_threads; ++i) {
        size_t nominal_offset = i * (size / num_threads);
        const char* ptr = data + nominal_offset;

        // Scan forward for the next newline
        while (ptr < data + size && *ptr != '\n') ++ptr;
        if (ptr >= data + size) ptr = data + size;

        offsets[i] = ptr - data;
    }

    return offsets;
}

#endif //UTILITY_H
