#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "./common/request.h"

#define SERVER_PORT 11000
#define SERVER_IP   "127.0.0.1"

int           create_socket(int domain, int port, int protocol);
void          set_server_address(sockaddr_in_t * address, const sa_family_t family, const unsigned short port, const char * const server_ip);
int           send_request_recieve_id(request_t * const request, const sockaddr_in_t * server_address, const int client_socket);
void          recieve_response(request_t * const request, const sockaddr_in_t * const server_address, const int client_socket);
unsigned char fill_request(request_t * const request, FILE * file);
void          print_response(const request_t * const request, FILE * file);

int main() {
    sockaddr_in_t server_address;
    request_t     request;

    int sock = create_socket(AF_INET, SOCK_DGRAM, 0);

    if (sock < 0) {
        return -1;
    }

    set_server_address(&server_address, AF_INET, SERVER_PORT, SERVER_IP);

    while (fill_request(&request, stdin)) {
        if (send_request_recieve_id(&request, &server_address, sock) < 0) {
            break;
        }
        recieve_response(&request, &server_address, sock);
        print_response(&request, stdout);
    }

    shutdown(sock, SHUT_RDWR);
    close(sock);

    return 0;
}

#pragma region functions

int create_socket(int domain, int type, int protocol) {
    int client_socket = socket(domain, type, protocol);

    if (client_socket < 0) {
        perror("Socket creation failed");
    }

    return client_socket;
}

void set_server_address(sockaddr_in_t * address, const sa_family_t family, const unsigned short port, const char * const server_ip) {

    address->sin_family      = family;
    address->sin_port        = htons(port);
    address->sin_addr.s_addr = inet_addr(server_ip);
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

    print_request(request);
}

unsigned char fill_request(request_t * const request, FILE * file) {
    memset((void *)request, 0, sizeof(request_t));

    if (fscanf(file, "%hhu ", &(request->operation)) != 1) {
        return 0;
    }

    for (int i = 0; i < MAX_ARGUMENTS; ++i) {
        if (feof(file))
            return 0;
        if (fscanf(file, "%ld ", &(request->arguments[i])) != 1)
            return 0;
    }

    return 1;
}

void print_response(const request_t * const request, FILE * file) {
    fprintf(file, "%lu %hhu %u %u %u %ld\n", request->id, request->operation, request->flags.stopped, request->flags.invalid, request->flags.handled, request->result);
}

#pragma endregion
