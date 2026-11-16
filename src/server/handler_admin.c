#include "server_helper.h"

#pragma region directive handler

static inline bool index_out_of_range(const size_t index, const size_t workers_count) {
    return (index > workers_count - 1);
}

static inline bool increment_too_big(const size_t increment, const size_t max_increment) {
    return (increment > max_increment);
}

static inline bool capacity_too_big(const size_t capacity, const size_t increment) {
    return (capacity + increment < capacity);
}

static inline void coordinate(coordination_t * const coordination, const coordination_flags_t new_state) {
    pthread_mutex_lock(&(coordination->lock));
    coordination->state = new_state;
    pthread_mutex_unlock(&(coordination->lock));
}

ssize_t handle_halt(coordination_t * const coordination) {
    coordinate(coordination, COORD_HALT);
    return DIRECTIVE_SUCCESS;
}

static inline ssize_t handle_thread_stop(queue_t * const queues, const size_t workers_count, const size_t id) {
    queue_t * const queue = queues + id;

    if (index_out_of_range(id, workers_count)) {
        return INDEX_OUT_OF_RANGE;
    }

    stop_queue(queue);
    return DIRECTIVE_SUCCESS;
}

static inline ssize_t handle_thread_continue(queue_t * const queues, const size_t workers_count, const size_t id) {
    queue_t * const queue = queues + id;

    if (index_out_of_range(id, workers_count)) {
        return INDEX_OUT_OF_RANGE;
    }

    resume_queue(queue);
    return DIRECTIVE_SUCCESS;
}

static inline ssize_t handle_thread_increase_capacity(queue_t * const queues, const size_t workers_count, const size_t id, const size_t increment) {
    queue_t * const queue = queues + id; // id == index

    if (index_out_of_range(id, workers_count)) {
        return INDEX_OUT_OF_RANGE;
    } else if (increment_too_big(increment, MAX_INCREMENT)) {
        return INCREMENT_TOO_BIG;
    } else if (capacity_too_big(queue->capacity, increment)) {
        return CAPACITY_TOO_BIG;
    }

    increment_queue_capacity(queue, increment);
    return DIRECTIVE_SUCCESS;
}

ssize_t handle_directive(queue_t * const queues, const size_t workers_count, coordination_t * const coordinate, const directive_t * const directive) {
    print_directive(directive);
    ssize_t result;

    switch (directive->dir) {
    case DIR_HALT:
        result = handle_halt(coordinate);
        break;
    case DIR_THEAD_STOP:
        result = handle_thread_stop(queues, workers_count, directive->id);
        break;
    case DIR_THREAD_CONTINUE:
        result = handle_thread_continue(queues, workers_count, directive->id);
        break;
    case DIR_THREAD_INCREASE_CAPACITY:
        result = handle_thread_increase_capacity(queues, workers_count, directive->id, directive->capacity);
        break;
    default:
        result = INVALID_DIRECTIVE;
        break;
    }

    if (result > 0) {
        printf("[Admin] Directive handled successfully.\n");
    } else {
        printf("[Admin] Directive handled unsuccessfully.\n");
    }

    return result;
}

#pragma endregion

#pragma region admin

static inline bool is_invalid_socket(const int socket) {
    return socket < 0;
}

static inline bool should_end_program(const connection_status_t connection_status) {
    return connection_status == CONNECTION_END_HALT;
}

static inline bool should_halt(ssize_t result, dcode_t directive_code) {
    return (result > 0) && (directive_code == DIR_HALT);
}

static int init_listener_socket(const int port) {
    int listener_socket = init_socket(SOCK_STREAM, 0, port);

    if (listener_socket < 0) {
        perror("Listener socket could not be initialized.\n");
        return -1;
    }

    if (listen(listener_socket, -1) < 0) {
        perror("Listener socket could not be put in listen mode.\n");
        shutdown(listener_socket, SHUT_RDWR);
        close(listener_socket);
        return -1;
    }

    return listener_socket;
}

static int connect_admin(const int listener_socket) {
    sockaddr_in_t admin_address;
    socklen_t     admin_address_len = sizeof(sockaddr_in_t);
    int           admin_socket;

    admin_socket = accept(listener_socket, (sockaddr_t *)&admin_address, &admin_address_len);

    if (admin_socket < 0) {
        printf("[Admin] Admin socket accepting failed.\n");
    } else {
        printf("[Admin] Connected from %s:%u\n", inet_ntoa(admin_address.sin_addr), ntohs(admin_address.sin_port));
    }

    return admin_socket;
}

static connection_status_t handle_single_directive(arguments_t * const arguments, const int admin_socket) {
    directive_t buffer;

    ssize_t recieved_length = recv(admin_socket, &buffer, sizeof(directive_t), 0);
    if (recieved_length < 0) {
        // printf("[Admin] Disconnected.\n");
        return CONNECTION_END_NO_HALT;
    } else if ((size_t)recieved_length != sizeof(directive_t)) {
        ssize_t error = INVALID_DIRECTIVE;
        printf("[Admin] Recieved directive has invalid format. Size : %ld\n", recieved_length);
        send(admin_socket, &error, sizeof(ssize_t), 0);
        return CONNECTION_PERSIST;
    }

    ssize_t result = handle_directive(arguments->input, arguments->num, arguments->coordination, &buffer);
    send(admin_socket, &result, sizeof(ssize_t), 0);

    return should_halt(result, buffer.dir) ? CONNECTION_END_HALT : CONNECTION_PERSIST;
}

static void communicate_with_admins(arguments_t * const arguments, const int listener_socket) {
    while (true) {
        int admin_socket = connect_admin(listener_socket);
        if (is_invalid_socket(admin_socket)) {
            continue;
        }

        connection_status_t connection_status;
        while (true) {
            connection_status = handle_single_directive(arguments, admin_socket);
            if (connection_status != CONNECTION_PERSIST) {
                break;
            }
        }

        close(admin_socket);

        if (should_end_program(connection_status))
            break;
    }
}

void admin(arguments_t * const arguments) {
    int listener_socket = init_listener_socket(LISTENER_PORT);

    if (is_invalid_socket(listener_socket)) {
        handle_halt(arguments->coordination);
        return;
    }

    communicate_with_admins(arguments, listener_socket);

    shutdown(listener_socket, SHUT_RDWR);
    close(listener_socket);
}

#pragma endregion
