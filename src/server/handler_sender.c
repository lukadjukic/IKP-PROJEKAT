#include "server_internals.h"

static inline void send_processed_request(const request_t * const request, const int server_socket) {
    ssize_t      sent_length;
    sockaddr_t * client_address;

    client_address = (sockaddr_t *)&(request->client_addr);
    sent_length    = sendto(server_socket, request, sizeof(request_t), 0, client_address, sizeof(sockaddr_in_t));

    if (sent_length < 0) {
        perror("Send failed.\n");
    }
}

void sender(queue_t * const pqueue, const int server_socket) {
    request_t * request;
    while (true) {
        request = dequeue(pqueue);

        if (!request) {
            break;
        }

        send_processed_request(request, server_socket);
        safe_dealocalte((void **)&request);
    }
}
