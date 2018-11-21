#include "server.h"


pthread_mutex_t lock_acc, lock_proc;


Queue reqQueue;

char* root;

void pushRegQueue(int handler)
{
    Node *node = malloc(sizeof(Node));
    node->handler = handler;
    node->next = NULL;
    if( reqQueue.head == NULL && reqQueue.tail ==NULL) {
        reqQueue.head = reqQueue.tail = node;
    } else {
        (reqQueue.tail)->next = node;
        reqQueue.tail = node;
    }
    reqQueue.count++;
}

int popRegQueue()
{
    if(reqQueue.head == NULL) return -1;
    int handler = (reqQueue.head)->handler;

    Node *temp = reqQueue.head;
    if(reqQueue.count!=1) {
        reqQueue.head = (reqQueue.head)->next;
    } else {
        reqQueue.head = NULL;
        reqQueue.tail = NULL;
    }
    free(temp);
    reqQueue.count--;
    return handler;
}

char* header(int status)
{
    char header[BUF_SIZE] = {0};
    int len = 0;
    if (status == OK) {
        len = sprintf(header, "HTTP/1.x %d OK\r\n", status_code[OK]);
    } else if (status == BAD_REQUEST) {
        len = sprintf(header, "HTTP/1.x %d BAD_REQUEST\r\n", status_code[BAD_REQUEST]);
    } else if (status == NOT_FOUND) {
        len = sprintf(header, "HTTP/1.x %d NOT_FOUND\r\n", status_code[NOT_FOUND]);
    } else if (status == METHOD_NOT_ALLOWED) {
        len = sprintf(header, "HTTP/1.x %d METHOD_NOT_ALLOWED\r\n", status_code[METHOD_NOT_ALLOWED]);
    } else if (status == UNSUPPORT_MEDIA_TYPE) {
        len = sprintf(header, "HTTP/1.x %d UNSUPPORT_MEDIA_TYPE\r\n", status_code[UNSUPPORT_MEDIA_TYPE]);
    }
    char* output = malloc(sizeof(char)*(len+1));
    output[0] = '\0';
    sprintf(output, "%s", header);

    //send(handler, output, strlen(output), 0);
    return output;
}

char *get_extn(char *filename)
{
    char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "dir";
    return dot + 1;
}
void resolve_failed(int handler, char *output)
{
    char buffer[BUF_SIZE] = "Content-Type:\r\nServer: httpserver/1.x\r\n\r\n";
    int len = strlen(output) + strlen(buffer);
    char *output_temp = malloc(sizeof(char)*(strlen(output)+1));
    strcpy(output_temp, output);

    output = (char *)realloc(output, sizeof(char)*(len+1));
    sprintf(output, "%s%s", output_temp,buffer);
    //printf("%s",output);
    send(handler, output, strlen(output)+1, 0);
}

void resolve(int handler)
{
    char buf[BUF_SIZE];
    char *method;
    char *filename_temp, *filename;
    char *output;

    recv(handler, buf, BUF_SIZE, 0);
    method = strtok(buf, " ");

    filename_temp = strtok(NULL, " ");
    filename = malloc(sizeof(char)*(strlen(root)+strlen(filename_temp)+1));

    if (filename_temp[0] == '/') ;//filename++;
    else {
        output = header(BAD_REQUEST);
        resolve_failed(handler, output);
        free(filename);
        return;
    }
    if (strcmp(method, "GET") != 0) {
        output = header(METHOD_NOT_ALLOWED);
        resolve_failed(handler, output);
        free(filename);
        return;
    }
    //filename = (char *)realloc(filename, sizeof(char)*(strlen(root)+strlen(filename)+1));
    sprintf(filename,"%s%s",root,filename_temp);
    //printf("%s\n",filename);

    char *extn = get_extn(filename);
    int i=0;
    while(extensions[i].ext != 0 && strcmp(extensions[i].ext, extn)!=0)
        ++i;
    if(extensions[i].ext == 0 && strcmp(extn, "dir")!=0) {
        //file extension is not supported and the requested path is not a directory
        output = header(UNSUPPORT_MEDIA_TYPE);
        resolve_failed(handler, output);
        free(filename);
        return;
    }

    if (access(filename, F_OK) != 0) {
        output = header(NOT_FOUND);
        resolve_failed(handler, output);
        free(filename);
        return;
    }

    output = header(OK);
    //checkCreateDir("output");
    if(strcmp(extn, "dir")!=0) {
        //requested path is a valid file
        char buffer[BUF_SIZE];
        int len = sprintf(buffer, "Content-Type: %s\r\nServer: httpserver/1.x\r\n\r\n", extensions[i].mime_type);
        len += strlen(output);
        char *output_temp = (char *)realloc(output, sizeof(char)*(len+2048+1));
        output = output_temp;
        strcat(output, buffer);

        //int output_init_len = strlen(output);
        FILE *file = fopen(filename, "r");
        //FILE *w_file = fopen(path, "w");
        while(fgets(buf, BUF_SIZE, file)) {
            //if(strlen(buf)+strlen(output)-output_init_len<=128)
            strcat(output, buf);
            //fprintf(w_file, "%s",buf);
            memset(buf, 0, BUF_SIZE);
        }
        send(handler, output, strlen(output)+1, 0);
        //printf("%s\n==============\n",output);
        fclose(file);
        free(output);
        //fclose(w_file);
    } else { //requested path is a valid directory
        char buffer[BUF_SIZE] = "Content-Type: directory\r\nServer: httpserver/1.x\r\n\r\n";
        int len = strlen(output) + strlen(buffer);

        char *output_temp = (char *)realloc(output, sizeof(char)*(len+1));
        output = output_temp;

        output_temp = malloc(sizeof(char)*(len+1));
        strcpy(output_temp, output);

        strcat(output,buffer);
        //sprintf(output, "%s%s", output_temp,buffer);
        //printf("%s",output);
        //send(handler, output, strlen(output)+1, 0);

        DIR *d;
        struct dirent *dir;
        d = opendir(filename);
        int count = 0;
        if (d) {
            while ((dir = readdir(d)) != NULL) {
                if((strcmp(dir->d_name,".")==0) || (strcmp(dir->d_name,"..")==0))
                    continue;
                len = strlen(output)+strlen(dir->d_name);
                char *output_temp = (char *)realloc(output, sizeof(char)*(len+3)); //for space, \n, and \0
                output = output_temp;

                output_temp = malloc(sizeof(char)*(strlen(output)+1));
                strcpy(output_temp, output);

                if(count == 0) {
                    count++;
                    sprintf(output, "%s%s", output_temp, dir->d_name);
                } else
                    sprintf(output, "%s %s", output_temp, dir->d_name);
                //printf("%s\n", dir->d_name);
                free(output_temp);
            }
        }
        len = strlen(output);
        output[len] = '\n';
        output[len+1] = '\0';
        //printf("Dir:%s\n\n%s\n==============\n",filename,output);
        send(handler, output, strlen(output)+1, 0);
        closedir(d);
        free(output);
    }
    free(filename);
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

//thread to create
void* takeRequest()
{
    //printf("yeay1\n");
    while(1) {
        //printf("");
        fflush(stdout);
        if(reqQueue.count>0) {
            //printf("yeay2\n");
            pthread_mutex_lock(&lock_acc);
            int handler = popRegQueue();
            pthread_mutex_unlock(&lock_acc);
            pthread_mutex_lock(&lock_proc);
            resolve(handler);
            close(handler);
            pthread_mutex_unlock(&lock_proc);
        }
    }
}


int main(int argc, char **argv)
{
    if (argc != 7) {
        fprintf(stderr, "USAGE: ./server -r root -p <port> -n <thread_number>\n");
        return 1;
    }

    root = malloc(sizeof(char)*(strlen(argv[2])+1));
    strcpy(root, argv[2]);
    // // bind a listener
    int server = bindListener(atoi(argv[4]));
    if (server < 0) {
        fprintf(stderr, "[main:72:bindListener] Failed to bind at port %s\n", argv[4]);
        return 2;
    }

    if (listen(server, 10) < 0) {
        perror("[main:82:listen]");
        return 3;
    }

    //for thread pool
    if (pthread_mutex_init(&lock_acc, NULL) != 0) {
        printf("\n mutex init failed\n");
        return 1;
    }
    if (pthread_mutex_init(&lock_proc, NULL) != 0) {
        printf("\n mutex init failed\n");
        return 1;
    }
    reqQueue.count = 0;
    int threadnum = atoi(argv[6]);
    int i;
    for(i = 0; i < threadnum; i++) {
        pthread_t tid;
        int err;
        err = pthread_create(&tid, NULL, takeRequest, NULL);
        if (err != 0)
            printf("\ncan't create thread :[%s]", strerror(err));
        // else
        // {
        //     printf("\nThread created successfully\n");
        //     pthread_join(tid, NULL);
        // }
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
        //printf("request accepted; handler = %d\n",handler);
        pthread_mutex_lock(&lock_acc);
        pushRegQueue(handler);
        //printf("%d\n",reqQueue.count);
        pthread_mutex_unlock(&lock_acc);
        //resolve(handler);
        //close(handler);
    }

    close(server);
    return 0;
}