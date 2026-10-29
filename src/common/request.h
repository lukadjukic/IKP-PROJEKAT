#ifndef REQUEST_HEADER
#define REQUEST_HEADER

#include <netinet/in.h>
#include <pthread.h>

#define MAX_ARGUMENTS 10

typedef pthread_mutex_t    mutex_t;
typedef pthread_cond_t     cond_t;
typedef struct sockaddr_in sockaddr_in_t;
typedef struct sockaddr    sockaddr_t;

typedef struct {
    unsigned char stopped : 1;
    unsigned char invalid : 1;
    unsigned char handled : 1;
    unsigned char padding : 5;
} request_flags_t;

typedef struct request_t {
    size_t          id;
    request_flags_t flags;
    unsigned char   operation;
    ssize_t         arguments[MAX_ARGUMENTS];
    ssize_t         result;
    sockaddr_in_t   client_addr;
} request_t;

void print_request(const request_t * const prequest);

#endif
