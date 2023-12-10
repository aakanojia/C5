// #include "client_registry.h"
// #include <pthread.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>

// #define NUM_THREADS 10
// #define NUM_ITERATIONS 100

// CLIENT_REGISTRY *registry;

// void *test_thread(void *arg) {
//     int thread_num = *(int *)arg;
//     for (int i = 0; i < NUM_ITERATIONS; i++) {
//         int fd = thread_num * NUM_ITERATIONS + i;
//         if (creg_register(registry, fd) != 0) {
//             fprintf(stderr, "Error registering fd %d\n", fd);
//             continue;
//         }
//         // Simulate some work
//         usleep(100);
//         if (creg_unregister(registry, fd) != 0) {
//             fprintf(stderr, "Error unregistering fd %d\n", fd);
//         }
//     }
//     return NULL;
// }

// int main() {
//     registry = creg_init();
//     if (registry == NULL) {
//         fprintf(stderr, "Failed to initialize client registry\n");
//         return EXIT_FAILURE;
//     }

//     pthread_t threads[NUM_THREADS];
//     int thread_nums[NUM_THREADS];

//     // Create threads
//     for (int i = 0; i < NUM_THREADS; i++) {
//         thread_nums[i] = i;
//         if (pthread_create(&threads[i], NULL, test_thread, &thread_nums[i]) != 0) {
//             perror("Failed to create thread");
//             return EXIT_FAILURE;
//         }
//     }

//     // Wait for threads to finish
//     for (int i = 0; i < NUM_THREADS; i++) {
//         pthread_join(threads[i], NULL);
//     }

//     // Test creg_wait_for_empty
//     printf("Waiting for client registry to become empty...\n");
//     creg_wait_for_empty(registry);
//     printf("Client registry is now empty.\n");

//     creg_fini(registry);
//     return EXIT_SUCCESS;
// }
