#include "server_helper.h"

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


//  Wrapper functions
void* _reciever(void* arg) {
    printf("[DEBUG] Reciever thread started.\n");
    struct arguments_t* arguments =  ( struct arguments_t * ) arg;
    reciever(arguments -> output, arguments -> coordination, arguments -> num);
    printf("[DEBUG] Reciever thread finished.\n");
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

void* _sender(void* arg) {
    struct arguments_t* arguments =  ( struct arguments_t * ) arg;
    printf("[DEBUG] Sender thread started.\n");
    sender( arguments -> input, arguments -> num );
    printf("[DEBUG] Sender thread finished.\n");
    return NULL;
}

void * _admin(void * params) {
    arguments_t * arguments = (arguments_t *) params; 
    printf("[DEBUG] Admin thread started.\n");
    admin(arguments);
    printf("[DEBUG] Admin thread finished.\n");
}

//  First queue  is input  (index 0)
//  Second queue is output (index 1)
//  The rest are workers   (indexes 2-)
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
    coordinate.state = COORD_START;
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

