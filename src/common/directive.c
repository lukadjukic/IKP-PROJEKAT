#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "directive.h"

static inline unsigned char index_out_of_range(const size_t index, const size_t workers_count) {
    return (index > workers_count -1);
}

static inline unsigned char increment_too_big(const size_t increment, const size_t max_increment) {
    return (increment > max_increment);
}

static inline unsigned char capacity_too_big(const size_t capacity, const size_t increment) {
    return (capacity + increment < capacity); 
}

void coordinate(coordination_t * const coordination, const coordination_flags_t new_state) {
    pthread_mutex_lock( &(coordination -> lock) );
    coordination -> state = new_state;
    pthread_mutex_unlock( &(coordination -> lock) );
}

void print_directive(const directive_t * const directive) {
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

ssize_t handle_directive(QUEUE * const queues, const size_t workers_count, coordination_t * const coordinate, const directive_t * const directive) {
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

ssize_t handle_halt(coordination_t * const coordination){
    coordinate(coordination, COORD_HALT);
    return DIRECTIVE_SUCCESS;
}

ssize_t handle_thread_stop(queue_t * const queues, const size_t workers_count, const size_t id) {
    queue_t * const queue  = queues + id;
    if ( index_out_of_range(id, workers_count) ) {
        return INDEX_OUT_OF_RANGE;
    }
    stop_queue(queue);
    return DIRECTIVE_SUCCESS;
}

ssize_t handle_thread_continue(queue_t * const queues, const size_t workers_count, const size_t id) {
    queue_t * const queue  = queues + id;
    if ( index_out_of_range(id, workers_count) ) {
        return INDEX_OUT_OF_RANGE;
    }
    resume_queue(queue);
    return DIRECTIVE_SUCCESS;
}

ssize_t handle_thread_increase_capacity(queue_t * const queues, const size_t workers_count, const parameters_t * const parameters) {
    const size_t    increment = parameters -> capacity;
    const size_t    index     = parameters -> id;
    queue_t * const queue     = queues + index;
    
    if ( index_out_of_range(index, workers_count) ) {
        return INDEX_OUT_OF_RANGE;
    }
    if ( increment_too_big(increment, MAX_INCREMENT) ) {
        return INCREMENT_TOO_BIG;
    }
    if ( capacity_too_big(queue -> capacity, increment) ) {
        return CAPACITY_TOO_BIG;
    }

    increment_queue_capacity(queue, increment);
    return DIRECTIVE_SUCCESS;
}

