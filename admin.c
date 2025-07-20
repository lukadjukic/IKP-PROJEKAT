#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "direcive.h"
#include "request.h"

#define SERVER_PORT 11001
#define TEST_PORT   11000
#define SERVER_IP   "127.0.0.1"
#define BUF_SIZE    1024

typedef struct test_t {
    request_t request;
    ssize_t   expected_answer;
} test_t;

int           connect_to_server();
void          communicate_with_server(const int server_socket);
void          set_server_address(sockaddr_in_t * address,const unsigned short server_port, const char * server_ip);
int           ask_user_for_directive(directive_t * const directive, unsigned char * const exit);
void          print_result(const ssize_t result);
void          print_menu();
unsigned char should_end(const directive_t * const directive, const int result);
int           fill_directive(directive_t * const directive, int * const error);
void          get_params(directive_t * const directive); 
void          get_directive_code(int * result) ;
void          test_primes(void);

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

    set_server_address(&server_address, SERVER_PORT, SERVER_IP);

    if (connect(admin_socket, (struct sockaddr *)&server_address, sizeof(struct sockaddr)) < 0 ) {
        perror("Connection failed.\n");
        return -2;
    }

    return admin_socket;
}

void set_server_address(sockaddr_in_t * address,const unsigned short server_port, const char * server_ip) {
    address -> sin_family       = AF_INET;
    address -> sin_port         = htons(server_port);
    address -> sin_addr.s_addr  = inet_addr(server_ip);
}

void communicate_with_server(const int server_socket) {
    ssize_t         result;
    directive_t     directive;
    unsigned char   exit = 0;
    
    while (1) {
        if (ask_user_for_directive(&directive, &exit) == 4)
            continue;
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

int ask_user_for_directive(directive_t * const directive, unsigned char * const exit) {
    int error, result;
    do {
        int answer;
        print_menu();
        scanf("%d",&answer);
        if ( !answer ) {
            * exit = 1;
            return -1;
        }

        * exit = 0;
        result = fill_directive(directive, &error);
    } while (error);
    return result;
}

void print_menu() {
    printf("============= MENU =============\n");
    printf("0) Exit\n");
    printf("1) Execute directive\n");
    printf("================================\n");
    printf(">>> ");
}

int fill_directive(directive_t * const directive, int * const error) {
    int result;
    get_directive_code(&result);
    if (result < 0 || result > 4){
        printf("Error. Invalid directive.\n");
        *error = 1;
        return -1;
    }

    if (result == 4){
        test_primes();
        *error = 0;
        return 4;
    }
        
    directive -> dir = (directive_code_t)(0x01 << result);
    *error = 0;

    if (directive -> dir == DIR_HALT) {
        directive -> params.capacity = 0;
        directive -> params.id = 0;
        return directive -> dir;
    }
    get_params(directive);
    return directive -> dir;
}

void get_directive_code(int * result) {
    printf("Directive options: \n");
    printf("0) Halt\n");
    printf("1) Stop worker\n");
    printf("2) Resume worker\n");
    printf("3) Increse worker max. load\n");
    printf("4) Test primes\n");
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

FILE* safe_fopen(const char * const filename, const char * const modes) {
    FILE* file = fopen(filename, modes);
    if ( !file ) {
        perror("Could not open the specified file with the specified modes.\n");
        exit(EXIT_FAILURE);
    }
    return file;
}

void safe_fclose(FILE * file) {
    if (file) {
        fclose(file);
        file = NULL;
    }
}

int create_socket() {
    int client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    
    if (client_socket < 0)  {
        perror("Socket creation failed");
    }

    return client_socket;
}

int  send_request_recieve_id(request_t * const request, const sockaddr_in_t * server_address, const int client_socket) {
    socklen_t  address_length = sizeof(sockaddr_t);    
    
    if (sendto(client_socket, request, sizeof(request_t), 0, (sockaddr_t *)server_address, sizeof(sockaddr_t)) < 0) {
        perror("Send failed");
        return -1;
    }

    if (recvfrom(client_socket, &(request -> id), sizeof(size_t), 0, (sockaddr_t *) server_address, &address_length) < 0) {
        perror("Recieving id failed.\n");
        return -1;
    }
    
    return 1;
}

void recieve_response(request_t * const request, const sockaddr_in_t * const server_address, const int client_socket) {
    socklen_t  address_length;    
    
    ssize_t recv_len = recvfrom(client_socket, request, sizeof(request_t), 0, (sockaddr_t *) server_address, &address_length);
    if (recv_len < 0) {
        perror("Receive failed.\n");
    }
}

void print_test(const test_t * const test){
    unsigned char passed = test->request.result == test -> expected_answer;
    
    if (!passed)
        printf("\033[0;31m================================\033[0m\n");
    else
        printf("\033[0;32m================================\033[0m\n");

    print_request(&(test -> request));
    printf("Expected answer: %ld\n", test ->expected_answer);
    if (!passed){
        printf("\033[0;31mTest FAILED\033[0m\n");
        printf("\033[0;31m================================\033[0m\n");
    } else {
        printf("\033[0;32mTest PASSED\033[0m\n");
        printf("\033[0;32m================================\033[0m\n");
    }
}

unsigned char fill_test(test_t* const test, FILE * file){
    memset((void *)test, 0, sizeof(test));
    
    if ( fscanf(file, "%hhu ", &(test ->request.operation)) != 1  ){
        return 0;
    }

    for (int i = 0; i < MAX_ARGUMENTS; ++i) {
        if ( fscanf(file,"%ld ", &(test -> request.arguments[i])) != 1 )
            return 0;
        }
    if (fscanf(file, "%ld", &(test ->expected_answer)) != 1){
        return 0;
    }

    fscanf(file, "\n");        
    return 1;
}

void print_summary(const int count, const int passed)
{
    if (count == passed){
        printf("\033[0;32mSUMMARY: \n\tTests ran: %d\n\tTests passed %d\n\tTests failed %d\n\tPercentage: %.2lf%%\033[0m\n", count, passed, count - passed, ((double)passed)/count*100);
    } else {
        printf("\033[0;31mSUMMARY: \n\tTests ran: %d\n\tTests passed %d\n\tTests failed %d\n\tPercentage: %.2lf%%\033[0m\n", count, passed, count - passed, ((double)passed)/count*100);
    }

}

void test_primes(){
    int tests_count = 0, tests_passed = 0;
    int           sock;
    sockaddr_in_t server_address;
    test_t        test;
    FILE *        in_file; 
    
    const char * const in_filename = "tests.txt";
    
    in_file  = safe_fopen(in_filename, "r+");
    
    sock = create_socket();

    if (sock < 0) {
        safe_fclose(in_file);
        return;
    }

    set_server_address(&server_address, TEST_PORT, SERVER_IP);

    while (fill_test(&test, in_file)){        
        if (send_request_recieve_id(&(test.request), &server_address, sock)  < 0) {
            break;            
        }
        recieve_response(&(test.request), &server_address, sock);
        ++tests_count;
        print_test(&test);
        if (test.expected_answer == test.request.result)
            ++tests_passed;
    }

    safe_fclose(in_file);    
    close(sock);
    print_summary(tests_count, tests_passed);
}

#pragma endregion