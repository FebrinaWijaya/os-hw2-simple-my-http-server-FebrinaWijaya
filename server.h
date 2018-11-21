#ifndef SERVER_H
#define SERVER_H

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

/* Others */
#include "status.h"

#define BUF_SIZE 128

extern int errno;

//for request queue
typedef struct node {
    int handler;
    struct node *next;
} Node;
typedef struct queue {
    int count;
    Node *head;
    Node *tail;
} Queue;

//for request queue
void pushRegQueue(int handler);
int popRegQueue();
//get header based on given request status
char* header(int status);
char *get_extn(char *filename);
//send failure output to client
void resolve_failed(int handler, char *output);
//check if request is valid and send corresponding output
void resolve(int handler);
//send message to client when error occurs
void error(char *msg);
//bind server socket on given port
int bindListener(int portno);
//thread to take request from queue
void* takeRequest();

#endif
