#include "server_internals.h"

static void init_socket_address(sockaddr_in_t * const server_address, const unsigned short port) {
    memset(server_address, 0, sizeof(sockaddr_in_t));
    server_address->sin_family      = AF_INET;
    server_address->sin_port        = htons(port);
    server_address->sin_addr.s_addr = INADDR_ANY;
}

static int create_socket(const int type, const int protocol) {
    int server_socket;
    server_socket = socket(AF_INET, type, protocol);
    if (server_socket < 0) {
        perror("Socket creation failed.\n");
        return INVALID_SOCKET;
    }
    return server_socket;
}

static int bind_server_socket(const sockaddr_in_t * const server_address, const int server_socket) {
    int binding_failure = bind(server_socket, (sockaddr_t *)server_address, sizeof(sockaddr_in_t));
    if (binding_failure) {
        perror("Socket binding failed.\n");
    }
    return binding_failure;
}

int init_socket(const int type, const int protocol, const unsigned short port) {
    sockaddr_in_t server_address;
    int           server_socket;
    int           binding_failure;

    init_socket_address(&server_address, port);
    server_socket = create_socket(type, protocol);

    if (server_socket < 0) {
        return INVALID_SOCKET;
    }

    binding_failure = bind_server_socket(&server_address, server_socket);

    if (binding_failure) {
        close(server_socket);
        return BINDING_FAILURE;
    }

    return server_socket;
}

void safe_dealocalte(void ** ptr) {
    if (ptr && *ptr) {
        free(*ptr);
        *ptr = NULL;
    }
}

void init_coordination(coordination_t * const coordination) {
    pthread_mutex_init(&(coordination->lock), NULL);
    coordination->state = COORD_START;
}

void delete_coordination(coordination_t * const coordination) {
    coordination->state = COORD_HALT;
    pthread_mutex_destroy(&(coordination->lock));
}
