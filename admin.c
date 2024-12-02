#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "direcive.h"

#define SERVER_PORT 11001
#define SERVER_IP "127.0.0.1"
#define BUF_SIZE 1024

int main() {
    int sock;
    struct sockaddr_in server_addr;
    ssize_t result;
    directive_t directive;
    directive.dir = DIR_HALT;
    directive.params.capacity = 15;
    directive.params.id = 0;

    socklen_t addr_len = sizeof(server_addr);

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed.\n");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) < 0 ) {
        perror("Connection failed.\n");
        return -1;
    }

    ssize_t sent = send(sock, (void *)&directive, sizeof(directive_t), 0);
    printf("Sent %ld bytes.\n", sent);
    recv(sock, (void *)&result, sizeof(ssize_t), 0);
    printf("Result was: %ld\n", result);

    close(sock);
    return 0;
}