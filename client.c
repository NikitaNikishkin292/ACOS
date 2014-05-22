#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <fcntl.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>

int sock;
pthread_t sending;
pthread_t receiving;

void* Send()
{
    char msg[1024];
    while(1)
    {
        usleep(50000);
        gets(msg);
        if(strcmp(msg, "/exit")==0)
        {
            send(sock, msg, 5, 0);
            close(sock);
            exit(0);
        }
        if(msg[0] != '\0')
            send(sock, msg, 100, 0);
    }
}

void* Receive()
{
    char buf[1024];
    int bytes_read;
    while(1)
    {
        usleep(50000);
        bytes_read = recv(sock, buf, 100, 0);
        if(bytes_read > 0)
        {
            if(strcmp(buf, "/exit") == 0)
            {
                puts("Server is closing");
                close(sock);
                exit(0);
            }
            buf[bytes_read] = '\0';
            printf("%s\n", buf);
        }
    }
}

int main()
{
    sock = socket(AF_INET, SOCK_STREAM, 0);
    
    if (sock == -1)
    {
        printf("Error: socket cannot be initialized\n");
        exit(1);
    }
    struct sockaddr_in caddr;
    caddr.sin_family = AF_INET;
    caddr.sin_port = htons(3425);
    caddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (connect(sock, (struct sockaddr *) &caddr, sizeof(caddr)) < 0)
    {
        perror("Error: binding impossible\n");
        exit(2);
    }
    fcntl(sock, F_SETFL, O_NONBLOCK);

    pthread_create(&sending, NULL,Send, NULL);
    pthread_create(&receiving, NULL, Receive, NULL);
    pthread_join(sending, NULL);
    pthread_join(receiving, NULL);
  
    close(sock);
    return 0;
}
