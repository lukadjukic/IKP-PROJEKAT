#ifndef DIRECTIVE_HEADER
#define DIRECTIVE_HEADER

#define DIRECTIVE_SUCCESS   1
#define INDEX_OUT_OF_RANGE -1
#define INCREMENT_TOO_BIG  -2
#define CAPACITY_TOO_BIG   -3
#define INVALID_DIRECTIVE  -4
#define MAX_INCREMENT      100


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

void print_directive(const directive_t * const directive);

#endif