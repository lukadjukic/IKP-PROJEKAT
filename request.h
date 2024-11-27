#ifndef REQUEST_HEADER
#define REQUEST_HEADER

#include <stdlib.h>
#include <string.h>

#define MAX_ARGUMENTS 10

typedef struct request_t {
    size_t          id;
    unsigned char   flags;
    unsigned char   operation;
    signed   long   arguments[MAX_ARGUMENTS];
} REQUEST;

typedef struct queue_item_t {
    REQUEST              request;
    struct queue_item_t* next;
    struct queue_item_t* prev;    
} QUEUE_ITEM;

typedef struct queue_t {
    struct queue_item_t* first;
    struct queue_item_t* last;
} QUEUE;

void        init_queue(QUEUE* pqueue);
void        delete_queue(QUEUE* pqueue);
int         enqueue(QUEUE* pqueue,  REQUEST* prequest);     //  Will TAKE ownership of request (and free it) 
REQUEST*    dequeue(QUEUE* pqueue);                         //  Will GIVE ownership of request (demands freeing)
void        _enqueue(QUEUE* pqueue, QUEUE_ITEM* pitem);
QUEUE_ITEM* _dequeue(QUEUE* pqueue);

#endif

//  FULL QUEUE:
//  NULL <- 0 <-> 1 <-> 2 <-> 3 <-> 4 <-> 5 -> NULL 
//          ^ last                        ^ first


// EMPTY QUEUE:
//  NULL    NULL
//  ^ last  ^first
