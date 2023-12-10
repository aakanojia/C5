#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include "data.h"
#include "debug.h"
#include "client_registry.h"
#include "transaction.h"
#include "store.h"
#include "protocol.h"


CLIENT_REGISTRY *client_registry;

// xacto_client_service function
void *xacto_client_service(void *arg) {
    int *client_fd_ptr = (int *)arg;
    int client_fd = *client_fd_ptr;
    free(client_fd_ptr); // Free the storage of the file descriptor

    pthread_detach(pthread_self()); // Detach the thread
    creg_register(client_registry, client_fd); // Register client

    TRANSACTION *transaction = trans_create(); // Create a transaction
    if (transaction == NULL) {
        // Handle transaction creation failure
        close(client_fd);
        creg_unregister(client_registry, client_fd);
        return NULL;
    }

    while (1) {
        XACTO_PACKET pkt;
        void *payload = NULL;
        if (proto_recv_packet(client_fd, &pkt, &payload) == -1) {
            // Handle receive error or disconnection
            trans_abort(transaction); // This consumes a reference
            break;
        }

        XACTO_PACKET reply_pkt;
        memset(&reply_pkt, 0, sizeof(reply_pkt));
        reply_pkt.type = XACTO_REPLY_PKT;
        reply_pkt.serial = pkt.serial; // Echo the serial number

        switch (pkt.type) {
            case XACTO_PUT_PKT: {
                // Receive KEY and VALUE packets
                XACTO_PACKET key_pkt, value_pkt;
                void *key_payload = NULL, *value_payload = NULL;
                if (proto_recv_packet(client_fd, &key_pkt, &key_payload) == -1 ||
                    proto_recv_packet(client_fd, &value_pkt, &value_payload) == -1) {
                    free(key_payload); // Free payloads if they were allocated
                    free(value_payload);
                    trans_abort(transaction);
                    break;
                }

                // Create KEY and VALUE blobs
                KEY *key = key_create(blob_create(key_payload, key_pkt.size));
                BLOB *value = blob_create(value_payload, value_pkt.size);

                // Perform store_put operation
                TRANS_STATUS status = store_put(transaction, key, value);

                // Prepare and send the reply packet
                reply_pkt.status = (status == TRANS_ABORTED) ? -1 : 0;

                // Clean up
                free(key_payload); // These are copied by blob_create, so we can free them
                free(value_payload);
                key_dispose(key);
                // blob_unref(value, "Disposing value blob");
                break;
            }
            case XACTO_GET_PKT: {
                // Receive KEY packet
                XACTO_PACKET key_pkt;
                void *key_payload = NULL;
                if (proto_recv_packet(client_fd, &key_pkt, &key_payload) == -1) {
                    free(key_payload); // Free payload if allocated
                    trans_abort(transaction); // This consumes a reference
                    break;
                }

                // Create KEY blob
                KEY *key = key_create(blob_create(key_payload, key_pkt.size));
                BLOB *value = NULL;

                // Perform store_get operation
                TRANS_STATUS status = store_get(transaction, key, &value);

                // Prepare and send the reply packet with value if successful
                reply_pkt.status = (status == TRANS_ABORTED) ? -1 : 0;
                if (status != TRANS_ABORTED && value != NULL) {
                    // Send the value back to the client
                    proto_send_packet(client_fd, &reply_pkt, value->content);
                }

                // Clean up
                free(key_payload);
                key_dispose(key);
                if (value != NULL) {
                    blob_unref(value, "Disposing value blob"); // Unref value if it was retrieved
                }
                break;
            }
            case XACTO_COMMIT_PKT: {
                // Perform trans_commit operation
                TRANS_STATUS status = trans_commit(transaction);
                // Prepare and send the commit status
                reply_pkt.status = (status == TRANS_COMMITTED) ? 0 : -1;
                break;
            }
            // ... other cases
        }

        free(payload); // Free the payload if any

        // Send a reply packet back to the client
        if (proto_send_packet(client_fd, &reply_pkt, NULL) == -1) {
            // Handle send error
            trans_abort(transaction);
            break;
        }

        // Check if transaction commits or aborts
        if (trans_get_status(transaction) != TRANS_PENDING) {
            break; // Exit the loop
        }
    }

    // Cleanup and close the client connection
    close(client_fd);
    creg_unregister(client_registry, client_fd);

    return NULL;
}