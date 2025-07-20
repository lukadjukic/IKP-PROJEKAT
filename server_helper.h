#ifndef SERVER_HELPER_HEADER
#define SERVER_HELPER_HEADER
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <math.h>
#include "queue.h"
#include "direcive.h"

#define RECIEVE_SUCCESS 1
#define RECIEVE_FAILURE 0

#define INVALID_SOCKET          -1
#define BINDING_FAILURE         -2
#define THREAD_CREATION_FAILURE -3
#define LISTENER_MODE_FAILURE   -4
#define BUFFER_SIZE             1024
#define SERVER_PORT             11000
#define LISTENER_PORT           11001
#define WORKER_NUMBER           (5) 
#define QUEUE_NUMBER            (WORKER_NUMBER + 2)
#define THREAD_NUMBER           (WORKER_NUMBER + 3)

extern mutex_t mut;

typedef struct threads_t {
    pthread_t * reciever_thread;
    pthread_t * load_balancer_thread;
    pthread_t * sender_thread;
    pthread_t * admin_thread;
    pthread_t * worker_threads;
    size_t      workers_count;
} threads_t;

typedef enum {
    CONNECTION_PERSIST      = 0,
    CONNECTION_END_NO_HALT  = 1,
    CONNECTION_END_HALT     = 2,
} connection_status_t;

typedef struct arguments_t {
    queue_t *        input;
    queue_t *        output;
    ssize_t          num;   
    coordination_t * coordination;      
} arguments_t;

int  init_socket(const int type, const int protocol, const unsigned short port);
void init_queues(QUEUE * const queues, const unsigned short queues_count);
void init_coordination(coordination_t * const coordination);
void delete_coordination(coordination_t * const coordination);
void delete_all_queues(QUEUE * const queues, const unsigned short queues_count);
void join_all_threads(const threads_t const * all_threads);
void safe_dealocalte(void ** ptr);
void reciever(queue_t * const pqueue,  coordination_t * const coordination, const int server_socket);
void sender(queue_t * const pqueue, const int server_socket);
void load_balancer(queue_t * const input,queue_t * const worker_queues);
void admin(arguments_t * const arguments);
void worker(queue_t * const input, queue_t * const output, const int id);

#endif