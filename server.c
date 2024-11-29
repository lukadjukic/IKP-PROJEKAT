#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "request.h"

#define BUFFER_SIZE 1024
#define SERVER_PORT 11000
typedef struct sockaddr_in sockaddr_in_t;

struct arguments_t {
    QUEUE* pqueue;
    socklen_t addr_len;
    int sock;
};


void reciever(QUEUE* pqueue, socklen_t addr_len, int sock) {
    sockaddr_in_t client_addr;
    REQUEST buffer;
    printf("[DEBUG]Server listening on port: %u ...\n", SERVER_PORT);
    unsigned char counter = 5;
    register_producer(pqueue);
    while ( --counter ) {
        ssize_t recv_len = recvfrom(sock, &buffer, sizeof(REQUEST), 0, (struct sockaddr*)&client_addr, &addr_len);
    
        if( recv_len < 0 ){
            perror("Recieve failed.\n");
            continue;
        } else if(recv_len < sizeof(REQUEST)){
            perror("Recieved request invalid format.\n");
            continue;
        }

        REQUEST* pr = &buffer;
        printf("Client: %s:%u sent request.\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        print_request(pr);
        pr -> id = counter;
        memcpy(&(pr -> client_addr), &client_addr, sizeof(sockaddr_in_t));
        enqueue(pqueue, pr);
    }
    deregister_producer(pqueue);
}

void sender(QUEUE* pqueue, socklen_t addr_len, int sock) {
    while (1) {
        REQUEST* req = dequeue(pqueue);
        if (!req) {
            break;
        }
        sockaddr_in_t* client_addr = &(req -> client_addr); 
        if( sendto(sock, req, sizeof(REQUEST), 0, (struct sockaddr*)client_addr, addr_len) < 0){
            perror("Send failed.\n");
            continue;
        }
        free(req);
        req = NULL;
    }
}
void* _sender(void* arg) {
    struct arguments_t* arguments =  ( struct arguments_t * ) arg;
    printf("[DEBUG] Sender thread started.\n");
    sender( arguments -> pqueue, arguments -> addr_len, arguments -> sock );
    printf("[DEBUG]Sender thread finished.\n");
    return NULL;
}

void* _reciever(void* arg) {
    printf("[DEBUG] Reciever thread started.\n");
    struct arguments_t* arguments =  ( struct arguments_t * ) arg;
    reciever(arguments -> pqueue, arguments -> addr_len, arguments -> sock);
    printf("[DEBUG] Reciever thread finished.\n");
    return NULL;
}

int main() {
    int sock;
    sockaddr_in_t server_addr;
    REQUEST buffer;
    socklen_t addr_len = sizeof(sockaddr_in_t);
    QUEUE q;
    pthread_t sender, reciever;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if ( sock < 0 ) {
        perror("Socket creation failed.\n");
        return -1;
    }
    memset(&server_addr, 0, sizeof(sockaddr_in_t));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if ( bind(sock, (struct sockaddr*)&server_addr, sizeof(sockaddr_in_t)) < 0) {
        perror("Socket binding failed.\n");
        close(sock);
        return -2;
    }

    init_queue(&q, 0);
    struct arguments_t args  = {&q, addr_len, sock};

    if (pthread_create(&reciever, NULL, _reciever, (void*)&args) != 0) {
        perror("Creation of thread sender failed.\n");
        close(sock);
        return -3;
    }
    sleep(1);

    if (pthread_create(&sender, NULL, _sender, (void*)&args) != 0) {
        printf("Creation of thread reciever failed.\n");
        close(sock);
    }

    pthread_join(sender, NULL);
    pthread_join(reciever, NULL);

    delete_queue(&q);
    close(sock);
    return 0;
}