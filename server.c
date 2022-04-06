#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT "28028"
#define BACKLOG 15
#define MAXDATASIZE 128

int sockfd, new_sockfd, numbytes;
unsigned char mip_str[INET_ADDRSTRLEN];
struct addrinfo hints;
struct addrinfo *res, *p;
socklen_t sin_size;
struct sockaddr_in their_addr;
int yes=1;
int rv;
char buf[MAXDATASIZE];
char new_buf[MAXDATASIZE];


int main(int argc, char *argv[])
{
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype=SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv=getaddrinfo(NULL, PORT, &hints, &res) != 0))
    {
        printf("ERROR: %s (%s:%d)\n", strerror(errno), __FILE__, __LINE__);
        return 1;
    }

    for(p = res; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("server:socket");
            continue;
        }
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        {
            perror("server:socket in use");
            exit(1);
        }
        
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("server:bind");
            continue;
        }
        break;
    }
    
    freeaddrinfo(res);
    
    if (p == NULL)
    {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }
    
    if (listen(sockfd, BACKLOG) == -1)
    {
        perror("server:listen");
        exit(1);
    }

    printf("server is waiting for connections!\n");
    for (;;)
    {
        sin_size = sizeof(their_addr);
        new_sockfd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_sockfd == -1)
        {
            perror("accept");
            continue;
        }

        inet_ntop(AF_INET, &(their_addr.sin_addr), mip_str, INET_ADDRSTRLEN);
        printf("server: connection from %s\n", mip_str);
        
        if (!fork())
        {
            close(sockfd);
            do{
            if ((numbytes = recv(new_sockfd, buf, MAXDATASIZE-1, 0))== -1)
            {
               perror("recv");
               exit(1);
            }
            buf[numbytes] = '\0';
            strcpy(new_buf, buf);
            strcat(new_buf, " message from:");
            strcat(new_buf, mip_str);
            printf("Server: received '%s'\n", new_buf);
            }
            while(strlen(buf)>0);
            close(new_sockfd);
            exit(0);
        }
        
        close(new_sockfd);
    }
        return 0;
    }
