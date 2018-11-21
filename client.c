#include "client.h"

/* Generic */
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <strings.h>
#include <unistd.h>
#include <dirent.h>
#include <pthread.h>

/* Network */
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>

#define BUF_SIZE 256
#define RCV_SIZE 2048
#define FILE_PATH 0
#define FOLDER_PATH 1

typedef struct {
    int clientfd;
    char *path;
    char *portno;
    char *hostname;
} Req_info;

void checkCreateDir(char* name)
{
    struct stat st = {0};

    if (stat(name, &st) == -1) {
        mkdir(name, 0750);
        //printf("created the directory %s\n",name);
    }
}
char* createDirInPath(char *path, int path_type) //e.g. secfolder/json.db -> output/secfolder/json.db
{
    char buf[BUF_SIZE];
    //printf("=======\nReprint file content:\n%s",content);
    strcpy(buf, "output/");
    char *temp = malloc(sizeof(char)*(strlen(buf)+strlen(path)+1));
    char *output_path = malloc(sizeof(char)*(strlen(buf)+strlen(path)+1));
    sprintf(output_path,"%s%s", buf, path);
    //printf("%s\n",path);

    char *delim = "/";

    char *pch = strstr(output_path, delim);
    while(pch!=NULL) {
        memcpy(temp,output_path,pch-output_path+1);
        temp[pch-output_path] = '\0';
        //printf("checking dir %s\n",temp);
        checkCreateDir(temp);
        //printf("%s\n", temp);
        pch = strstr(pch+1, delim);
    }
    free(temp);
    if(path_type==FOLDER_PATH) {
        checkCreateDir(output_path);
    }
    return output_path;
}

int establishConnection(int portno, char* hostname);
// Send GET request
void *GET(void *req_info_void)
{
    Req_info *req_info = (Req_info *)req_info_void;
    char req[1000] = {0};
    sprintf(req, "GET %s HTTP/1.x\r\nHOST: localhost:port\r\n\r\n", req_info->path);
    send(req_info->clientfd, req, strlen(req), 0);

    char buf[RCV_SIZE];
    while (recv(req_info->clientfd, buf, RCV_SIZE, 0) > 0) {
        fputs(buf, stdout);
        char* respond = malloc(sizeof(char)*(strlen(buf)+1));
        strcpy(respond, buf);
        printf("\n\n");
        //request
        //HTTP/1.x 200 OK\r\nContent-type: directory\r\nServer: httpserver/1.x\r\n\r\ncontent_1 .. content_n\n
        char *content_type = strtok(buf, " ");
        char *code = strtok(NULL, " ");
        content_type = strtok(NULL, " ");
        content_type = strtok(NULL, "\r");
        //printf("content type: %s\n", content_type);
        if(strcmp(content_type, "directory")==0) {
            char *temp = strtok(NULL, " ");
            temp = strtok(NULL, "\n");
            temp = strtok(NULL, "\n");
            temp = strtok(NULL, " \n");
            char *next_start;// = temp + strlen(temp) + 1;
            if(temp!=NULL)
                next_start = strtok(NULL, " \n");
            //if(temp!=NULL) next_start = strtok(NULL, " \n");
            //printf("1st content: %s", temp);

            int len = strlen(req_info->path);
            char *req_path = malloc(sizeof(char)*(len+1));
            strcpy(req_path,req_info->path);

            while(temp!=NULL) {
                //printf(" %s\n", temp);
                char* new_req_path = realloc(req_path, len+strlen(temp)+2);
                req_path = new_req_path;
                if(req_info->path[strlen(req_info->path)-1]!='/')
                    sprintf(req_path,"%s/%s", req_info->path, temp);
                else
                    sprintf(req_path,"%s%s", req_info->path, temp);
                //printf(" %s\n", req_path);
                req_info->clientfd = establishConnection(atoi(req_info->portno), req_info->hostname);
                if (req_info->clientfd == -1) {
                    printf("ERROR\n");
                    fprintf(stderr, "[GET:56] Failed to connect to: %s:%s%s \n",req_info->hostname,
                            req_info->portno, req_path);
                    free(respond);
                    return  NULL;
                }
                char *req_path_to_send = malloc(sizeof(char)*(strlen(req_path)+1));
                strcpy(req_path_to_send, req_path);
                if(next_start!=NULL) {
                    next_start[strlen(next_start)] = ' ';
                    //printf("%s",next_start);
                    temp = strtok(next_start, " \n");
                    next_start = strtok(NULL, " \n");
                } else
                    temp = NULL;

                Req_info info_to_send = {req_info->clientfd, req_path_to_send,
                                         req_info->portno, req_info->hostname
                                        };

                pthread_t tid;
                int err;
                err = pthread_create(&tid, NULL, GET, (&info_to_send));
                if (err != 0)
                    printf("\ncan't create thread :[%s]", strerror(err));
                else {
                    //printf("\nThread created successfully\n");
                    pthread_join(tid, NULL);
                }

                //GET(&info_to_send);

            }
            char* output_path = createDirInPath(req_info->path, FOLDER_PATH);
            free(output_path);
        } else if(strcmp(code, "200")==0) { //a valid file
            char* path = req_info->path;
            //printf("This is a valid file\n");
            //HTTP/1.x 200 OK\r\nContent-type: directory\r\nServer: httpserver/1.x\r\n\r\ncontent_1 .. content_n\n
            char *content = strtok(respond, " ");
            content = strtok(NULL, " ");
            content = strtok(NULL, " ");
            content = strtok(NULL, "\r");
            content = strtok(NULL, " ");
            content = strtok(NULL, "\n");
            content = strtok(NULL, "\n");
            //content = strtok(NULL, " \n");
            content = content + strlen(content)+1;
            //printf("=======\nReprint file content:\n%s",content);
            char* output_path = createDirInPath(path, FILE_PATH);
            //printf("==============================\n%s",content);
            FILE *w_file = fopen(output_path, "w");
            fprintf(w_file, "%s",content);
            //fputs(content, w_file);
            fclose(w_file);
            free(output_path);
        }
        free(respond);
        memset(buf, 0, BUF_SIZE);
    }
    return NULL;
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
    //char buf[BUF_SIZE];

    if (argc != 7) {
        //fprintf(stderr, "USAGE: ./client <hostname> <port> <request path>\n");
        fprintf(stderr, "USAGE: ./client -t <request path> -h localhost -p <port>\n");
        return 1;
    }

    // Establish connection with <hostname>:<port>
    //clientfd = establishConnection(getHostInfo(argv[1], argv[2]));
    clientfd = establishConnection(atoi(argv[6]), argv[4]);
    if (clientfd == -1) {
        fprintf(stderr,
                "[main:73] Failed to connect to: %s:%s%s \n",
                argv[4], argv[6], argv[2]);
        return 3;
    }

    // Send GET request > stdout
    Req_info info_to_send = {clientfd, argv[2], argv[6], argv[4]};
    GET(&info_to_send);
    // char *port = argv[6];
    // while (recv(clientfd, buf, BUF_SIZE, 0) > 0) {
    //     fputs(buf, stdout);
    //     //request
    //     //HTTP/1.x 200 OK\r\nContent-type: directory\r\nServer: httpserver/1.x\r\n\r\ncontent_1 .. content_n\n
    //     char *content_type = strtok(buf, " ");
    //     content_type = strtok(NULL, " ");
    //     content_type = strtok(NULL, " ");
    //     content_type = strtok(NULL, "\r");
    //     printf("content type: %s\n", content_type);
    //     if(strcmp(content_type, "directory")==0)
    //     {
    //         char *temp = strtok(NULL, " ");
    //         temp = strtok(NULL, "\n");
    //         temp = strtok(NULL, "\n");
    //         temp = strtok(NULL, " ");
    //         printf("1st content: %s\n", temp);
    //     }
    //     memset(buf, 0, BUF_SIZE);
    // }

    close(clientfd);
    return 0;
}
/*
int main(int argc, char *argv[])
{
	return 0;
}
*/

