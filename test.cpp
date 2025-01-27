// #include <pthread.h>
// #include <mach/mach.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
//
// void* thread_function(void* arg) {
//     int thread_num = *((int*)arg);
//
//     // Get the Mach thread for this POSIX thread
//     thread_t mach_thread = mach_thread_self();
//
//     // Set thread affinity policy
//     thread_affinity_policy_data_t policy = { thread_num };
//     kern_return_t kr = thread_policy_set(mach_thread,
//                                          THREAD_AFFINITY_POLICY,
//                                          (thread_policy_t)&policy,
//                                          THREAD_AFFINITY_POLICY_COUNT);
//     if (kr != KERN_SUCCESS) {
//         printf("Failed to set affinity for thread %d (error: %d)\n", thread_num, kr);
//     } else {
//         printf("Thread %d bound to core %d\n", thread_num, thread_num);
//     }
//
//     // Simulate some workload
//     while (1) {
//         // Dummy computation
//         usleep(100000); // Sleep for 100 ms
//     }
//
//     return NULL;
// }
//
// int main() {
//     int num_threads = 4; // Number of threads
//     pthread_t threads[num_threads];
//     int thread_ids[num_threads];
//
//     for (int i = 0; i < num_threads; i++) {
//         thread_ids[i] = i; // Assign a unique ID to each thread
//         if (pthread_create(&threads[i], NULL, thread_function, &thread_ids[i]) != 0) {
//             perror("Failed to create thread");
//             return 1;
//         }
//     }
//
//     for (int i = 0; i < num_threads; i++) {
//         pthread_join(threads[i], NULL);
//     }
//
//     return 0;
// }



#include <pthread.h>
#include <iostream>
#include <mach/mach.h>
#include <mach/thread_policy.h>

#define NUM_THREADS 4

void* threadFunction(void* arg) {
    int core_id = *reinterpret_cast<int*>(arg);
    std::cout << "Thread running on core " << core_id << std::endl;
    pthread_exit(nullptr);
}

void setThreadAffinity(pthread_t thread, int core_id) {
    thread_port_t mach_thread = pthread_mach_thread_np(thread);
    thread_affinity_policy_data_t policy = {core_id};
    kern_return_t ret = thread_policy_set(
        mach_thread,
        THREAD_AFFINITY_POLICY,
        reinterpret_cast<thread_policy_t>(&policy),
        THREAD_AFFINITY_POLICY_COUNT
    );
    if (ret != KERN_SUCCESS) {
        std::cerr << "Failed to set thread affinity for core " << core_id
                  << ": " << mach_error_string(ret) << std::endl;
    }
}

//int main() {
//    pthread_t threads[NUM_THREADS];
//    int core_ids[NUM_THREADS];
//
//    for (int i = 0; i < NUM_THREADS; ++i) {
//        core_ids[i] = i; // Assign each thread to a specific core
//        if (pthread_create(&threads[i], nullptr, threadFunction, &core_ids[i]) != 0) {
//            std::cerr << "Failed to create thread" << std::endl;
//            return 1;
//        }
//        // Set thread affinity
//        setThreadAffinity(threads[i], i % NUM_THREADS);
//    }
//
//    for (int i = 0; i < NUM_THREADS; ++i) {
//        pthread_join(threads[i], nullptr);
//    }
//
//    return 0;
//}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <file_path> <num_threads> [core_affinity]\n";
        return 1;
    }

    std::string file_path = argv[1];
    int num_threads = std::stoi(argv[2]);
    bool core_affinity = (argc > 3) ? (std::string(argv[3]) == "true") : false;

    auto start_time = std::chrono::high_resolution_clock::now();

    // Perform the task (e.g., graph analysis, word counting, etc.)
    // This is where the main logic of the task goes.

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;

    std::cout << "Execution Time: " << elapsed.count() << " seconds\n";

    // Example output
    std::cout << "Results:\n";
    std::cout << "Nodes: 100, Edges: 500\n"; // Replace with actual results.
}




