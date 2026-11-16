#ifndef SERVER_HELPER_HEADER
#define SERVER_HELPER_HEADER
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "../common/queue.h"
#include "../common/directive.h"

#define INVALID_SOCKET  (-1)
#define BINDING_FAILURE (-2)

typedef enum coordination_flags_t {
    COORD_START = 0x01,
    COORD_HALT  = 0x02
} coordination_flags_t;

typedef struct coordination_t {
    coordination_flags_t state;
    mutex_t              lock;
} coordination_t;

typedef enum {
    CONNECTION_PERSIST     = 0,
    CONNECTION_END_NO_HALT = 1,
    CONNECTION_END_HALT    = 2,
} connection_status_t;

void reciever(queue_t * const pqueue, coordination_t * const coordination, const int server_socket, const int server_port);
void sender(queue_t * const pqueue, const int server_socket);
void load_balancer(queue_t * const input, queue_t * const worker_queues, const size_t worker_count);
void admin(queue_t * const worker_queues, const size_t worker_count, coordination_t * coordination, const int listener_port);
void worker(queue_t * const input, queue_t * const output, const int id, const bool * primes, const size_t n);

ssize_t handle_halt(coordination_t * const coordination);
void    sieve_of_erathostenes(bool * primes, size_t n);

int  init_socket(const int type, const int protocol, const unsigned short port);
void init_coordination(coordination_t * const coordination);
void delete_coordination(coordination_t * const coordination);
void safe_dealocalte(void ** ptr);

#endif
