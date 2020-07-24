#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int MAX_PENDING = 5;
int MAX_LINE = 1024;

int main(int argc, char** argv)
{
    int sockfd, new_s, x, y, z, n;
    struct sockaddr_in server;
    char buffer[100];
    char* str;
    int add_len = sizeof(server);
    int server_port = atoi(argv[1]);

    //create a socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("simplex-talk: socket");
    }
    bzero(&server, sizeof(server)); //set to 0
    server.sin_family = AF_INET;
    server.sin_port = htons(server_port);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (bind(sockfd, (struct sockaddr*)&server, sizeof(server)) == -1) {
        perror("simplex-talk: bind");
    }

    if (listen(sockfd, MAX_PENDING) == -1) {
        perror("simplex-talk: listen");
    }
    while (1) {
        if ((new_s = accept(sockfd, (struct sockaddr*)&server, (socklen_t*)&add_len)) == -1) {
            perror("simplex-talk: accept");
        }

        //first handshaking
        memset(buffer, 0, sizeof(buffer)); //reset memory for buffer
        if ((n = read(new_s, buffer, MAX_LINE)) == -1) {
            perror("read");
        }
        buffer[n] = '\0';
        printf("%s\n", buffer);
        char* temp = buffer;
        x = atoi(temp + 6);
        y = x + 1;

        //second handshaking
        memset(buffer, 0, sizeof(buffer));
        sprintf(buffer, "HELLO %d", y);
        send(new_s, buffer, sizeof(buffer), 0);

        //third handshaking
        memset(buffer, 0, sizeof(buffer));
        if ((n = read(new_s, buffer, MAX_LINE)) == -1) {
            perror("read");
        }
        printf("%s", buffer);
        char* temp2 = buffer;
        z = atoi(temp2 + 6);
        if (z != (y + 1)) {
            printf("ERROR\n");
            exit(0);
        }
    }
    return (0);
}
