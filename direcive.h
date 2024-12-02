#include <stdio.h>
#include <stdlib.h>
#include "request.h"

#define INDEX_OUT_OF_RANGE -1
#define INCREMENT_TOO_BIG  -2
#define CAPACITY_TOO_BIG   -3
#define INVALID_DIRECTIVE  -4
#define MAX_INCREMENT      100

typedef enum COORDINATION_FLAGS {
    COORD_START = 0x01,
    COORD_HALT  = 0x02
} COORDINATION_FLAGS;

typedef struct {
    COORDINATION_FLAGS  flag;
    mutex_t             lock; 
} coordination_t;

typedef enum DIRECTIVE {
    DIR_HALT                     = 0x01,
    DIR_THEAD_STOP               = 0x02,
    DIR_THREAD_CONTINUE          = 0x04,
    DIR_THREAD_INCREASE_CAPACITY = 0x08
} DIRECTIVE;

typedef struct {
    size_t id;
    size_t capacity; 
} parameters_t;

typedef struct directive_t {
    DIRECTIVE    dir;
    parameters_t params;    
} directive_t;

void      coordinate(coordination_t * coordination, COORDINATION_FLAGS new_state);
void      print_directive(directive_t * directive);
ssize_t   handle_directive(QUEUE * queues, size_t workers_count, coordination_t * coordinate, directive_t * directive);
ssize_t   handle_halt(coordination_t * coordination);
ssize_t   handle_thread_stop(QUEUE * queues, size_t workers_count, size_t id);
ssize_t   handle_thread_continue(QUEUE * queues, size_t workers_count, size_t id);
ssize_t   handle_thread_increase_capacity(QUEUE * queues, size_t workers_count, parameters_t * parameters);