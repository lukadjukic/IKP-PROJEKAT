#include <stdio.h>
#include "queue.h"

#pragma region requests

static void print_array(const ssize_t const * array, const size_t array_length) {
    printf("\tArguments:\t[");
    for(size_t i = 0; i < array_length; ++i) {
        printf("%ld", array[i]);
        if(i != array_length -1){
            printf(", ");
        }
    }
    printf("]\n");
}

void print_request(const request_t * const  prequest) {
    printf("Request:\n");
    printf("\tId:\t\t%lu\n", prequest -> id);
    printf("\tClient:\t\t%s\n", inet_ntoa(prequest -> client_addr.sin_addr));
    printf("\tFlags:\t\tS=%u I=%u H=%u \n", prequest -> flags.stopped, prequest -> flags.invalid, prequest -> flags.handled);
    printf("\tOperation code:\t%u\n", prequest -> operation);
    print_array(prequest -> arguments, MAX_ARGUMENTS);
    printf("\tResult:\t\t%ld\n", prequest -> result);
}

#pragma endregion

#pragma region init_delete_register_deregister

void init_queue(queue_t * const pqueue, const size_t _capacity) {
    pqueue -> first          = NULL; 
    pqueue -> last           = NULL;
    pqueue -> size           = 0;
    pqueue -> producer_count = 0;
    pqueue -> capacity       = _capacity;
    pqueue -> flags          = QUEUE_INITIALIZED;
    pthread_mutex_init( &(pqueue -> lock), NULL);
    pthread_cond_init( &(pqueue -> consumers_waiting), NULL);
}

static void delete_all_requests(queue_t * const pqueue) {
    request_t * prequest = NULL;
    while ( pqueue -> first ){
        prequest = dequeue(pqueue);
        if ( prequest )
            free(prequest);
        prequest = NULL;
    }
}

void delete_queue(queue_t * const pqueue) {
    delete_all_requests(pqueue);
    pthread_mutex_destroy( &(pqueue -> lock) );
    pthread_cond_destroy( &(pqueue -> consumers_waiting) );
    pqueue -> first = NULL;
    pqueue -> last  = NULL;
}

void register_producer(queue_t * const pqueue) {
    pthread_mutex_lock( &(pqueue -> lock) );
    pqueue -> flags |= QUEUE_STARTED;
    pqueue -> producer_count++;
    pthread_mutex_unlock( &(pqueue -> lock) );
}

void deregister_producer(queue_t * const pqueue) {
    pthread_mutex_lock( &(pqueue -> lock) );
    if ( --(pqueue -> producer_count) <= 0) {
        pthread_cond_broadcast( &(pqueue -> consumers_waiting) );
    }
    pthread_mutex_unlock( &(pqueue -> lock) );
}

#pragma endregion

#pragma region enqueue

static unsigned char is_queue_full(queue_t * const pqueue) {
    //  If capacity == 0 <==> queue capacity is limitless
    pthread_mutex_lock(&(pqueue ->lock));
    unsigned char result =  (pqueue -> capacity > 0) && (pqueue -> size >= pqueue -> capacity); 
    pthread_mutex_unlock(&(pqueue ->lock));
    return result;
}

static void enqueue_handle_pointers(queue_t * const pqueue, queue_item_t * const pitem) {
    pitem   -> prev = NULL;
    pitem   -> next = pqueue -> last;
    pqueue  -> last = pitem;
    if ( pitem -> next ) {
        pitem   -> next -> prev = pitem; 
        return;
    }
    pqueue -> first = pitem;
}

static void enqueue_item(queue_t * const pqueue, queue_item_t * const pitem) {
    pthread_mutex_lock( &(pqueue -> lock) );
    enqueue_handle_pointers(pqueue, pitem);
    pqueue -> size++;
    pthread_mutex_unlock( &(pqueue -> lock) );
    pthread_cond_signal( &(pqueue -> consumers_waiting) );
}

int enqueue(queue_t * const pqueue, const request_t * const prequest) {
    if ( is_queue_full(pqueue) ) {
        return QUEUE_FULL;   
    }

    queue_item_t * pqueue_item = (queue_item_t *) malloc( sizeof(queue_item_t) );
    if ( !pqueue_item ) {
        return OUT_OF_MEMORY;
    }

    memcpy(&(pqueue_item->request), prequest, sizeof(request_t) );
    enqueue_item(pqueue, pqueue_item);
    return SUCCESS;
}

#pragma endregion

#pragma region dequeue

static void dequeue_handle_pointers(queue_t * const pqueue, queue_item_t * const temp) {
    pqueue  -> first = temp -> prev;
    temp    -> prev  = NULL;

    if ( pqueue -> first ) {
        pqueue -> first -> next = NULL;        
    } else {
        pqueue -> last = NULL; 
    }
}

static inline unsigned char should_wait(const queue_t * const pqueue) {
    return !(pqueue -> flags & QUEUE_STARTED) || ( (pqueue -> first == NULL) && (pqueue -> producer_count > 0));
}

static inline unsigned char is_end(const queue_t * const pqueue) {
    return (pqueue -> first == NULL ) && (pqueue -> producer_count <= 0);
}

static unsigned char wait_for_producer(queue_t * const pqueue) {
    while ( should_wait(pqueue) ) {
        // printf("Consumer waiting....\n");
        pthread_cond_wait( &(pqueue -> consumers_waiting), &(pqueue -> lock) );
    }  
    // printf("Consumer awakened.\n");
    if ( is_end(pqueue) ) {
        printf("Consumer doesn't see any producers. Exiting...\n");
        return 0;
    }
    return 1;
}

static queue_item_t * dequeue_item(queue_t * const pqueue) {
    pthread_mutex_lock( &(pqueue -> lock) );  
    if ( !wait_for_producer(pqueue) ) {
        pthread_mutex_unlock( &(pqueue -> lock) );    
        return NULL;
    }
    queue_item_t * temp = pqueue -> first;
    dequeue_handle_pointers(pqueue, temp);
    pqueue -> size--;
    pthread_mutex_unlock( &(pqueue -> lock) );
    return temp;
}

request_t * dequeue(queue_t * const pqueue) {
    queue_item_t * first = dequeue_item(pqueue);
    if (!first) {
        return NULL;
    }

    request_t * request = (request_t *) malloc( sizeof(request_t) );
    if (!request){
        perror("Unable to allocate memory for request.\n");
        return NULL;
    }

    memcpy(request, &(first->request), sizeof(request_t) );
    free(first);
    first = NULL;
    return request;
}

#pragma endregion

#pragma region admin_options

size_t increment_queue_capacity(queue_t * const pqueue, const size_t increment) {
    pthread_mutex_lock( &(pqueue -> lock) );
    pqueue -> capacity += increment;
    size_t capacity = pqueue -> capacity;  
    pthread_mutex_unlock( &(pqueue -> lock) );  
    return capacity;
}

void stop_queue(queue_t * const pqueue) {
    pthread_mutex_lock( &(pqueue -> lock) );
    pqueue -> flags |= QUEUE_STOPPED;
    pthread_mutex_unlock( &(pqueue -> lock) );      
}

void resume_queue(queue_t * const pqueue) {
    pthread_mutex_lock( &(pqueue -> lock) );
    pqueue -> flags &= ~QUEUE_STOPPED;
    pthread_mutex_unlock( &(pqueue -> lock) );    
}

unsigned char is_queue_stopped(queue_t *const pqueue) {
    pthread_mutex_lock(&(pqueue -> lock));
    unsigned char stopped =  pqueue -> flags & QUEUE_STOPPED; 
    pthread_mutex_unlock(&(pqueue -> lock));        
    return stopped;
}

#pragma endregion

//  FULL QUEUE:
//  NULL <- 0 <-> 1 <-> 2 <-> 3 <-> 4 <-> 5 -> NULL 
//          ^ last                        ^ first

// EMPTY QUEUE:
//  NULL    NULL
//  ^ last  ^first
