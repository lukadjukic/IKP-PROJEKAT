#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "request.h"

#define INVALID_SOCKET          -1
#define BINDING_FAILURE         -2
#define THREAD_CREATION_FAILURE -3
#define BUFFER_SIZE             1024
#define SERVER_PORT             11000
#define WORKER_NUMBER           (5) 
#define QUEUE_NUMBER            (WORKER_NUMBER + 2)
#define THREAD_NUMBER           (WORKER_NUMBER + 3)

typedef struct sockaddr_in sockaddr_in_t;

typedef struct arguments_t {
    QUEUE*      input;
    QUEUE*      output;
    ssize_t     num;         
} arguments_t;

void reciever(QUEUE* pqueue, int sock) {
    sockaddr_in_t client_addr;
    socklen_t addr_len = sizeof(sockaddr_in_t);
    REQUEST buffer;
    printf("[DEBUG] Server listening on port: %u ...\n", SERVER_PORT);
    unsigned char counter = 5;
    register_producer(pqueue);
    while ( --counter ) {
        ssize_t recv_len = recvfrom(sock, &buffer, sizeof(REQUEST), 0, (struct sockaddr*)&client_addr, &addr_len);
    
        if( recv_len < 0 ){
            perror("Recieve failed.\n");
            continue;
        } else if(recv_len < sizeof(REQUEST)){
            perror("Recieved request invalid format.\n");
            continue;
        }

        REQUEST* pr = &buffer;
        printf("Client: %s:%u sent request. Assigned id: %u\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), counter);
        pr -> id = counter;
        memcpy(&(pr -> client_addr), &client_addr, sizeof(sockaddr_in_t));
        enqueue(pqueue, pr);
    }
    deregister_producer(pqueue);
}

void sender(QUEUE* pqueue, int sock) {
    while (1) {
        REQUEST* req = dequeue(pqueue);
        if (!req) {
            break;
        }
        sockaddr_in_t* client_addr = &(req -> client_addr); 
        if( sendto(sock, req, sizeof(REQUEST), 0, (struct sockaddr*)client_addr, sizeof(sockaddr_in_t)) < 0){
            perror("Send failed.\n");
            continue;
        }
        free(req);
        req = NULL;
    }
}

int balance_load(QUEUE* worker_queues) {
    register size_t capacity, size, i;
    while (1) {
        for ( i = 0;  i < WORKER_NUMBER; ++i) {
            capacity = worker_queues[i].capacity;
            size     = worker_queues[i].size;
            if ( !capacity || size < capacity) {
                return i;
            }
        }
        sleep(1);
    }
}

void load_balancer(QUEUE* input, QUEUE* worker_queues) {
    for (int i = 0; i < WORKER_NUMBER; ++i) {
        register_producer(worker_queues + i);
    }
    int i;
    while(1) {
        REQUEST* req = dequeue(input);
        if (!req) {
            break;
        }
        //  Should balance between multiple queues
        i = balance_load(worker_queues);
        printf("[DEBUG] LB assignes request to thread %d.\n", i);    
        enqueue(worker_queues + i, req);
        free(req);
        req = NULL;
    }
    for (int i = 0; i < WORKER_NUMBER; ++i) {
        deregister_producer(worker_queues + i);
    }
}

void worker(QUEUE* input, QUEUE* output, int id) {
    register_producer(output);
    while (1) {
        REQUEST* req = dequeue(input);
        if (!req) {
            break;
        }
        req -> flags = 2;
        printf("[DEBUG] Worker %d handled a request.\n", id);
        enqueue(output, req);
        free(req);
        req = NULL;
    }
    deregister_producer(output);
}

void* _sender(void* arg) {
    struct arguments_t* arguments =  ( struct arguments_t * ) arg;
    printf("[DEBUG] Sender thread started.\n");
    sender( arguments -> input, arguments -> num );
    printf("[DEBUG]Sender thread finished.\n");
    return NULL;
}

void* _load_balancer(void* arg) {
    struct arguments_t* arguments = ( struct arguments_t * ) arg;
    printf("[DEBUG] Load balancer thread started.\n");
    load_balancer(arguments -> input, arguments -> output);
    printf("[DEBUG] Load balancer thread finished.\n");
    return NULL;
}

void* _worker(void* arg) {
    struct arguments_t * arguments = ( struct arguments_t * ) arg;
    int i = arguments -> num;
    printf("Worker id: %d started.\n", i);
    worker((arguments -> input ) + i, arguments -> output, i);
    printf("Worker id: %d finished.\n", i);
}

void* _reciever(void* arg) {
    printf("[DEBUG] Reciever thread started.\n");
    struct arguments_t* arguments =  ( struct arguments_t * ) arg;
    reciever(arguments -> output, arguments -> num);
    printf("[DEBUG] Reciever thread finished.\n");
    return NULL;
}

int create_socket() {
    int server_socket;
    server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if ( server_socket < 0 ) {
        perror("Socket creation failed.\n");
        return INVALID_SOCKET;
    }
    return server_socket;
}

void init_socket_address(sockaddr_in_t * server_address) {
    memset( server_address, 0, sizeof(sockaddr_in_t) );
    server_address -> sin_family      = AF_INET;
    server_address -> sin_port        = htons(SERVER_PORT);
    server_address -> sin_addr.s_addr = INADDR_ANY;
}

int bind_server_socket(sockaddr_in_t * server_address, int server_socket) {
    int binding_failure = bind(server_socket, ( struct sockaddr * ) server_address, sizeof(sockaddr_in_t));
    if (  binding_failure  ) {
        perror("Socket binding failed.\n");
    }
    return binding_failure;
} 

int init_socket() {
    sockaddr_in_t server_address;
    int           server_socket;
    int           binding_failure;

    init_socket_address(&server_address);
    server_socket = create_socket();
    
    if ( server_socket < 0 ) {
        return INVALID_SOCKET;
    }
    
    binding_failure = bind_server_socket( &server_address, server_socket );
    
    if ( binding_failure ) {
        close( server_socket );
        return BINDING_FAILURE;
    }
    
    return server_socket;
}

//  First queue  is input  (index 0)
//  Second queue is output (index 1)
//  The rest are workers   (indexes 2-)
void init_queues(QUEUE * queues, unsigned short queues_count) {
    for ( int i = 0; i < queues_count; ++i ) {
        if ( i < 2) {
            init_queue( queues + i, QUEUE_CAPACITY_UNLIMITED);
        } else {
            init_queue( queues + i, QUEUE_CAPACITY_DEFAULT );
        }
    }
}

void delete_all_queues(QUEUE * queues, unsigned short queues_count) {
    for ( int i = 0; i < queues_count; ++i ) {
        delete_queue( queues + i );
    }
}

int create_all_threads(pthread_t * threads, unsigned short thread_count, arguments_t * arguments) {
    if ( pthread_create(threads + 0, NULL, _reciever, (void*) (arguments + 0) ) ) {
        perror("Creation of thread sender failed.\n");
        return THREAD_CREATION_FAILURE;
    }
    sleep(1);

    if ( pthread_create(threads + 1, NULL, _load_balancer, (void*) (arguments + 1) ) ) {
        perror("Creation of thread load balancer failed.\n");
        return THREAD_CREATION_FAILURE;
    }
    sleep(1);

    for ( int i = 2; i < thread_count - 1; ++i ) {
        if ( pthread_create(threads + i, NULL, _worker, (void*) (arguments + 2) )) {
            perror("Creation of worker threads failed.\n");
            return THREAD_CREATION_FAILURE;
        }
        sleep(1);
        ( (arguments +2) -> num)++;
    }

    if ( pthread_create(threads + thread_count - 1, NULL, _sender, (void*) (arguments + 3)) ) {
        perror("Creation of thread reciever failed.\n");
        return THREAD_CREATION_FAILURE;
    }
    return 0;
}

void join_all_threads(pthread_t * threads, unsigned short thread_count) {
    for ( int i = 0; i < thread_count; ++i ) {
        pthread_join( threads[i], NULL );
    }
}

int close_program(QUEUE * queues, unsigned short queues_count, int server_socket) {
    delete_all_queues(queues, queues_count);
    return close(server_socket);
}

int main() {
    int       sock;
    QUEUE     queues[QUEUE_NUMBER];
    pthread_t threads[THREAD_NUMBER];

    sock = init_socket();
    if ( sock < 0 ) {
        return sock;
    }

    init_queues(queues , QUEUE_NUMBER);

    arguments_t  reciever_args  = { NULL,          queues + 0,    sock };
    arguments_t  balancer_args  = { queues + 0,    queues + 2,    0    };
    arguments_t  worker_args    = { queues + 2,    queues + 1,    0    };
    arguments_t  sender_args    = { queues + 1,    NULL,          sock };
    arguments_t  arguments[4]   = { reciever_args, balancer_args, worker_args, sender_args};

    create_all_threads(threads, THREAD_NUMBER, arguments);

    join_all_threads(threads, THREAD_NUMBER);   
    close_program(queues, QUEUE_NUMBER, sock);
    return 0;
}