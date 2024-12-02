#include <stdio.h>
#include "request.h"

void print_request(REQUEST* prequest) {
    printf("Request:\n");
    printf("\t\tId: %lu\n", prequest -> id);
    printf("\t\tClient: %s\n", inet_ntoa(prequest -> client_addr.sin_addr));
    printf("\t\tFlags: %u\n", prequest -> flags);
    printf("\t\tOperation code: %u\n", prequest -> operation);
    printf("\t\tArguments: [");
    for(int i = 0; i < MAX_ARGUMENTS; ++i) {
        printf("%ld", prequest -> arguments[i]);
        if(i != MAX_ARGUMENTS -1){
            printf(", ");
        }
    }
    printf("]\n");
}

void init_queue(QUEUE* pqueue, size_t _capacity) {
    pqueue -> first          = NULL; 
    pqueue -> last           = NULL;
    pqueue -> size           = 0;
    pqueue -> producer_count = 0;
    pqueue -> capacity       = _capacity;
    pqueue -> flags          = QUEUE_INITIALIZED;
    pthread_mutex_init( &(pqueue -> lock), NULL);
    pthread_cond_init( &(pqueue -> consumers_waiting), NULL);
    return;
}

void delete_queue(QUEUE* pqueue) {
    while ( pqueue -> first ){
        REQUEST* prequest = dequeue(pqueue);
        if ( prequest ) {
            free(prequest);
        }
    }
    pthread_mutex_destroy( &(pqueue -> lock) );
    pthread_cond_destroy( &(pqueue -> consumers_waiting) );
    pqueue -> first = NULL;
    pqueue -> last  = NULL;
}

void register_producer(QUEUE* pqueue) {
    pthread_mutex_lock( &(pqueue -> lock) );
    (pqueue -> flags) |= QUEUE_STARTED;
    ++(pqueue -> producer_count);
    pthread_mutex_unlock( &(pqueue -> lock) );
}

void deregister_producer(QUEUE* pqueue) {
    pthread_mutex_lock( &(pqueue -> lock) );
    if ( --(pqueue -> producer_count) <= 0) {
        pthread_cond_broadcast( &(pqueue -> consumers_waiting) );
    }
    pthread_mutex_unlock( &(pqueue -> lock) );
}

static void _enqueue(QUEUE* pqueue, QUEUE_ITEM* pitem) {
    pthread_mutex_lock( &(pqueue -> lock) );
    pitem   -> prev = NULL;
    pitem   -> next = pqueue -> last;
    pqueue  -> last = pitem;
    if ( pitem -> next ) {
        pitem   -> next -> prev = pitem; 
        return;
    }
    pqueue -> first = pitem;

    ++(pqueue -> size);
    pthread_mutex_unlock( &(pqueue -> lock) );
    pthread_cond_signal( &(pqueue -> consumers_waiting) );
    return;
}

static QUEUE_ITEM* _dequeue(QUEUE* pqueue) {
    pthread_mutex_lock( &(pqueue -> lock) );  
    while ( !(pqueue -> flags & QUEUE_STARTED) || ( (pqueue -> first == NULL) && (pqueue -> producer_count > 0) ) ) {
        // printf("Consumer waiting....\n");
        pthread_cond_wait( &(pqueue -> consumers_waiting), &(pqueue -> lock) );
    }  
    // printf("Consumer awakened.\n");
    if ( (pqueue -> first == NULL ) && (pqueue -> producer_count <= 0) ) {
        //printf("Consumer doesn't see any producers. Exiting...\n");
        return NULL;
    }
    QUEUE_ITEM* temp = pqueue -> first;
    pqueue  -> first = temp -> prev;
    temp    -> prev  = NULL;
    if ( pqueue -> first ) {
        pqueue -> first -> next = NULL;        
    } else {
        pqueue -> last = NULL; 
    }
    --(pqueue -> size);
    pthread_mutex_unlock( &(pqueue -> lock) );
    return temp;
}

int enqueue(QUEUE* pqueue, REQUEST* prequest) {
    //  If capacity <= 0 <==> queue capacity is limitless
    if ( (pqueue -> capacity > 0) && (pqueue -> size >= pqueue -> capacity) ) {
        return -1;   
    }

    QUEUE_ITEM* pqueue_item = (QUEUE_ITEM*) malloc( sizeof(QUEUE_ITEM) );
    if ( !pqueue_item ) {
        return -2;
    }
    memcpy(&(pqueue_item->request), prequest, sizeof(REQUEST) );
    _enqueue(pqueue, pqueue_item);
    return 0;
}

REQUEST* dequeue(QUEUE* pqueue) {
    QUEUE_ITEM* first = _dequeue(pqueue);
    if (!first) {
        return NULL;
    }
    REQUEST* request = (REQUEST*) malloc( sizeof(REQUEST) );
    if (!request){
        perror("Unable to allocate memory for request.\n");
        return NULL;
    } 
    memcpy(request, &(first->request), sizeof(REQUEST) );
    free(first);
    return request;
}

size_t increment_queue_capacity(QUEUE * pqueue, size_t increment) {
    pthread_mutex_lock( &(pqueue -> lock) );
    pqueue -> capacity += increment;
    size_t capacity = pqueue -> capacity;  
    pthread_mutex_unlock( &(pqueue -> lock) );  
    return capacity;
}

void stop_queue(QUEUE * pqueue) {
    pthread_mutex_lock( &(pqueue -> lock) );
    pqueue -> flags |= QUEUE_STOPPED;
    pthread_mutex_unlock( &(pqueue -> lock) );      
}

void resume_queue(QUEUE * pqueue) {
    pthread_mutex_lock( &(pqueue -> lock) );
    pqueue -> flags &= ~QUEUE_STOPPED;
    pthread_mutex_unlock( &(pqueue -> lock) );    
}

unsigned char is_queue_stopped(QUEUE * pqueue) {
    pthread_mutex_lock( &(pqueue -> lock) );
    unsigned char stopped =  pqueue -> flags & QUEUE_STOPPED; 
    pthread_mutex_unlock( &(pqueue -> lock) );    
    return stopped;
}

//  FULL QUEUE:
//  NULL <- 0 <-> 1 <-> 2 <-> 3 <-> 4 <-> 5 -> NULL 
//          ^ last                        ^ first


// EMPTY QUEUE:
//  NULL    NULL
//  ^ last  ^first
