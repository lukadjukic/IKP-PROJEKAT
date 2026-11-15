#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdbool.h>
#include "./common/directive.h"
#include "./common/request.h"

#define SERVER_PORT   11001
#define TEST_PORT     11000
#define SERVER_IP     "127.0.0.1"
#define BUF_SIZE      1024
#define TEST_FILENAME "./tests/integration/tests.txt"

typedef struct test_t {
    request_t request;
    ssize_t   expected_answer;
} test_t;

int  connect_to_server();
void communicate_with_server(const int server_socket);
void set_server_address(sockaddr_in_t * address, const sa_family_t family, const unsigned short server_port, const char * server_ip);
void test_primes(const char * const in_filename);

int main() {
    int server_socket = connect_to_server();

    if (server_socket < 0)
        return -1;

    communicate_with_server(server_socket);

    shutdown(server_socket, SHUT_RDWR);
    close(server_socket);

    return 0;
}

#pragma region functions

int connect_to_server() {
    int           admin_socket;
    sockaddr_in_t server_address;

    admin_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (admin_socket < 0) {
        perror("Socket creation failed.\n");
        return admin_socket;
    }

    set_server_address(&server_address, AF_INET, SERVER_PORT, SERVER_IP);

    if (connect(admin_socket, (struct sockaddr *)&server_address, sizeof(struct sockaddr)) < 0) {
        perror("Connection failed.\n");
        return -2;
    }

    return admin_socket;
}

void set_server_address(sockaddr_in_t * address, const sa_family_t family, const unsigned short server_port, const char * server_ip) {
    address->sin_family      = family;
    address->sin_port        = htons(server_port);
    address->sin_addr.s_addr = inet_addr(server_ip);
}

int get_option(void) {
    int option = -1;

    while (option < 0 || option > 5) {
        printf("============= MENU =============\n");
        printf("0) Exit\n");
        printf("1) Halt\n");
        printf("2) Stop worker\n");
        printf("3) Resume worker\n");
        printf("4) Increse worker max. load\n");
        printf("5) Test primes\n");
        printf("================================\n");
        printf(">>> ");
        scanf("%d", &option);
    }

    return option;
}

size_t get_size_t(const char * const param_name) {
    size_t value = 0;
    printf("Enter %s:\n", param_name);
    printf(">>>");
    scanf("%zu", &value);
    return value;
}

void fill_params(directive_t * directive) {
    directive->capacity = 0;
    directive->id       = 0;

    switch (directive->dir) {
    case DIR_THEAD_STOP:
        directive->id = get_size_t("id");
        break;
    case DIR_HALT:
        break;
    case DIR_THREAD_CONTINUE:
        directive->id = get_size_t("id");
        break;
    case DIR_THREAD_INCREASE_CAPACITY:
        directive->id       = get_size_t("id");
        directive->capacity = get_size_t("increase amount");
        break;
    default:
        break;
    }
}

void print_result(const ssize_t result) {
    switch (result) {
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

inline bool should_end(const directive_t * const directive, const ssize_t result) {
    return (directive->dir == DIR_HALT) && (result == DIRECTIVE_SUCCESS);
}

void communicate_with_server(const int server_socket) {
    directive_t directive;
    ssize_t     result;

    while (1) {
        int option = get_option();
        if (option == 0) {
            break;
        } else if (option == 5) {
            test_primes(TEST_FILENAME);
            continue;
        }

        directive.dir = (dcode_t)(0x1 << (option - 1));
        fill_params(&directive);

        print_directive(&directive);
        send(server_socket, (void *)&directive, sizeof(directive_t), 0);
        recv(server_socket, (void *)&result, sizeof(ssize_t), 0);
        print_result(result);

        if (should_end(&directive, result)) {
            break;
        }
    }
}

FILE * safe_fopen(const char * const filename, const char * const modes) {
    FILE * file = fopen(filename, modes);

    if (!file) {
        perror("Could not open the specified file with the specified modes.\n");
        return NULL;
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

    if (client_socket < 0) {
        perror("Socket creation failed");
    }

    return client_socket;
}

int send_request_recieve_id(request_t * const request, const sockaddr_in_t * server_address, const int client_socket) {
    socklen_t address_length = sizeof(sockaddr_t);

    if (sendto(client_socket, request, sizeof(request_t), 0, (sockaddr_t *)server_address, sizeof(sockaddr_t)) < 0) {
        perror("Send failed");
        return -1;
    }

    if (recvfrom(client_socket, &(request->id), sizeof(size_t), 0, (sockaddr_t *)server_address, &address_length) < 0) {
        perror("Recieving id failed.\n");
        return -1;
    }

    return 1;
}

void recieve_response(request_t * const request, const sockaddr_in_t * const server_address, const int client_socket) {
    socklen_t address_length;

    ssize_t recv_len = recvfrom(client_socket, request, sizeof(request_t), 0, (sockaddr_t *)server_address, &address_length);
    if (recv_len < 0) {
        perror("Receive failed.\n");
    }
}

void print_test(const test_t * const test) {
    unsigned char passed = test->request.result == test->expected_answer;

    if (!passed)
        printf("\033[0;31m================================\033[0m\n");
    else
        printf("\033[0;32m================================\033[0m\n");

    print_request(&(test->request));
    printf("Expected answer: %ld\n", test->expected_answer);
    if (!passed) {
        printf("\033[0;31mTest FAILED\033[0m\n");
        printf("\033[0;31m================================\033[0m\n");
    } else {
        printf("\033[0;32mTest PASSED\033[0m\n");
        printf("\033[0;32m================================\033[0m\n");
    }
}

bool fill_test(test_t * const test, FILE * file) {
    memset((void *)test, 0, sizeof(test_t));

    if (fscanf(file, "%hhu ", &(test->request.operation)) != 1) {
        return false;
    }

    for (int i = 0; i < MAX_ARGUMENTS; ++i) {
        if (fscanf(file, "%ld ", &(test->request.arguments[i])) != 1)
            return false;
    }
    if (fscanf(file, "%ld", &(test->expected_answer)) != 1) {
        return false;
    }

    fscanf(file, "\n");
    return true;
}

void print_summary(const int count, const int passed) {
    if (count == passed) {
        printf("\033[0;32mSUMMARY: \n\tTests ran: %d\n\tTests passed %d\n\tTests failed %d\n\tPercentage: %.2lf%%\033[0m\n", count, passed, count - passed, ((double)passed) / count * 100);
    } else {
        printf("\033[0;31mSUMMARY: \n\tTests ran: %d\n\tTests passed %d\n\tTests failed %d\n\tPercentage: %.2lf%%\033[0m\n", count, passed, count - passed, ((double)passed) / count * 100);
    }
}

void test_primes(const char * const in_filename) {
    sockaddr_in_t server_address;
    test_t        test;

    FILE * in_file = safe_fopen(in_filename, "r+");
    int    sock    = create_socket();

    if (sock < 0) {
        safe_fclose(in_file);
        return;
    } else if (!in_file) {
        return;
    }

    set_server_address(&server_address, AF_INET, TEST_PORT, SERVER_IP);

    int tests_count  = 0;
    int tests_passed = 0;

    while (fill_test(&test, in_file)) {
        if (send_request_recieve_id(&(test.request), &server_address, sock) < 0) {
            break;
        }
        recieve_response(&(test.request), &server_address, sock);

        tests_count++;
        print_test(&test);
        if (test.expected_answer == test.request.result)
            tests_passed++;
    }

    safe_fclose(in_file);
    close(sock);
    print_summary(tests_count, tests_passed);
}

#pragma endregion
