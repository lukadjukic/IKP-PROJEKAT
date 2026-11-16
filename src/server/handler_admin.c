#include "server_helper.h"

#pragma region admin

static void init_listener_socket(int * const listener_socket, int * const error_code, const int port) {
    *listener_socket = init_socket(SOCK_STREAM, 0, port);

    if (*listener_socket < 0) {
        perror("Listener socket could not be initialized.\n");
        *error_code = *listener_socket;
        return;
    }

    if (listen(*listener_socket, -1) < 0) {
        perror("Listener socket could not be put in listen mode.\n");
        *error_code = LISTENER_MODE_FAILURE;
        return;
    }

    *error_code = 0;
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

static inline unsigned char is_invalid_socket(const int socket) {
    return socket < 0;
}

static inline unsigned char should_end_program(const connection_status_t connection_status) {
    return connection_status == CONNECTION_END_HALT;
}

static ssize_t recieve_and_validate_directive(directive_t * const buffer, const int admin_socket) {
    ssize_t error = INVALID_DIRECTIVE;
    ssize_t recieved_data_length;

    recieved_data_length = recv(admin_socket, buffer, sizeof(directive_t), 0);

    if (recieved_data_length == 0) {
        // printf("[Admin] Disconnected.\n");
    } else if ((size_t)recieved_data_length < sizeof(directive_t)) {
        printf("[Admin] Recieved directive has invalid format. Size : %ld\n", recieved_data_length);
        send(admin_socket, &error, sizeof(ssize_t), 0);
    }

    return recieved_data_length;
}

static connection_status_t should_halt(ssize_t result, dcode_t directive_code) {
    unsigned char halt = (result > 0) && (directive_code == DIR_HALT);
    return halt ? CONNECTION_END_HALT : CONNECTION_PERSIST;
}

static ssize_t get_result(arguments_t * const arguments, const directive_t * const directive) {
    ssize_t result;

    result = handle_directive(arguments->input, arguments->num, arguments->coordination, directive);

    if (result > 0) {
        printf("[Admin] Directive handled successfully.\n");
    } else {
        printf("[Admin] Directive handled unsuccessfully.\n");
    }

    return result;
}

static ssize_t respond_to_admin(arguments_t * const arguments, const directive_t * const directive, const int admin_socket) {
    ssize_t result;

    print_directive(directive);
    result = get_result(arguments, directive);
    send(admin_socket, &result, sizeof(ssize_t), 0);

    return result;
}

static connection_status_t handle_single_directive(arguments_t * const arguments, const int admin_socket) {
    directive_t buffer;
    ssize_t     recieved_length;
    ssize_t     result;

    recieved_length = recieve_and_validate_directive(&buffer, admin_socket);
    if (recieved_length == 0)
        return CONNECTION_END_NO_HALT;
    else if ((size_t)recieved_length < sizeof(directive_t))
        return CONNECTION_PERSIST;

    result = respond_to_admin(arguments, &buffer, admin_socket);
    return should_halt(result, buffer.dir);
}

static connection_status_t communicate_with_admin(arguments_t * const arguments, const int admin_socket) {
    connection_status_t status;
    while (1) {
        status = handle_single_directive(arguments, admin_socket);
        if (status != CONNECTION_PERSIST) {
            return status;
        }
    }
}

static void communicate_with_admins(arguments_t * const arguments, const int listener_socket) {
    int                 admin_socket;
    connection_status_t connection_status;

    while (1) {

        admin_socket = connect_admin(listener_socket);
        if (is_invalid_socket(admin_socket)) {
            continue;
        }

        connection_status = communicate_with_admin(arguments, admin_socket);
        close(admin_socket);

        if (should_end_program(connection_status))
            break;
    }
}

void admin(arguments_t * const arguments) {
    int listener_socket;
    int error_code;

    init_listener_socket(&listener_socket, &error_code, LISTENER_PORT);
    if (error_code) {
        handle_halt(arguments->coordination);
        return;
    }

    communicate_with_admins(arguments, listener_socket);

    shutdown(listener_socket, SHUT_RDWR);
    close(listener_socket);
}

#pragma endregion

#pragma region directive handler

static inline unsigned char index_out_of_range(const size_t index, const size_t workers_count) {
    return (index > workers_count - 1);
}

static inline unsigned char increment_too_big(const size_t increment, const size_t max_increment) {
    return (increment > max_increment);
}

static inline unsigned char capacity_too_big(const size_t capacity, const size_t increment) {
    return (capacity + increment < capacity);
}

void coordinate(coordination_t * const coordination, const coordination_flags_t new_state) {
    pthread_mutex_lock(&(coordination->lock));
    coordination->state = new_state;
    pthread_mutex_unlock(&(coordination->lock));
}

ssize_t handle_directive(queue_t * const queues, const size_t workers_count, coordination_t * const coordinate, const directive_t * const directive) {
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

    return result;
}

ssize_t handle_halt(coordination_t * const coordination) {
    coordinate(coordination, COORD_HALT);
    return DIRECTIVE_SUCCESS;
}

ssize_t handle_thread_stop(queue_t * const queues, const size_t workers_count, const size_t id) {
    queue_t * const queue = queues + id;
    if (index_out_of_range(id, workers_count)) {
        return INDEX_OUT_OF_RANGE;
    }
    stop_queue(queue);
    return DIRECTIVE_SUCCESS;
}

ssize_t handle_thread_continue(queue_t * const queues, const size_t workers_count, const size_t id) {
    queue_t * const queue = queues + id;
    if (index_out_of_range(id, workers_count)) {
        return INDEX_OUT_OF_RANGE;
    }
    resume_queue(queue);
    return DIRECTIVE_SUCCESS;
}

ssize_t handle_thread_increase_capacity(queue_t * const queues, const size_t workers_count, const size_t id, const size_t increment) {
    queue_t * const queue = queues + id; // id == index

    if (index_out_of_range(id, workers_count)) {
        return INDEX_OUT_OF_RANGE;
    }
    if (increment_too_big(increment, MAX_INCREMENT)) {
        return INCREMENT_TOO_BIG;
    }
    if (capacity_too_big(queue->capacity, increment)) {
        return CAPACITY_TOO_BIG;
    }

    increment_queue_capacity(queue, increment);
    return DIRECTIVE_SUCCESS;
}

#pragma endregion
