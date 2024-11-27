#include "request.h"

inline void init_queue(QUEUE* pqueue) {
    pqueue -> first = NULL; 
    pqueue -> last = NULL;
    return;
}

void delete_queue(QUEUE* pqueue) {
    while ( pqueue -> first ){
        REQUEST* prequest = dequeue(pqueue);
        if ( prequest ) {
            free(prequest);
        }
    }
}
int enqueue(QUEUE* pqueue, REQUEST* prequest) {
    QUEUE_ITEM* pqueue_item = (QUEUE_ITEM*) malloc( sizeof(QUEUE_ITEM) );
    if ( !pqueue_item ) {
        return 0;
    }
    memcpy(&(pqueue_item->request), prequest, sizeof(REQUEST) );
    free(prequest);
    _enqueue(pqueue, pqueue_item);
}

REQUEST* dequeue(QUEUE* pqueue) {
    REQUEST* request = (REQUEST*)malloc( sizeof(REQUEST) );
    if (!request){
        return NULL;
    } 
    QUEUE_ITEM* first = _dequeue(pqueue);
    memcpy(request, &(first->request), sizeof(REQUEST) );
    free(first);
    return request;
}

void _enqueue(QUEUE* pqueue, QUEUE_ITEM* pitem) {
    pitem   -> prev = NULL;
    pitem   -> next = pqueue -> last;
    pqueue  -> last = pitem;
    if ( pitem -> next ) {
        pitem   -> next -> prev = pitem; 
        return;
    }
    pqueue -> first = pitem;
    return;
}

QUEUE_ITEM* _dequeue(QUEUE* pqueue) {
    QUEUE_ITEM* temp = pqueue -> first;
    pqueue  -> first = temp -> prev;
    temp    -> prev  = NULL;
    if ( pqueue -> first ) {
        pqueue -> first -> next = NULL;        
    } else {
        pqueue -> last = NULL; 
    }
    return temp;
}

//  FULL QUEUE:
//  NULL <- 0 <-> 1 <-> 2 <-> 3 <-> 4 <-> 5 -> NULL 
//          ^ last                        ^ first


// EMPTY QUEUE:
//  NULL    NULL
//  ^ last  ^first
