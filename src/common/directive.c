#include <stdio.h>
#include "directive.h"

void print_directive(const directive_t * const directive) {
    printf("Directive:\n");
    printf("\tDirective: ");

    switch (directive->dir) {
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
        printf("Directive %u not recognized.\n", directive->dir);
        break;
    }

    printf("\tParameters: id = %lu, capacity = %lu\n", directive->id, directive->capacity);
}
