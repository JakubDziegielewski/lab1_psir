#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <termios.h>
#include <fcntl.h>

#define SERVER "192.168.89.3"
#define PORT "19370"
#define MAXDATASIZE 256

int sockfd;
int rv;
unsigned char my_str[MAXDATASIZE];
struct addrinfo hints, *res, *p;
struct sockaddr_in c;
int c_len = sizeof(c);
int kbhit(void) //method that detects keyboard interruption, returns 1 when a key is hit
        {
          struct termios oldt, newt;
          int ch;
          int oldf;

          tcgetattr(STDIN_FILENO, &oldt);
          newt = oldt;
          newt.c_lflag &= ~(ICANON | ECHO);
          tcsetattr(STDIN_FILENO, TCSANOW, &newt);
          oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
          fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

          ch = getchar();

          tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
          fcntl(STDIN_FILENO, F_SETFL, oldf);

          if(ch != EOF)
          {
            ungetc(ch, stdin);
            return 1;
          }

          return 0;
        }
int main(int argc, char *argv[])
{
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    snprintf(my_str, MAXDATASIZE, "Hello");

    if ((rv = getaddrinfo(SERVER, PORT, &hints, &res)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for (p = res; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("client: socket");
            continue;
        }

        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "client:failed to create socket\n");
        return 2;
    }


    freeaddrinfo(res);
    int pos;
    if ((pos = sendto(sockfd, my_str, strlen(my_str), 0, p->ai_addr, p->ai_addrlen)) == -1) //sending first message
    {
        perror("client: sendto");
        exit(1);
    }
    while (!kbhit())
    {

        if ((pos = recvfrom(sockfd, my_str, MAXDATASIZE, 0, (struct sockaddr *)&c, &c_len)) == -1) //recieving a message
        {
            perror("Recvfrom");
            exit(-4);
        }
        my_str[pos] = '\0';
        printf("Recv(%s): %s\n", inet_ntoa(c.sin_addr), my_str);
        char response[MAXDATASIZE] = "Re: ";
        strcat(response, my_str);
        if ((pos = sendto(sockfd, response, strlen(response), 0, p->ai_addr, p->ai_addrlen)) == -1) //sending a response
        {
            perror("client: sendto");
            exit(1);
        }
    }
    if ((pos = sendto(sockfd, " ", 1, 0, p->ai_addr, p->ai_addrlen)) == -1)
    {
        perror("client: sendto");
        exit(1);
    }
    close(sockfd);
}