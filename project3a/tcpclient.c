#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int MAX_LINE = 1024;

int main(int argc, char** argv)
{
    int client_socket, x, y, z, n;
    char buffer[100];
    char* str;
    struct sockaddr_in server;

    char* server_addr = argv[1];
    int server_port = atoi(argv[2]);
    int init_seq = atoi(argv[3]);

    //Initialize socket
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("simplex-talk: socket");
    }
    server.sin_family = AF_INET;
    server.sin_port = htons(server_port);
    server.sin_addr.s_addr = inet_addr(server_addr);

    if ((connect(client_socket, (struct sockaddr*)&server, (socklen_t)sizeof(server))) != 0) {
        perror("simplex-talk: connect");
    }

    //first handshaking, sent buffer
    memset(buffer, 0, sizeof(buffer));
    sprintf(buffer, "HELLO %d", init_seq);
    send(client_socket, buffer, strlen(buffer), 0);

    //second handshaking, clear memory and read buffer from server
    memset(buffer, 0, sizeof(buffer));
    if ((read(client_socket, buffer, MAX_LINE)) == -1) {
        perror("read");
    }
    printf("%s\n", buffer);
    char* temp = buffer;
    y = atoi(temp + 6);
    if (y != (init_seq + 1)) {
        printf("ERROR\n");
        exit(0);
    }

    //third handshaking, sent buffer
    memset(buffer, 0, sizeof(buffer));
    z = y + 1;
    sprintf(buffer, "HELLO %d", z);
    send(client_socket, buffer, strlen(buffer), 0);

    close(client_socket);
    return (0);
}
