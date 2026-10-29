#ifndef SERVER_HELPER_HEADER
#define SERVER_HELPER_HEADER
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "./common/queue.h"
#include "./common/directive.h"

#define RECIEVE_SUCCESS 1
#define RECIEVE_FAILURE 0

#define INVALID_SOCKET          (-1)
#define BINDING_FAILURE         (-2)
#define THREAD_CREATION_FAILURE (-3)
#define LISTENER_MODE_FAILURE   (-4)
#define BUFFER_SIZE             (1024)
#define SERVER_PORT             (11000)
#define LISTENER_PORT           (11001)
#define WORKER_NUMBER           (5)
#define QUEUE_NUMBER            (WORKER_NUMBER + 2)
#define THREAD_NUMBER           (WORKER_NUMBER + 3)

extern mutex_t mut;

typedef enum coordination_flags_t {
    COORD_START = 0x01,
    COORD_HALT  = 0x02
} coordination_flags_t;

typedef struct coordination_t {
    coordination_flags_t state;
    mutex_t              lock;
} coordination_t;

typedef struct threads_t {
    pthread_t * reciever_thread;
    pthread_t * load_balancer_thread;
    pthread_t * sender_thread;
    pthread_t * admin_thread;
    pthread_t * worker_threads;
    size_t      workers_count;
} threads_t;

typedef enum {
    CONNECTION_PERSIST     = 0,
    CONNECTION_END_NO_HALT = 1,
    CONNECTION_END_HALT    = 2,
} connection_status_t;

typedef struct arguments_t {
    queue_t *        input;
    queue_t *        output;
    ssize_t          num;
    coordination_t * coordination;
} arguments_t;

int  init_socket(const int type, const int protocol, const unsigned short port);
void init_queues(queue_t * const queues, const unsigned short queues_count);
void init_coordination(coordination_t * const coordination);
void delete_coordination(coordination_t * const coordination);
void delete_all_queues(queue_t * const queues, const unsigned short queues_count);
void join_all_threads(const threads_t * const all_threads);
void safe_dealocalte(void ** ptr);
void reciever(queue_t * const pqueue, coordination_t * const coordination, const int server_socket);
void sender(queue_t * const pqueue, const int server_socket);
void load_balancer(queue_t * const input, queue_t * const worker_queues);
void admin(arguments_t * const arguments);
void worker(queue_t * const input, queue_t * const output, const int id);

void    coordinate(coordination_t * const coordination, const coordination_flags_t new_state);
ssize_t handle_directive(queue_t * const queues, const size_t workers_count, coordination_t * const coordinate, const directive_t * const directive);
ssize_t handle_halt(coordination_t * const coordination);
ssize_t handle_thread_stop(queue_t * const queues, const size_t workers_count, const size_t id);
ssize_t handle_thread_continue(queue_t * const queues, const size_t workers_count, const size_t id);
ssize_t handle_thread_increase_capacity(queue_t * const queues, const size_t workers_count, const size_t id, const size_t increment);

#endif