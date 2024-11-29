#ifndef REQUEST_HEADER
#define REQUEST_HEADER

#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>

#define MAX_ARGUMENTS             10
#define QUEUE_CAPACITY_UNLIMITED  0

typedef pthread_mutex_t mutex_t;
typedef pthread_cond_t  cond_t;

typedef struct request_t {
    size_t              id;
    unsigned char       flags;
    unsigned char       operation;
    signed   long       arguments[MAX_ARGUMENTS];
    struct sockaddr_in  client_addr;
} REQUEST;

typedef struct queue_item_t {
    REQUEST              request;
    struct queue_item_t* next;
    struct queue_item_t* prev;    
} QUEUE_ITEM;

typedef struct queue_t {
    struct queue_item_t* first;
    struct queue_item_t* last;
    size_t               capacity;
    size_t               size;
    size_t               producer_count;
    cond_t               consumers_waiting;
    mutex_t              lock;
} QUEUE;

void        print_request(REQUEST* prequest);
void        init_queue(QUEUE* pqueue, size_t _capacity);
void        delete_queue(QUEUE* pqueue);
void        register_producer(QUEUE* pqueue);
void        deregister_producer(QUEUE* pqueue);
int         enqueue(QUEUE* pqueue,  REQUEST* prequest);     //  WON'T TAKE  ownership of request (and free it) 
REQUEST*    dequeue(QUEUE* pqueue);                         //  WILL GIVE   ownership of request (demands freeing)
void        _enqueue(QUEUE* pqueue, QUEUE_ITEM* pitem);
QUEUE_ITEM* _dequeue(QUEUE* pqueue);

#endif

//  FULL QUEUE:
//  NULL <- 0 <-> 1 <-> 2 <-> 3 <-> 4 <-> 5 -> NULL 
//          ^ last                        ^ first


// EMPTY QUEUE:
//  NULL    NULL
//  ^ last  ^first
