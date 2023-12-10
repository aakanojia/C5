#include "client_registry.h"
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>

typedef struct client_registry {
    int *client_fds; // Dynamic array of client file descriptors
    int capacity;    // Capacity of the array
    int count;       // Number of clients currently registered
    pthread_mutex_t mutex; // Mutex for thread safety
} CLIENT_REGISTRY;

CLIENT_REGISTRY *creg_init() {
    CLIENT_REGISTRY *cr = malloc(sizeof(CLIENT_REGISTRY));
    if (cr == NULL) return NULL;

    cr->capacity = 10; // Initial capacity
    cr->count = 0;
    cr->client_fds = malloc(cr->capacity * sizeof(int));
    if (cr->client_fds == NULL) {
        free(cr);
        return NULL;
    }
    pthread_mutex_init(&cr->mutex, NULL);
    return cr;
}

void creg_fini(CLIENT_REGISTRY *cr) {
    pthread_mutex_destroy(&cr->mutex);
    free(cr->client_fds);
    free(cr);
}

int creg_register(CLIENT_REGISTRY *cr, int fd) {
    pthread_mutex_lock(&cr->mutex);
    if (cr->count == cr->capacity) {
        // Resize the array if needed
        int new_capacity = cr->capacity * 2;
        int *new_array = realloc(cr->client_fds, new_capacity * sizeof(int));
        if (new_array == NULL) {
            pthread_mutex_unlock(&cr->mutex);
            return -1;
        }
        cr->client_fds = new_array;
        cr->capacity = new_capacity;
    }
    cr->client_fds[cr->count++] = fd;
    pthread_mutex_unlock(&cr->mutex);
    return 0;
}

int creg_unregister(CLIENT_REGISTRY *cr, int fd) {
    pthread_mutex_lock(&cr->mutex);
    for (int i = 0; i < cr->count; i++) {
        if (cr->client_fds[i] == fd) {
            cr->client_fds[i] = cr->client_fds[--cr->count];
            pthread_mutex_unlock(&cr->mutex);
            return 0;
        }
    }
    pthread_mutex_unlock(&cr->mutex);
    return -1;
}

void creg_wait_for_empty(CLIENT_REGISTRY *cr) {
    pthread_mutex_lock(&cr->mutex);
    while (cr->count > 0) {
        pthread_mutex_unlock(&cr->mutex);
        sleep(1); // Wait and check again
        pthread_mutex_lock(&cr->mutex);
    }
    pthread_mutex_unlock(&cr->mutex);
}

void creg_shutdown_all(CLIENT_REGISTRY *cr) {
    pthread_mutex_lock(&cr->mutex);
    for (int i = 0; i < cr->count; i++) {
        shutdown(cr->client_fds[i], SHUT_RDWR);
    }
    pthread_mutex_unlock(&cr->mutex);
}
