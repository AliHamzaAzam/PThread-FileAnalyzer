//
// Created by Ali Hamza Azam on 24/01/2025.
//
#include <dispatch/dispatch.h>
#include <stdio.h>

int main() {
    // Create a serial queue
    dispatch_queue_t serialQueue = dispatch_queue_create("com.example.serialqueue", NULL);

    // Create a concurrent queue
    dispatch_queue_t concurrentQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);

    // Dispatch work to a specific queue
    dispatch_async(concurrentQueue, ^{
        printf("Running on a global concurrent queue\n");
    });

    // Dispatch work to a serial queue
    dispatch_async(serialQueue, ^{
        printf("Running on a serial queue\n");
    });

    // Wait for queued blocks to complete
    dispatch_barrier_async(concurrentQueue, ^{
        printf("Barrier block - ensures previous blocks are complete\n");
    });

    // Optional: wait for all work to complete
    dispatch_barrier_sync(concurrentQueue, ^{
        printf("All previous work is now complete\n");
    });

    return 0;
}