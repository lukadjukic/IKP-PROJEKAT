#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "queue.h"

#define SERVER_PORT 11000
#define SERVER_IP "127.0.0.1"
#define BUF_SIZE 1024

int main() {
    int sock;
    struct sockaddr_in server_addr;

    REQUEST r;
    r.id = 0;
    r.operation = 1;
    memset(&r.arguments, 0, MAX_ARGUMENTS*sizeof(long));
    print_request(&r);

    char *message = (char*)&r;
    char buffer[BUF_SIZE];
    socklen_t addr_len = sizeof(server_addr);

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (sendto(sock, message, sizeof(REQUEST), 0, (struct sockaddr *)&server_addr, addr_len) < 0) {
        perror("Send failed");
        close(sock);
        return -1;
    }

    ssize_t recv_len = recvfrom(sock, buffer, BUF_SIZE, 0, (struct sockaddr *)&server_addr, &addr_len);
    if (recv_len < 0) {
        perror("Receive failed");
        close(sock);
        return -1;
    }

    REQUEST* pr = (REQUEST*)buffer;
    print_request(pr);

    close(sock);
    return 0;
}
