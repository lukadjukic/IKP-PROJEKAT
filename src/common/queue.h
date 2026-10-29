#ifndef QUEUE_HEADER
#define QUEUE_HEADER
#include "request.h"

#define QUEUE_CAPACITY_UNLIMITED (0)
#define QUEUE_CAPACITY_DEFAULT   (10)
#define QUEUE_FULL               (-1)
#define OUT_OF_MEMORY            (-2)
#define SUCCESS                  (0)

typedef struct queue_item_t {
    struct queue_item_t * next;
    struct queue_item_t * prev;
    request_t             request;
} queue_item_t;

typedef enum queue_flags_t {
    QUEUE_INITIALIZED = 0x00,
    QUEUE_STARTED     = 0x01,
    QUEUE_STOPPED     = 0x02
} queue_flags_t;

typedef struct queue_t {
    queue_item_t * first;
    queue_item_t * last;
    size_t         capacity;
    size_t         size;
    size_t         producer_count;
    cond_t         consumers_waiting;
    mutex_t        lock;
    queue_flags_t  flags;
} queue_t;

void          init_queue(queue_t * const pqueue, const size_t _capacity);
void          delete_queue(queue_t * const pqueue);
void          register_producer(queue_t * const pqueue);
void          deregister_producer(queue_t * const pqueue);
size_t        increment_queue_capacity(queue_t * pqueue, size_t increment);
int           enqueue(queue_t * const pqueue, const request_t * const prequest); //  WON'T TAKE  ownership of request
request_t *   dequeue(queue_t * const pqueue);                                   //  WILL GIVE   ownership of request (demands freeing)
size_t        increment_queue_capacity(queue_t * const pqueue, const size_t increment);
void          stop_queue(queue_t * const pqueue);
void          resume_queue(queue_t * const pqueue);
unsigned char is_queue_stopped(queue_t * const pqueue);

#endif