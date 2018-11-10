#include "client.h"

/* Generic */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

/* Network */
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#define BUF_SIZE 100

// Send GET request
void GET(int clientfd, char *path)
{
    char req[1000] = {0};
    sprintf(req, "GET %s HTTP/1.0\r\n\r\n", path);
    send(clientfd, req, strlen(req), 0);
}

void error(char *msg)
{
    perror(msg);
    exit(0);
}

// Get host information (used to establishConnection
int establishConnection(int portno, char* hostname)
{
    int clientfd;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientfd < 0)
        error("ERROR opening socket");
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr_list[0],
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(clientfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
        error("ERROR connecting");
    return clientfd;
}

int main(int argc, char **argv)
{
    int clientfd;
    char buf[BUF_SIZE];

    if (argc != 4) {
        fprintf(stderr, "USAGE: ./httpclient <hostname> <port> <request path>\n");
        return 1;
    }

    // Establish connection with <hostname>:<port>
    //clientfd = establishConnection(getHostInfo(argv[1], argv[2]));
    clientfd = establishConnection(atoi(argv[2]), argv[1]);
    if (clientfd == -1) {
        fprintf(stderr,
                "[main:73] Failed to connect to: %s:%s%s \n",
                argv[1], argv[2], argv[3]);
        return 3;
    }

    // Send GET request > stdout
    GET(clientfd, argv[3]);
    while (recv(clientfd, buf, BUF_SIZE, 0) > 0) {
        fputs(buf, stdout);
        memset(buf, 0, BUF_SIZE);
    }

    close(clientfd);
    return 0;
}
/*
int main(int argc, char *argv[])
{
	return 0;
}
*/

