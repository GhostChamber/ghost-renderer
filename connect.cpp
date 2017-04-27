#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>


#define MYPORT 4000
#define MAXBUFLEN 512

int reply(const char* client);

int main(void)
{
    int sockfd;
    struct sockaddr_in my_addr;        // my address information
    struct sockaddr_in their_addr;    // connector's address information
    socklen_t addr_len;
    int numbytes;
    char buf[MAXBUFLEN];


    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    my_addr.sin_family = AF_INET;             // host byte order
    my_addr.sin_addr.s_addr = INADDR_ANY;     // automatically fill with my IP
    my_addr.sin_port = htons(MYPORT);         // short, network byte order
    memset(&(my_addr.sin_zero), '\0', 8);     // zero the rest of the struct

    if (bind(sockfd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr)) == -1)
    {
        perror("bind");
        exit(1);
    }

    addr_len = sizeof(struct sockaddr);

    while(true)
    {
        if ((numbytes = recvfrom(sockfd, buf, (MAXBUFLEN - 1), 0, (struct sockaddr *)&their_addr, &addr_len)) == -1)
        {
            perror("recvfrom");
            exit(1);
        }

        buf[numbytes] = '\0';
        if (strcmp("GHOST-CONTROLLER", buf) == 0)
        {
            
            printf("got connection request from %s\n", inet_ntoa(their_addr.sin_addr));
            /*numbytes = sendto(sockfd,inet_ntoa(my_addr.sin_addr), strlen(inet_ntoa(my_addr.sin_addr)), 0, (struct sockaddr *)&their_addr, addr_len);
            if (numbytes  < 0)
            {
                perror("sendto");
            }*/
            reply(inet_ntoa(their_addr.sin_addr));
        }
        else
        {
            printf("got packet from %s\n", inet_ntoa(their_addr.sin_addr));
            printf("packet is %d bytes long\n", numbytes);
            printf("packet contains \"%s\"\n",buf);
        }
    }

    close(sockfd);
    return 0;
}


int reply(const char* client)
{
    int sock, n;
    socklen_t length;
    struct sockaddr_in server;
    struct hostent *hp;
    char buffer[256];

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) perror("socket");

    server.sin_family = AF_INET;
    hp = gethostbyname(client);
    if (hp==0) perror("Unknown host");

    bcopy((char *)hp->h_addr, (char *)&server.sin_addr, hp->h_length);
    server.sin_port = htons(MYPORT + 1);
    length = sizeof(struct sockaddr_in);

    strcpy(buffer, "SERVER ACTIVE");
    printf("Sending message to %s\n", client);
    n = sendto(sock, buffer, strlen(buffer), 0, (const struct sockaddr *)&server, length);
    if (n < 0) perror("Sendto");

    close(sock);
    return 0;
}