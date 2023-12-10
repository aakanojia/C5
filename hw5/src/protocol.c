#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "protocol.h"

int proto_send_packet(int fd, XACTO_PACKET *pkt, void *data) {
    // First, send the packet header
    ssize_t header_size = sizeof(XACTO_PACKET);
    ssize_t sent_bytes = write(fd, pkt, header_size);
    if (sent_bytes != header_size) {
        // Error in sending packet header
        return -1;
    }

    // Check if there is data to send
    if (data != NULL && pkt->size > 0) {
        // Send the data payload
        sent_bytes = write(fd, data, pkt->size);
        if (sent_bytes != pkt->size) {
            // Error in sending data payload
            return -1;
        }
    }

    return 0;
}


int proto_recv_packet(int fd, XACTO_PACKET *pkt, void **datap) {
    // First, receive the packet header
    ssize_t header_size = sizeof(XACTO_PACKET);
    ssize_t received_bytes = read(fd, pkt, header_size);
    if (received_bytes != header_size) {
        // Error in receiving packet header
        return -1;
    }

    // Check if there is a data payload to receive
    if (pkt->size > 0) {
        *datap = malloc(pkt->size);
        if (*datap == NULL) {
            // Memory allocation failure
            return -1;
        }

        // Receive the data payload
        received_bytes = read(fd, *datap, pkt->size);
        if (received_bytes != pkt->size) {
            // Error in receiving data payload
            free(*datap);
            return -1;
        }
    } else {
        *datap = NULL;
    }

    return 0;
}