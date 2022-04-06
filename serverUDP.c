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
#include <sys/select.h>
#include <time.h>

#define PORT "19370"
#define LENGTH 15
#define MAXDATASIZE 256

unsigned char my_str[MAXDATASIZE];
unsigned char new_str[MAXDATASIZE];
struct addrinfo hints;
struct addrinfo *res, *p;
struct sockaddr_in c;
int c_len = sizeof(c);
int sockfd;
int rv;
char characters[62] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

int main(int argc, char *argv[])
{
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // using ipv4, udp, self ip address

    if ((rv = getaddrinfo(NULL, PORT, &hints, &res) != 0)) // preparing network structures
    {
        printf("ERROR: %s (%s:%d)\n", strerror(errno), __FILE__, __LINE__);
        return 1;
    }

    for (p = res; p != NULL; p = p->ai_next) // looking for potential sockets
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("server:socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) // binding socket
        {
            close(sockfd);
            perror("server:bind");
            continue;
        }
        break;
    }

    freeaddrinfo(res); // freeing memory, cause res is no longer neccessary

    if (p == NULL)
    {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    printf("server is waiting to recvfrom!\n");
    // server now is ready

    fd_set readfds;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 5000;
    int r;
    int pos;
    struct sockaddr_in addr_list[10]; // list of hosts that are active and waiting for messages
    unsigned int list_len = 0;        // length of addr_list
    for (;;)
    {
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        if ((r = select(sockfd + 1, &readfds, NULL, NULL, &tv)) < 0) //checking if there are new messages
        {
            printf("ERROR: %s\n", strerror(errno));
        }
        else if (r > 0)
        {
            if ((pos = recvfrom(sockfd, my_str, MAXDATASIZE, 0, (struct sockaddr *)&c, &c_len)) == -1) //
            {
                perror("Recvfrom");
                exit(-4);
            }
            my_str[pos] = '\0';
            unsigned int my_str_len = strlen(my_str);
            if (my_str_len < LENGTH && my_str_len > 1)
            {
                printf("First message from: %s\n", inet_ntoa(c.sin_addr)); //recieved the first message
                addr_list[list_len] = c; //adding an address to addr_list
                list_len++;
            }
            else if (my_str_len >= LENGTH)
            {
                printf("Recv from %s: %s\n", inet_ntoa(c.sin_addr), my_str);  //recieved a normal message
            }
            else
            {
                //deleting an address from addr_list
                char ip_printable[INET_ADDRSTRLEN];
                char existing_ip[INET_ADDRSTRLEN];
                memcpy(ip_printable, inet_ntoa(c.sin_addr), INET_ADDRSTRLEN); //copying ip addr which should be deleted to ip_printable
                printf("Host %s stopped being active\n", ip_printable);
                unsigned i, j;
                for (i = 0; i < list_len; i++)
                {
                    memcpy(existing_ip, inet_ntoa(addr_list[i].sin_addr), INET_ADDRSTRLEN); //copying an address from the list to existing_ip
                    int condition =  !strcmp(ip_printable, existing_ip); //comparing addresses
                    if (condition){
                        j = i;
                    }
                }
                for (; j < list_len; j++)
                {
                    addr_list[j] = addr_list[j + 1];
                }
                memset(&addr_list[j+1], 0, sizeof(struct sockaddr_in)); //cleaning memory
                list_len--;
            }
        }
        else
        {
            if (list_len > 0)
            {
                unsigned int index = rand() % list_len;
                for (size_t i = 0; i < LENGTH; i++)
                {
                    new_str[i] = characters[rand() % strlen(characters)];
                }

                if ((pos = sendto(sockfd, new_str, strlen(new_str), 0, (const struct sockaddr *)&(addr_list[index]), c_len)) == -1) // sending random string
                {
                    perror("client: sendto");
                    exit(1);
                }
            }
        }

        /*for (unsigned int k = 0; k < list_len; k++)
        {
            printf("%s\n", inet_ntoa(addr_list[k].sin_addr)); //print all active hosts
        }*/
        
        usleep(100000 + rand() % 1610000); // waiting for an appropriate period of time
    }

    close(sockfd);
    return 0;
}
