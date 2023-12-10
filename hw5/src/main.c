#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include "server.h"
#include "debug.h"
#include "client_registry.h"
#include "transaction.h"
#include "store.h"
#include "protocol.h"

#define MAX_BACKLOG 5

static void terminate(int status);
static void sighup_handler(int sig);

int main(int argc, char* argv[]) {
    // Option processing
    int port = 0;
    int opt;
    while ((opt = getopt(argc, argv, "p:")) != -1) {
        switch (opt) {
        case 'p':
            port = atoi(optarg);
            break;
        default:
            fprintf(stderr, "Usage: %s -p <port>\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (port == 0) {
        fprintf(stderr, "Port number is required\n");
        exit(EXIT_SUCCESS);
    }

    // Install SIGHUP handler
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sighup_handler;
    sigaction(SIGHUP, &sa, NULL);

    // Initialize modules
    client_registry = creg_init();
    trans_init();
    store_init();

    // Set up server socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_BACKLOG) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    // Accept connections loop
    while (1) {
        int *client_fd = malloc(sizeof(int));
        if (client_fd == NULL) {
            perror("Malloc failed");
            continue;
        }

        *client_fd = accept(server_fd, NULL, NULL);
        if (*client_fd < 0) {
            perror("Accept failed");
            free(client_fd);
            continue;
        }

        pthread_t tid;
        if (pthread_create(&tid, NULL, xacto_client_service, client_fd) != 0) {
            perror("Failed to create thread");
            close(*client_fd);
            free(client_fd);
        }
    }

    // Should not reach here
    terminate(EXIT_SUCCESS);
}

void sighup_handler(int sig) {
    terminate(EXIT_SUCCESS);
}

void terminate(int status) {
    creg_shutdown_all(client_registry);
    debug("Waiting for service threads to terminate...");
    creg_wait_for_empty(client_registry);
    debug("All service threads terminated.");

    creg_fini(client_registry);
    trans_fini();
    store_fini();

    debug("Xacto server terminating");
    exit(status);
}