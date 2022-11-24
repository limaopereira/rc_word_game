#include <stdio.h>
#include <string.h>

#define DEFAULT_PORT "58000" // + GN
#define MAX_COMMAND 10
#define MAX_PLID 6

int main(int argc, char **argv){
    char *ip, *port, command[MAX_COMMAND],plid[MAX_PLID];

    ip = argv[1];
    if(argv[2]==NULL)
        port = DEFAULT_PORT;
    else
        port=argv[2];
    
    printf("ip=%s port=%s\n",ip,port);

    scanf("%s",command);
    printf("command=%s\n",command);
    
    if(strcmp(command,"start")==0){
        scanf("%s",plid);
        printf("username=%s\n",plid);
    }
}