#ifndef UDP_CLIENT_H
#define UDP_CLIENT_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

int open_udp(char *ip, char *port,int *socket_fd, struct addrinfo **res );
int udp_player_server_com(char* ip, char* port, char* player_message, char* server_response);


#endif