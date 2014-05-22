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
int clients_size=0;
int clients[1000];
int isOnline[1000];
char names[1000][128];

pthread_t chat;
pthread_t server_exit;

int checkName(const char *newName)
{
    for(int i=0;i<clients_size;i++)
        if(isOnline[i] && strcmp(names[i], newName)==0)
            return i;
    return -1;
}

void sendAll(const char* msg,int exception)
{
    if(exception==-1)
    {
        for(int i=0;i<clients_size;i++)
            if(isOnline[i] == 1)
                send(clients[i], msg, strlen(msg)+1, 0);
    }
    else
        for(int i=0;i<clients_size;i++)
            if(isOnline[i] && i!=exception)
                send(clients[i], msg, strlen(msg)+1, 0);
        
}

void* serverExit (void* arg)
{
    char msg[10];
    char ex[] = "/exit";
    while(1)
    {
        gets(msg);
        if(strcmp(msg,ex) == 0)
        {
            sendAll("/exit",-1);
            for(int i=0; i<clients_size;i++)
                close(clients[i]);
            close(sock);
            exit(0);
        }
    }
   
}

void* chatting(void* arg)
{
    while(1)
    {
        usleep(50000);
        int newClient;
        newClient=accept(sock, NULL, NULL);
        if(newClient!=-1)
        {
            int free=-1;
            for(int i=0;i<clients_size;i++)
                if(isOnline[i]==0)
                {
                    free=i;
                    break;
                }
            if(free==-1)
            {
                clients_size++;
                free=clients_size-1;
            }
            isOnline[free]=1;
            clients[free]=newClient;
            fcntl(newClient, F_SETFL, O_NONBLOCK);
            sprintf(names[free],"Client_%d",free);
            char msg[100];
            sprintf(msg,"%s joined!",names[free]);
            
            sendAll(msg,-1);
        }
        char buf[1024];
        int bytes_read;
        for(int i=0; i<clients_size;i++)
        {
            bytes_read = recv(clients[i], buf, 100, 0);
            if(bytes_read > 0)
            {
                buf[bytes_read] = '\0';
                if(strcmp(buf, "/exit")==0)
                {
                    char msg[100];
                    sprintf(msg,"%s left",names[i]);
                    sendAll(msg,i);
                    isOnline[i] = 0;
                    continue;
                }
                if(buf[0]!='/')
                {
                    char msg[1024];
                    sprintf(msg,"%s: %s",names[i],buf);
                    sendAll(msg,i);
                    continue;
                }
                char command[5];
                memcpy(command, &buf[1], 4);
                command[4]='\0';
                if(strcmp(command, "name")==0)
                {
                    char newName[128];
                    memcpy(newName, &buf[6], strlen(buf)-6);
                    if(checkName(newName)!=-1)
                        send(clients[i], "Name is already taken" , 21, 0);
                    else
                    {
                        char msg[100];
                        sprintf(msg, "%s changed name to %s", names[i], newName);
                        sendAll(msg,i);
                        strcpy(names[i], newName);
                    }
                    continue;
                }
                if(strcmp(command, "msg ")==0)
                {
                    char receiver[128];
                    char msg[1024];
                    int j=5;
                    while(buf[j]!=' ' && buf[j]!='\n')
                        j++;
                    memcpy(receiver, &buf[5], j-5);
                    receiver[j-5]='\0';
                    
                    int k=j;
                    while(buf[k]!='\0')
                        k++;
                    memcpy(msg, &buf[j+1], k-j-1);
                    msg[k-j-1]='\0';
                    int receiverIndex=-1;
                    receiverIndex=checkName(receiver);
                    if(receiverIndex==-1)
                    {
                        char msg[] = "No such user online";
                        send(clients[i], msg, sizeof(msg), 0);
                        continue;
                    }
                    char str[1024];
                    strcpy(str, names[i]);
                    strcat(str, " (private): ");
                    strcat(str, msg);
                    send(clients[receiverIndex], str, strlen(str)+1, 0);
                }
            }
        }
    }
}

int main()
{
    for(int i=0; i<1000; i++)
        isOnline[i] = 0;
    
    sock=socket(AF_INET, SOCK_STREAM, 0);
    
    if (sock==-1)
    {
        printf("Error: socket cannot be initialized\n");
        exit(1);
    }
    
    struct sockaddr_in saddr;
    saddr.sin_family=AF_INET;
    saddr.sin_port=htons(3425);
    saddr.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    if (bind(sock, (struct sockaddr *) &saddr, sizeof(saddr)) < 0)
    {
        perror("Error: binding impossible\n");
        exit(2);
    }
    listen(sock, 20);
    fcntl(sock, F_SETFL, O_NONBLOCK);
    
    pthread_create(&server_exit, NULL, serverExit, NULL);
    pthread_create(&chat, NULL, chatting, NULL);
    pthread_join(server_exit, NULL);
    pthread_join(chat, NULL);
    return 0;
}
