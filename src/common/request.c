#include <stdio.h>
#include <arpa/inet.h>
#include "request.h"

static void print_array(const ssize_t * array, const size_t array_length)
{
    printf("\tArguments:\t[");
    for(size_t i = 0; i < array_length; ++i) {
        printf("%ld", array[i]);
        if(i != array_length -1){
            printf(", ");
        }
    }
    printf("]\n");
}

void print_request(const request_t * const  prequest)
{
    printf("Request:\n");
    printf("\tId:\t\t%lu\n", prequest -> id);
    printf("\tClient:\t\t%s\n", inet_ntoa(prequest -> client_addr.sin_addr));
    printf("\tFlags:\t\tS=%u I=%u H=%u \n", prequest -> flags.stopped, prequest -> flags.invalid, prequest -> flags.handled);
    printf("\tOperation code:\t%u\n", prequest -> operation);
    print_array(prequest -> arguments, MAX_ARGUMENTS);
    printf("\tResult:\t\t%ld\n", prequest -> result);
}
