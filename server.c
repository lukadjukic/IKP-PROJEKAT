#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include "request.h"
#include "direcive.h"

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

typedef struct sockaddr_in sockaddr_in_t;

typedef enum {
    CONNECTION_PERSIST      = 0,
    CONNECTION_END_NO_HALT  = 1,
    CONNECTION_END_HALT     = 2,
} connection_status_t;

typedef struct arguments_t {
    QUEUE*           input;
    QUEUE*           output;
    ssize_t          num;   
    coordination_t * coordination;      
} arguments_t;

int would_block(ssize_t recieved_length) {
    return (recieved_length < 0) && (errno == EWOULDBLOCK || errno == EAGAIN);
}

int recieve_request(REQUEST * buffer, sockaddr_in_t * client_address, socklen_t * address_length, int server_socket ) {
    ssize_t recieved_length = recvfrom(server_socket, buffer, sizeof(REQUEST), MSG_DONTWAIT, (struct sockaddr*)client_address, address_length);
    
    if( would_block(recieved_length) ) {
        // printf("No requests to process. Sleeping ...\n");
        sleep(1);
        return 0;
    } else if (recieved_length < 0){
        perror("Recieve failed.\n");
        return 0;
    } else if(recieved_length < sizeof(REQUEST)){
        perror("Recieved request invalid format.\n");
        return 0;
    }

    return 1;
}

void fill_request(REQUEST * prequest, sockaddr_in_t * client_address, size_t id) {
    memcpy(&(prequest ->client_addr), client_address, sizeof(sockaddr_in_t));
    prequest -> id = id;
}

int recieve_single_request(REQUEST * prequest, size_t * counter, int server_socket) {
    sockaddr_in_t   client_address;
    socklen_t       address_length = sizeof(sockaddr_in_t);

    if ( !recieve_request(prequest, &client_address, &address_length, server_socket)) {
        return 0;
    }

    fill_request(prequest, &client_address, ++(*counter));
    printf("[Reciever] Client: %s:%u sent request. Assigned id: %lu\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port), prequest -> id);
    return 1;
}

void recieve_requests(QUEUE * pqueue, coordination_t * coordination, int server_socket) {
    REQUEST buffer;
    size_t  counter = 0;

    while ( coordination -> flag == COORD_START ) {
        if ( !recieve_single_request(&buffer, &counter, server_socket) ) {
            continue;
        }
        enqueue(pqueue, &buffer);
    }
}

void reciever(QUEUE* pqueue,  coordination_t * coordination, int server_socket) {
    printf("[DEBUG] Server listening on port: %u ...\n", SERVER_PORT);    
    register_producer(pqueue);
    recieve_requests(pqueue, coordination, server_socket);
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
        printf("[Load Balancer] Request id: %lu assignes request to thread %d.\n",req -> id ,i);    
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
        if ( is_queue_stopped(input) ) {
            req -> flags = 1;
        } else {
            req -> flags = 2;
        }
        printf("[Worker %d] Worker handled request id: %lu.\n", id, req -> id);
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
    printf("[DEBUG] Sender thread finished.\n");
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
    printf("[DEBUG] Worker id: %d started.\n", i);
    worker((arguments -> input ) + i, arguments -> output, i);
    printf("[DEBUG] Worker id: %d finished.\n", i);
}

void* _reciever(void* arg) {
    printf("[DEBUG] Reciever thread started.\n");
    struct arguments_t* arguments =  ( struct arguments_t * ) arg;
    reciever(arguments -> output, arguments -> coordination, arguments -> num);
    printf("[DEBUG] Reciever thread finished.\n");
    return NULL;
}

int create_socket(int type, int protocol) {
    int server_socket;
    server_socket = socket(AF_INET, type, protocol);
    if ( server_socket < 0 ) {
        perror("Socket creation failed.\n");
        return INVALID_SOCKET;
    }
    return server_socket;
}

void init_socket_address(sockaddr_in_t * server_address, unsigned short port) {
    memset( server_address, 0, sizeof(sockaddr_in_t) );
    server_address -> sin_family      = AF_INET;
    server_address -> sin_port        = htons(port);
    server_address -> sin_addr.s_addr = INADDR_ANY;
}

int bind_server_socket(sockaddr_in_t * server_address, int server_socket) {
    int binding_failure = bind(server_socket, ( struct sockaddr * ) server_address, sizeof(sockaddr_in_t));
    if (  binding_failure  ) {
        perror("Socket binding failed.\n");
    }
    return binding_failure;
} 

int init_socket(int type, int protocol, int port) {
    sockaddr_in_t server_address;
    int           server_socket;
    int           binding_failure;

    init_socket_address(&server_address, port);
    server_socket = create_socket(type, protocol);
    
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

    if ( pthread_create(threads + 1, NULL, _load_balancer, (void*) (arguments + 1) ) ) {
        perror("Creation of thread load balancer failed.\n");
        return THREAD_CREATION_FAILURE;
    }

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

ssize_t get_result(arguments_t * arguments, directive_t * directive) {
    ssize_t result;

    result = handle_directive(arguments -> input, arguments -> num, arguments -> coordination, directive);
    
    if (result > 0) {
        printf("[Admin] Directive handled successfully.\n");
    } else  {
        printf("[Admin] Directive handled unsuccessfully.\n");
    }
    
    return result;
}

connection_status_t should_halt(ssize_t result, DIRECTIVE directive_code) {
    int halt = (result > 0) && (directive_code == DIR_HALT);
    return halt ? CONNECTION_END_HALT : CONNECTION_PERSIST;
} 

ssize_t respond_to_admin(arguments_t * arguments, directive_t * directive, int admin_socket) {
    ssize_t result;

    print_directive(directive);
    result = get_result(arguments, directive);
    send(admin_socket, &result, sizeof(ssize_t), 0);
    
    return result;
}

ssize_t recieve_and_validate_directive(directive_t * buffer, int admin_socket) {
    ssize_t error = INVALID_DIRECTIVE;
    ssize_t recieved_data_length;
    
    recieved_data_length = recv(admin_socket, buffer, sizeof(directive_t), 0);

    if (recieved_data_length == 0) {
        printf("[Admin] Disconnected.\n");
    } else if (recieved_data_length < sizeof(directive_t)) {
        printf("[Admin] Recieved directive has invalid format. Size : %ld\n", recieved_data_length);
        send(admin_socket, &error, sizeof(ssize_t), 0);        
    }

    return recieved_data_length;
}

connection_status_t handle_single_directive(arguments_t * arguments, int admin_socket) {
    directive_t buffer;
    ssize_t     recv_len;
    ssize_t     result;

    recv_len  = recieve_and_validate_directive(&buffer, admin_socket);
    if (recv_len == 0)
        return CONNECTION_END_NO_HALT;
    else if (recv_len < sizeof(directive_t))
        return CONNECTION_PERSIST;

    result = respond_to_admin(arguments, &buffer, admin_socket);
    return should_halt(result, buffer.dir);
}

connection_status_t communicate_with_admin(arguments_t * arguments, int admin_socket) {
    connection_status_t status;
    while (1) {
        status = handle_single_directive(arguments, admin_socket);
        if (status != CONNECTION_PERSIST) {
            return status;
        }
    }
}

int connect_admin(int listener_socket) {
    sockaddr_in_t admin_address;
    socklen_t     admin_address_len = sizeof(sockaddr_in_t);
    int           admin_socket;

    admin_socket = accept(listener_socket, (struct sockaddr *)&admin_address, &admin_address_len);
    
    if (admin_socket < 0) {
        printf("[Admin] Admin socket accepting failed.\n");
    } else {
        printf("[Admin] Connected from %s:%u\n", inet_ntoa(admin_address.sin_addr), ntohs(admin_address.sin_port));
    }

    return admin_socket;
}

void handle_admins(arguments_t * arguments, int listener_socket) {
    int admin_socket;

    while (1) {

        admin_socket = connect_admin(listener_socket);
        if (admin_socket < 0) {
            continue;
        }

        if (communicate_with_admin(arguments, admin_socket) == CONNECTION_END_HALT) {
            break;
        }
        
        close(admin_socket);
    }
}

void init_listener_socket(int * listener_socket, int * error_code) {
    * listener_socket = init_socket(SOCK_STREAM, 0, LISTENER_PORT); 
    
    if (* listener_socket < 0) {
        perror("Listener socket could not be initialized.\n");
        * error_code = * listener_socket;
        return;
    }

    if (listen(*listener_socket, -1) < 0) {
        perror("Listener socket could not be put in listen mode.\n");
        * error_code = LISTENER_MODE_FAILURE;
        return;
    }

    * error_code = 0;
}

void admin(arguments_t * arguments) {
    int listener_socket;
    int error_code;

    init_listener_socket(&listener_socket, &error_code);
    if (error_code) {
        handle_halt(arguments -> coordination);
        return;
    }
    handle_admins(arguments, listener_socket);
    close(listener_socket);
}

void * _admin(void * params) {
    arguments_t * arguments = (arguments_t *) params; 
    printf("[DEBUG] Admin thread started.\n");
    admin(arguments);
    printf("[DEBUG] Admin thread finished.\n");
}

int main() {
    int       sock;
    QUEUE     queues[QUEUE_NUMBER];
    pthread_t threads[THREAD_NUMBER];
    pthread_t admin;

    sock = init_socket(SOCK_DGRAM, 0, SERVER_PORT);
    if ( sock < 0 ) {
        return sock;
    }

    init_queues(queues , QUEUE_NUMBER);

    coordination_t coordinate;
    pthread_mutex_init( &(coordinate.lock), NULL);
    coordinate.flag = COORD_START;
    arguments_t admin_args      = { queues + 2,    NULL,          WORKER_NUMBER, &coordinate  };

    arguments_t  reciever_args  = { NULL,          queues + 0,    sock, &coordinate };
    arguments_t  balancer_args  = { queues + 0,    queues + 2,    0,    NULL        };
    arguments_t  worker_args    = { queues + 2,    queues + 1,    0,    NULL        };
    arguments_t  sender_args    = { queues + 1,    NULL,          sock, NULL        };
    arguments_t  arguments[4]   = { reciever_args, balancer_args, worker_args, sender_args};

    create_all_threads(threads, THREAD_NUMBER, arguments);

    pthread_create(&admin, NULL, _admin, (void *) &admin_args);
    pthread_join(admin, NULL);
    pthread_mutex_destroy( &(coordinate.lock) );

    join_all_threads(threads, THREAD_NUMBER);   
    close_program(queues, QUEUE_NUMBER, sock);
    return 0;
}

