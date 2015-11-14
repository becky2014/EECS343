#include <stdlib.h>
#include <signal.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>

#include "thread_pool.h"
//#include "thread_pool.c"
#include "seats.h"
#include "util.h"

#define BUFSIZE 1024
#define FILENAMESIZE 100

void shutdown_server(int);
void parseProcess(void *argument);

int listenfd;

// TODO: Declare your threadpool!
pool_t *threadpool = NULL;

int main(int argc,char *argv[])
{
    int flag, num_seats = 20;
    int connfd = 0;
    struct sockaddr_in serv_addr;

    char send_buffer[BUFSIZE];
    
    listenfd = 0; 

    int server_port = 8080;

    if (argc > 1)
    {
        num_seats = atoi(argv[1]);      //convert char to int
    } 

    if (server_port < 1500)
    {
        fprintf(stderr,"INVALID PORT NUMBER: %d; can't be < 1500\n",server_port);
        exit(-1);
    }
    
    if (signal(SIGINT, shutdown_server) == SIG_ERR)      //set a function to handle a signal
        printf("Issue registering SIGINT handler");

    listenfd = socket(AF_INET, SOCK_STREAM, 0);      //establish a socket using default TCP protocol (0),return socket file descriptor
    if ( listenfd < 0 ){
        perror("Socket");
        exit(errno);
    }
    printf("Established Socket: %d\n", listenfd);
    flag = 1;
    setsockopt( listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag) );      //set options on sockets
    //SO_REUSEADDR    enables local address reuse

    // Load the seats;
    load_seats(num_seats);

    // set server address 
    memset(&serv_addr, '0', sizeof(serv_addr));
    memset(send_buffer, '0', sizeof(send_buffer));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);          //host to network long
    serv_addr.sin_port = htons(server_port);

    // bind to socket
    if ( bind(listenfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) != 0)
    {
        perror("socket--bind");
        exit(errno);
    }

    // listen for incoming requests
    listen(listenfd, 10);   //10, The backlog parameter defines the maximum length for the queue of pending connections

    // TODO: Initialize your threadpool!
    threadpool = pool_create(40, 50);       //pool_t *pool_create(int thread_count, int queue_size); 50/100

    // This while loop "forever", handling incoming connections
    while(1)
    {
        connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);

        /*********************************************************************
            You should not need to modify any of the code above this comment.
            However, you will need to add lines to declare and initialize your 
            threadpool!

            The lines below will need to be modified! Some may need to be moved
            to other locations when you make your server multithreaded.
        *********************************************************************/
        
        //TODO: add parse_request and process_request to the working queue 
        parse_argument *argument = (parse_argument *)malloc(sizeof(parse_argument));
        argument->connfd = connfd;
        argument->request = (struct request *)malloc(sizeof(struct request));
        pool_add_task(threadpool, parseProcess, (void *)argument);

        // struct request req;
        // parse_request fills in the req struct object
        // parse_request(connfd, &req);
        // process_request reads the req struct and processes the command
        // process_request(connfd, &req);
        // close(connfd);
    }
}

void parseProcess(void *argument) {
    parse_argument * arg = (parse_argument *)argument;
    parse_request(arg->connfd, arg->request);
    process_request(arg->connfd, arg->request);
    close(arg->connfd);
    free(arg->request);
    free(arg);
}

void shutdown_server(int signo){
    printf("Shutting down the server...\n");
    
    // TODO: Teardown your threadpool
    pool_destroy(threadpool);

    // TODO: Print stats about your ability to handle requests.  
    //print the average time 
    unload_seats();
    close(listenfd);
    exit(0);
}
