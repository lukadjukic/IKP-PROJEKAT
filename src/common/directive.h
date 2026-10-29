#ifndef DIRECTIVE_HEADER
#define DIRECTIVE_HEADER

#include "queue.h"

#define DIRECTIVE_SUCCESS   1
#define INDEX_OUT_OF_RANGE -1
#define INCREMENT_TOO_BIG  -2
#define CAPACITY_TOO_BIG   -3
#define INVALID_DIRECTIVE  -4
#define MAX_INCREMENT      100

typedef enum coordination_flags_t {
    COORD_START = 0x01,
    COORD_HALT  = 0x02
} coordination_flags_t;

typedef struct coordination_t {
    coordination_flags_t  state;
    mutex_t               lock; 
} coordination_t;

typedef enum directive_code_t {
    DIR_HALT                     = 0x01,
    DIR_THEAD_STOP               = 0x02,
    DIR_THREAD_CONTINUE          = 0x04,
    DIR_THREAD_INCREASE_CAPACITY = 0x08
} directive_code_t;

typedef struct {
    size_t id;
    size_t capacity; 
} parameters_t;

typedef struct directive_t {
    parameters_t        params;    
    directive_code_t    dir; 
} directive_t;

void      coordinate(coordination_t * const coordination, const coordination_flags_t new_state);
void      print_directive(const directive_t * const directive);
ssize_t   handle_directive(queue_t * const queues, const size_t workers_count, coordination_t * const coordinate, const directive_t * const directive);
ssize_t   handle_halt(coordination_t * const coordination);
ssize_t   handle_thread_stop(queue_t * const queues, const size_t workers_count, const size_t id);
ssize_t   handle_thread_continue(queue_t * const queues, const size_t workers_count, const size_t id);
ssize_t   handle_thread_increase_capacity(queue_t * const queues, const size_t workers_count, const parameters_t * const parameters);

#endif