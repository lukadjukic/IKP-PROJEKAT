#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "queue.h"

#define SERVER_PORT 11000
#define SERVER_IP "127.0.0.1"
int counter = 0;

int           create_socket();
int           send_request_recieve_id(request_t * const request, const sockaddr_in_t * server_address, const int client_socket);
void          recieve_response(request_t * const request, const sockaddr_in_t * const server_address, const int client_socket);
void          set_server_address(sockaddr_in_t * address);
unsigned char fill_request(request_t * const request);

int main() {
    int           sock;
    sockaddr_in_t server_address;
    request_t     request;
    
    sock = create_socket();
    if (sock < 0) {
        return -1;
    }

    while (fill_request(&request)){
        set_server_address(&server_address);
        if (send_request_recieve_id(&request, &server_address, sock)  < 0) {
            break;
        }

        recieve_response(&request, &server_address, sock);
    }

    
    close(sock);
    return 0;
}

int create_socket() {
    int client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    
    if (client_socket < 0)  {
        perror("Socket creation failed");
    }

    return client_socket;
}

void set_server_address(sockaddr_in_t * address) {

    address -> sin_family       = AF_INET;
    address -> sin_port         = htons(SERVER_PORT);
    address -> sin_addr.s_addr  = inet_addr(SERVER_IP);
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

    print_request(request);
}

unsigned char fill_request(request_t * const request){
    memset((void *)request, 0, sizeof(request_t));
    request -> operation = 3;
    request -> arguments[0] = -10;
    
    return !counter++;
}