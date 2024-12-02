#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "direcive.h"

static inline unsigned char index_out_of_range(size_t index, size_t workers_count) {
    return ( (index < 0) || (index > workers_count -1) );
}

static inline unsigned char increment_too_big(size_t increment, size_t max_increment) {
    return (increment > max_increment);
}

static inline unsigned char capacity_too_big(size_t capacity, size_t increment) {
    return (capacity + increment < capacity); 
}

void coordinate(coordination_t * coordination, COORDINATION_FLAGS new_state) {
    pthread_mutex_lock( &(coordination -> lock) );
    coordination -> flag = new_state;
    pthread_mutex_unlock( &(coordination -> lock) );
}

void print_directive(directive_t * directive) {
    printf("Directive:\n");
    printf("\tDirective: ");
    switch ( directive -> dir) {
        case DIR_HALT:
            printf("HALT\n");
            break;
        case DIR_THEAD_STOP:
            printf("THREAD STOP\n");
            break;
        case DIR_THREAD_CONTINUE:
            printf("THREAD CONTINUE\n");
            break;
        case DIR_THREAD_INCREASE_CAPACITY:
            printf("THREAD INCREASE CAPACITY\n");
            break;
        default:
            printf("Directive %u not recognized.\n", directive ->dir);
            break;
    }
    printf("\tParameters: id = %lu, capacity = %lu\n", directive -> params.id, directive -> params.capacity);
}

ssize_t handle_directive(QUEUE * queues, size_t workers_count, coordination_t * coordinate, directive_t * directive) {
    ssize_t result;

    switch (directive -> dir) {
        case DIR_HALT:
            result = handle_halt(coordinate);    
            break;
        case DIR_THEAD_STOP:
            result = handle_thread_stop(queues, workers_count, directive -> params.id);
            break;
        case DIR_THREAD_CONTINUE:
            result = handle_thread_continue(queues, workers_count, directive -> params.id);
            break;
        case DIR_THREAD_INCREASE_CAPACITY:
            result = handle_thread_increase_capacity(queues, workers_count, &(directive -> params) );
            break;    
        default:
            result = INVALID_DIRECTIVE;
            break;
    }

    return result;
}

ssize_t handle_halt(coordination_t * coordination){
    coordinate(coordination, COORD_HALT);
    return 1;
}

ssize_t handle_thread_stop(QUEUE * queues, size_t workers_count, size_t id) {
    QUEUE * queue  = queues + id;
    if ( index_out_of_range(id, workers_count) ) {
        return INDEX_OUT_OF_RANGE;
    }
    stop_queue(queue);
    return 1;
}

ssize_t handle_thread_continue(QUEUE * queues, size_t workers_count, size_t id) {
    QUEUE * queue  = queues + id;
    if ( index_out_of_range(id, workers_count) ) {
        return INDEX_OUT_OF_RANGE;
    }
    resume_queue(queue);
    return 1;
}

ssize_t handle_thread_increase_capacity(QUEUE * queues, size_t workers_count, parameters_t * parameters) {
    size_t  increment = parameters -> capacity;
    size_t  index     = parameters -> id;
    QUEUE * queue     = queues + index;
    
    if ( index_out_of_range(index, workers_count) ) {
        return INDEX_OUT_OF_RANGE;
    }
    if ( increment_too_big(increment, MAX_INCREMENT) ) {
        return INCREMENT_TOO_BIG;
    }
    if ( capacity_too_big(queue -> capacity, increment) ) {
        return CAPACITY_TOO_BIG;
    }

    return increment_queue_capacity(queue, increment);
}



