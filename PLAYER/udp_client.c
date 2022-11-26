#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "udp_client.h"


int open_udp(char *ip, char *port,int *socket_fd,struct addrinfo **res){
    int errcode;
    struct sockaddr_in addr;
    struct addrinfo hints;
    socklen_t addrlen;
    ssize_t n;
    char host[NI_MAXHOST], service[NI_MAXSERV];

    /*...*///see previous task code
    *socket_fd=socket(AF_INET,SOCK_DGRAM,0);//UDP socket
    if(*socket_fd==-1)/*error*/return -1;

    memset(&hints,0,sizeof hints);
    hints.ai_family=AF_INET;//IPv4
    hints.ai_socktype=SOCK_DGRAM;//UDP socket

    errcode=getaddrinfo(ip,port,&hints,&(*res));
    if(errcode!=0)/*error*/return -1;
}


int udp_player_server_com(char* ip, char* port, char* player_message, char* server_response){
    int fd,errcode;
    struct sockaddr_in addr;
    struct addrinfo hints,*res;
    socklen_t addrlen;
    ssize_t n;
    char host[NI_MAXHOST], service[NI_MAXSERV];


    /*...*///see previous task code
    fd=socket(AF_INET,SOCK_DGRAM,0);//UDP socket
    if(fd==-1)/*error*/return -1;

    memset(&hints,0,sizeof hints);
    hints.ai_family=AF_INET;//IPv4
    hints.ai_socktype=SOCK_DGRAM;//UDP socket

    errcode=getaddrinfo(ip,port,&hints,&res);
    if(errcode!=0)/*error*/return -1;

    n=sendto(fd,player_message,strlen(player_message),0,res->ai_addr,res->ai_addrlen);
    if(n==-1)/*error*/return -1;

    write(1,"sended: ",9);
    write(1,player_message,n);
    
    addrlen=sizeof(addr);
    n=recvfrom(fd,server_response,sizeof(server_response),0,(struct sockaddr*)&addr,&addrlen);
    if(n==-1)/*error*/return -1;
    
    write(1,"received: ",10);//stdout
    write(1,server_response,n);
 
    if((errcode=getnameinfo((struct sockaddr *)&addr,addrlen,host,sizeof host,service,sizeof service,0))!=0)
        fprintf(stderr,"error: getnameinfo: %s\n",gai_strerror(errcode));
    else
        printf("sent by [%s:%s]\n",host,service);

    freeaddrinfo(res);
    close(fd);

}