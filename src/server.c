#include "server_helper.h"
//  Wrapper functions
void * _reciever(void * arg) {
    printf("[DEBUG] Reciever thread started.\n");
    struct arguments_t* arguments =  ( struct arguments_t * ) arg;
    reciever(arguments -> output, arguments -> coordination, arguments -> num);
    printf("[DEBUG] Reciever thread finished.\n");
    return NULL;
}

void * _load_balancer(void * arg) {
    struct arguments_t* arguments = ( struct arguments_t * ) arg;
    printf("[DEBUG] Load balancer thread started.\n");
    load_balancer(arguments -> input, arguments -> output);
    printf("[DEBUG] Load balancer thread finished.\n");
    return NULL;
}

void * _worker(void * arg) {
    struct arguments_t * arguments = ( struct arguments_t * ) arg;
    int i = arguments -> num;
    printf("[DEBUG] Worker id: %d started.\n", i);
    worker((arguments -> input ) + i, arguments -> output, i);
    printf("[DEBUG] Worker id: %d finished.\n", i);
    return NULL;
}

void * _sender(void * arg) {
    struct arguments_t* arguments =  ( struct arguments_t * ) arg;
    printf("[DEBUG] Sender thread started.\n");
    sender( arguments -> input, arguments -> num );
    printf("[DEBUG] Sender thread finished.\n");
    return NULL;
}

void * _admin(void * arg) {
    arguments_t * arguments = (arguments_t *) arg; 
    printf("[DEBUG] Admin thread started.\n");
    admin(arguments);
    printf("[DEBUG] Admin thread finished.\n");
    return NULL;
}

int create_all_threads(threads_t * const all_threads, const arguments_t * const arguments, const  arguments_t * const worker_arguments) {
  
    if ( pthread_create( all_threads -> reciever_thread, NULL, _reciever, (void*) (arguments + 0) ) ) {
        perror("Creation of thread reciever failed.\n");
        return THREAD_CREATION_FAILURE;
    }

    if ( pthread_create(all_threads -> load_balancer_thread, NULL, _load_balancer, (void*) (arguments + 1) ) ) {
        perror("Creation of thread load balancer failed.\n");
        return THREAD_CREATION_FAILURE;
    }

    for (size_t i = 0; i < all_threads -> workers_count; ++i ) {
        if ( pthread_create(all_threads -> worker_threads + i, NULL, _worker, (void*) (worker_arguments + i) )) {
            perror("Creation of worker threads failed.\n");
            return THREAD_CREATION_FAILURE;
        }
    }

    if ( pthread_create(all_threads -> sender_thread, NULL, _sender, (void*) (arguments + 2)) ) {
        perror("Creation of thread sender failed.\n");
        return THREAD_CREATION_FAILURE;
    }

    if ( pthread_create(all_threads -> admin_thread, NULL, _admin, (void *) (arguments + 3)) ) {
        perror("Creation of thread admin failed.\n");
        return THREAD_CREATION_FAILURE;
    }

    return 0;
}

void init_worker_args(arguments_t * const worker_arguments, queue_t * const input,  queue_t * const output, const size_t workers_count ) {
    for (size_t i = 0; i < workers_count; ++i) {
        worker_arguments[i].input        = input;
        worker_arguments[i].output       = output;
        worker_arguments[i].coordination = NULL; 
        worker_arguments[i].num          = i;
    }
}

//  First queue  is input  (index 0)
//  Second queue is output (index 1)
//  The rest are workers   (indexes 2-)
void init_main_args(arguments_t * const main_arguments, queue_t * const queues, coordination_t * const coordinate, const int main_socket, const size_t worker_count) {
    
    arguments_t  reciever_args  = { NULL,          queues + 0,    main_socket,  coordinate };
    arguments_t  balancer_args  = { queues + 0,    queues + 2,    0,            NULL       };
    arguments_t  sender_args    = { queues + 1,    NULL,          main_socket,  NULL       };
    arguments_t  admin_args     = { queues + 2,    NULL,          worker_count, coordinate };

    memcpy(main_arguments + 0, &reciever_args,  sizeof(arguments_t));
    memcpy(main_arguments + 1, &balancer_args,  sizeof(arguments_t));
    memcpy(main_arguments + 2, &sender_args,    sizeof(arguments_t));
    memcpy(main_arguments + 3, &admin_args,     sizeof(arguments_t));
    
}

int main() {
    int             main_socket;
    queue_t         queues[QUEUE_NUMBER];
    coordination_t  coordinate;
    pthread_t       reciever, load_balancer, sender, admin, workers[WORKER_NUMBER];
    threads_t       all_threads = { &reciever, &load_balancer, &sender, &admin, workers, WORKER_NUMBER };
    arguments_t     worker_arguments[WORKER_NUMBER];
    arguments_t     main_arguments[4];

    main_socket = init_socket(SOCK_DGRAM, 0, SERVER_PORT);
    if ( main_socket < 0 ) {
        return main_socket;
    }

    init_coordination(&coordinate);
    init_queues(queues , QUEUE_NUMBER);
    pthread_mutex_init(&mut, NULL);
    init_main_args(main_arguments, queues, &coordinate, main_socket, WORKER_NUMBER);
    init_worker_args(worker_arguments, queues + 2, queues + 1, WORKER_NUMBER);

    if ( create_all_threads( &all_threads, main_arguments, worker_arguments) == THREAD_CREATION_FAILURE ) {
        handle_halt(&coordinate);
    }

    join_all_threads(&all_threads); 
    delete_coordination(&coordinate);
    delete_all_queues(queues, QUEUE_NUMBER);
    pthread_mutex_destroy(&mut); 
    close(main_socket);
    return 0;
}

