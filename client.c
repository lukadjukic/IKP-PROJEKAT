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
unsigned char fill_request(request_t * const request, FILE * file);
FILE *        safe_fopen(const char * const filename, const char * const modes);
void          safe_fclose(FILE * file);
void          print_response(const request_t * const request, FILE * file);

int main() {
    int           sock;
    sockaddr_in_t server_address;
    request_t     request;
    FILE *        in_file; 
    FILE *        out_file;

    const char * const in_filename = "input.txt";
    const char * const out_filename = "output.txt";    

    out_file = safe_fopen(out_filename, "w+"); 
    in_file  = safe_fopen(in_filename, "r+");
    
    sock = create_socket();

    if (sock < 0) {
        safe_fclose(in_file);
        safe_fclose(out_file);
        return -1;
    }

    while (fill_request(&request, in_file)){
        set_server_address(&server_address);
        if (send_request_recieve_id(&request, &server_address, sock)  < 0) {
            break;
        }
        recieve_response(&request, &server_address, sock);
        print_response(&request, out_file);
    }

    close(sock);
    safe_fclose(in_file);
    safe_fclose(out_file);

    return 0;
}

#pragma region functions

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

    ++counter;
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

unsigned char fill_request(request_t * const request, FILE * file){
    memset((void *)request, 0, sizeof(request_t));
    
    if ( fscanf(file, "%hhu ", &(request -> operation)) != 1  ){
        return 0;
    }

    for (int i = 0; i < MAX_ARGUMENTS; ++i) {
        if ( fscanf(file,"%ld ", &(request -> arguments[i])) != 1 )
            return 0;
        fscanf(file, "\n");
    }
    
    return 1;
}

void print_response(const request_t * const request, FILE * file) {
    fprintf(file,"%lu %hhu %u %u %u %ld\n", request -> id, request -> operation, request -> flags.stopped, request -> flags.invalid, request -> flags.handled, request -> result);
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

#pragma endregion

