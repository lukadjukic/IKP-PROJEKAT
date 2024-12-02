#ifndef REQUEST_HEADER
#define REQUEST_HEADER

#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>

#define MAX_ARGUMENTS 10

typedef pthread_mutex_t     mutex_t;
typedef pthread_cond_t      cond_t;
typedef struct sockaddr_in  sockaddr_in_t;
typedef struct sockaddr     sockaddr_t;

typedef struct request_t {
    size_t              id;
    unsigned char       flags;
    unsigned char       operation;
    ssize_t             arguments[MAX_ARGUMENTS];
    sockaddr_in_t       client_addr;
} request_t;

void print_request(const request_t * const prequest);

#endif
