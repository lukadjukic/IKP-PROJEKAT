#include "server_helper.h"

#pragma region reciever

static inline bool program_is_running(coordination_t * const coordination) {
    pthread_mutex_lock(&(coordination->lock));
    coordination_flags_t state = coordination->state;
    pthread_mutex_unlock(&(coordination->lock));
    return state != COORD_HALT;
}

static inline bool would_block(const ssize_t recieved_length) {
    return (recieved_length < 0) && (errno == EWOULDBLOCK || errno == EAGAIN);
}

static inline int recieve_request(request_t * const buffer, sockaddr_in_t * const client_address, socklen_t * const address_length, const int server_socket) {
    const ssize_t recieved_length = recvfrom(server_socket, buffer, sizeof(request_t), MSG_DONTWAIT, (sockaddr_t *)client_address, address_length);

    if (would_block(recieved_length)) {
        // printf("No requests to process. Sleeping ...\n");
        sleep(1);
        return RECIEVE_FAILURE;
    } else if (recieved_length < 0) {
        perror("Recieve failed.\n");
        return RECIEVE_FAILURE;
    } else if ((size_t)recieved_length < sizeof(request_t)) {
        perror("Recieved request invalid format.\n");
        return RECIEVE_FAILURE;
    }

    return RECIEVE_SUCCESS;
}

static inline void fill_request(request_t * const prequest, const sockaddr_in_t * const client_address, const size_t id) {
    memcpy(&(prequest->client_addr), client_address, sizeof(sockaddr_in_t));
    prequest->id            = id;
    prequest->flags.handled = 0;
    prequest->flags.invalid = 0;
    prequest->flags.stopped = 0;
    prequest->flags.padding = 0;
    prequest->result        = 0;
}

static inline void send_id(const int server_socket, const size_t * counter, const sockaddr_in_t * client_address) {
    sendto(server_socket, counter, sizeof(size_t), 0, (sockaddr_t *)client_address, sizeof(sockaddr_t));
}

static unsigned char recieved_valid_request(request_t * const prequest, size_t * counter, const int server_socket) {
    sockaddr_in_t client_address;
    socklen_t     address_length = sizeof(sockaddr_in_t);

    if (!recieve_request(prequest, &client_address, &address_length, server_socket)) {
        return RECIEVE_FAILURE;
    }

    fill_request(prequest, &client_address, ++(*counter));
    send_id(server_socket, counter, &client_address);
    printf("[Reciever] Client: %s:%u sent request. Assigned id: %lu\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port), prequest->id);
    return RECIEVE_SUCCESS;
}

static void recieve_requests(queue_t * const pqueue, coordination_t * const coordination, const int server_socket) {
    request_t buffer;
    size_t    counter = 0;

    while (program_is_running(coordination)) {
        if (!recieved_valid_request(&buffer, &counter, server_socket)) {
            continue;
        }

        request_t * request = (request_t *)malloc(sizeof(request_t));
        if (!request) {
            continue; // Should halt? Maybe.
        }

        memcpy(request, &buffer, sizeof(request_t));
        enqueue(pqueue, request);
    }
}

void reciever(queue_t * const pqueue, coordination_t * const coordination, const int server_socket) {
    printf("[DEBUG] Server listening on port: %u ...\n", SERVER_PORT);
    register_producer(pqueue);
    recieve_requests(pqueue, coordination, server_socket);
    deregister_producer(pqueue);
}

#pragma endregion

#pragma region sender

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
    while (1) {
        request = dequeue(pqueue);

        if (!request) {
            break;
        }

        send_processed_request(request, server_socket);
        safe_dealocalte((void **)&request);
    }
}

#pragma endregion
