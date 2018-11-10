#include "server.h"

/* Generic */
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <strings.h>
#include <unistd.h>

/* Network */
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

/* Others */
#include "status.h"

#define BUF_SIZE 128

void header(int handler, int status)
{
    // char* output = malloc(sizeof(char));
//    output[0] = '\0';
    char header[BUF_SIZE] = {0};
    int len = 0;
    if (status == 0) {
        len = sprintf(header, "HTTP/1.0 200 OK\r\n\r\n");
    } else if (status == 1) {
        len = sprintf(header, "HTTP/1.0 403 Forbidden\r\n\r\n");
    } else {
        len = sprintf(header, "HTTP/1.0 404 Not Found\r\n\r\n");
    }
    send(handler, header, strlen(header), 0);
}

void resolve(int handler)
{
    //int size;
    char buf[BUF_SIZE];
    char *method;
    char *filename;

    recv(handler, buf, BUF_SIZE, 0);
    method = strtok(buf, " ");
    if (strcmp(method, "GET") != 0) return;

    filename = strtok(NULL, " ");
    if (filename[0] == '/') filename++;

    if (access(filename, F_OK) != 0) {
        header(handler, 2);
        return;
    } else if (access(filename, R_OK) != 0) {
        header(handler, 1);
        return;
    } else {
        header(handler, 0);
    }

    FILE *file = fopen(filename, "r");
    while(fgets(buf, BUF_SIZE, file)) {
        send(handler, buf, strlen(buf), 0);
        memset(buf, 0, BUF_SIZE);
    }
    fclose(file);
}
void error(char *msg)
{
    perror(msg);
    exit(1);
}

int bindListener(int portno)
{
    int sockfd;
    struct sockaddr_in serv_addr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
             sizeof(serv_addr)) < 0)
        error("ERROR on binding");
    return sockfd;
}



int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "USAGE: ./httpserver <port>\n");
        return 1;
    }

    // // bind a listener
    int server = bindListener(atoi(argv[1]));
    // int server = bindListener(getAddrInfo(argv[1]));
    if (server < 0) {
        fprintf(stderr, "[main:72:bindListener] Failed to bind at port %s\n", argv[1]);
        return 2;
    }

    if (listen(server, 10) < 0) {
        perror("[main:82:listen]");
        return 3;
    }

    // accept incoming requests asynchronously
    int handler;
    socklen_t size;
    struct sockaddr_storage client;
    while (1) {
        size = sizeof(client);
        handler = accept(server, (struct sockaddr *)&client, &size);
        if (handler < 0) {
            perror("[main:82:accept]");
            continue;
        }
        resolve(handler);
        close(handler);
    }

    close(server);
    return 0;
}

