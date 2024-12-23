#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "direcive.h"

#define SERVER_PORT 11001
#define SERVER_IP   "127.0.0.1"
#define BUF_SIZE    1024

int           connect_to_server();
void          communicate_with_server(const int server_socket);
void          set_server_address(sockaddr_in_t * address);
void          ask_user_for_directive(directive_t * const directive, unsigned char * const exit);
void          print_result(const ssize_t result);
void          print_menu();
unsigned char should_end(const directive_t * const directive, const int result);
void          fill_directive(directive_t * const directive, int * const error);
void          get_params(directive_t * const directive); 
void          get_directive_code(int * result) ;

int main() {
    int server_socket;

    server_socket = connect_to_server();
    if ( server_socket < 0)
        return -1;

    communicate_with_server(server_socket);

    shutdown(server_socket, SHUT_RDWR);
    close(server_socket);

    return 0;
}

#pragma region functions

int connect_to_server() {
    int admin_socket;
    sockaddr_in_t server_address;

    if ((admin_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed.\n");
        return admin_socket;
    }

    set_server_address(&server_address);

    if (connect(admin_socket, (struct sockaddr *)&server_address, sizeof(struct sockaddr)) < 0 ) {
        perror("Connection failed.\n");
        return -2;
    }

    return admin_socket;
}

void set_server_address(sockaddr_in_t * address) {
    address -> sin_family       = AF_INET;
    address -> sin_port         = htons(SERVER_PORT);
    address -> sin_addr.s_addr  = inet_addr(SERVER_IP);
}

void communicate_with_server(const int server_socket) {
    ssize_t         result;
    directive_t     directive;
    unsigned char   exit = 0;
    
    while (1) {
        ask_user_for_directive(&directive, &exit);
        if ( exit )
            break;
        
        print_directive(&directive);
        send(server_socket, (void *)&directive, sizeof(directive_t), 0);
        recv(server_socket, (void *)&result, sizeof(ssize_t), 0);
        print_result(result);

        if (should_end(&directive, result)) {
            break;
        }
    }
}

unsigned char should_end(const directive_t * const directive, const int result) {
    return (directive -> dir == DIR_HALT) && (result == DIRECTIVE_SUCCESS);
}   

void print_result(const ssize_t result) {
    switch (result){
        case DIRECTIVE_SUCCESS:
            printf("[SUCCESS] Directive success.\n");
            break;
        case INDEX_OUT_OF_RANGE:
            printf("[FAILURE] Index out of range.\n");
            break;
        case INCREMENT_TOO_BIG:
            printf("[FAILURE] Increment too big.\n");
            break;
        case CAPACITY_TOO_BIG:
            printf("[FAILURE] Capacity could not be incremented by said ammount (too big).\n");
            break;
        case INVALID_DIRECTIVE:
            printf("[FAILURE] Invalid directive.\n");
            break;
        default:
            printf("[???] Result could not be interpreted.\n");
            break;
    }
}

void ask_user_for_directive(directive_t * const directive, unsigned char * const exit) {
    int error;
    do {
        int answer;
        print_menu();
        scanf("%d",&answer);
        if ( !answer ) {
            * exit = 1;
            return;
        }

        * exit = 0;
        fill_directive(directive, &error);
    } while (error);
}

void print_menu() {
    printf("============= MENU =============\n");
    printf("0) Exit\n");
    printf("1) Execute directive\n");
    printf("================================\n");
    printf(">>> ");
}

void fill_directive(directive_t * const directive, int * const error) {
    int result;
    get_directive_code(&result);
    if (result < 0 || result > 3){
        printf("Error. Invalid directive.\n");
        *error = 1;
        return;
    }

    directive -> dir = (directive_code_t)(0x01 << result);
    *error = 0;

    if (directive -> dir == DIR_HALT) {
        directive -> params.capacity = 0;
        directive -> params.id = 0;
        return;
    }
    get_params(directive);
}

void get_directive_code(int * result) {
    printf("Directive options: \n");
    printf("0) Halt\n");
    printf("1) Stop worker\n");
    printf("2) Resume worker\n");
    printf("3) Increse worker max. load\n");
    printf(">>> ");
    scanf("%d", result);
}

void get_params(directive_t * const directive) {
    printf("Enter worker Id: ");
    scanf("%lu", &(directive ->params.id) );

    if (directive -> dir != DIR_THREAD_INCREASE_CAPACITY) {
        directive -> params.capacity = 0;
        return;
    }

    printf("Enter load increase: ");
    scanf("%lu", &(directive -> params.capacity) );
}

#pragma endregion