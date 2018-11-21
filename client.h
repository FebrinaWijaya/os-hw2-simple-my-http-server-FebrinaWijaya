#ifndef CLIENT_H
#define CLIENT_H

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

//will be called inside createDIrInPath()
void checkCreateDir(char* name);
//check if file/dir exists in ./output directory, if not, create.
char* createDirInPath(char *path, int path_type);
//establish connection to server via socket
int establishConnection(int portno, char* hostname);
/*****************************************************************
*Send GET Request to server with format:						 *
*GET QUERY_FILE_OR_DIR HTTP/1.x\r\nHOST: LOCALHOST:PORT\r\n\r\n  *
*And then wait for response from server and print the response.  *
*If QUERY_FILE_OR_DIR is a valid dir, create thread for each     *
*file/subdirectory to get the content.							 *
*Save the file/dir under ./output								 *
*****************************************************************/
void *GET(void *req_info_void);
//prints error message and exit
void error(char *msg);

#endif
