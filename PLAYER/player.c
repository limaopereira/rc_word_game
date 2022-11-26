#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "udp_client.h"

#define DEFAULT_PORT "58000" // + GN
#define MAX_LINE 37
#define MAX_COMMAND 10
#define MAX_PLID 7
#define MAX_WORD 31
#define ERROR_MESSAGE_COMMAND "ERROR COMMAND\n"
#define ERROR_MESSAGE_PLID "ERROR PLID\n"
#define ERROR_MESSAGE_SENDTO "ERROR SENDTO UDP\n"
#define ERROR_MESSAGE_RECVFROM "ERROR RECVFROM UDP\n"

int parse_rsg(char *response, char *word){
    char status[4];
    int i,number_commands,word_size,max_errors;
    
    number_commands=sscanf(response,"RSG %s %d %d",status,&word_size,&max_errors);
    if(number_commands!=1 && number_commands!=3)
        return -1;
    for(i=0; i<word_size;i++){
        word[i]='_';
    }
    word[i]='\0';
    printf("New game started (max %d errors): %s\n",max_errors,word);
}


int count_spaces(char* string){
    int size = strlen(string);
    int spaces = 0;

    for(int i = 0; i<size;i++)
        if(string[i]==' ')
            spaces++;

    return spaces;
}

int check_plid(char* string){
    int size = strlen(string);
    if(size==6){
        for(int i=0;i<size;i++)
            if(!(string[i]>='0' && string[i]<='9'))
                return -1;
        return 0;
    }
    return -1;
}


int read_command(char* command, char* command_input){
    char line[MAX_LINE];
    int number_commands, number_spaces, diff_commands_spaces;

    if(fgets(line,sizeof(line),stdin)){
        number_commands = sscanf(line,"%s %s",command,command_input);
        number_spaces = count_spaces(line);
        diff_commands_spaces = number_commands-number_spaces-1;
        if(number_commands > 0 &&  diff_commands_spaces==0)
            return 0;    
    }
    return -1;
}


int main(int argc, char **argv){
    int trial,n,fd;
    char *ip, *port, line[MAX_LINE],command[MAX_COMMAND],command_input[MAX_LINE],plid[MAX_PLID],word[MAX_WORD];
    const char s[2] =" ";
    struct sockaddr_in addr;
    struct addrinfo *res;
    socklen_t addrlen;

    ip=NULL;
    port=DEFAULT_PORT;

    for(int i=2;i<argc;i++){    
        if(strcmp(argv[i-1],"-n")==0)
            ip=argv[i];
        else if(strcmp(argv[i-1],"-p")==0)
            port=argv[i];
    }

    printf("ip=%s port=%s\n",ip,port);


    trial = 0;
    while(read_command(command,command_input)!=-1){
        if(strcmp(command,"start")==0 || strcmp(command,"sg")==0){
            char server_response[128], player_message[12];
            
            if(check_plid(command_input)==-1){
                printf(ERROR_MESSAGE_PLID);
                // Falta tratar os erros
            }

            strcpy(plid,command_input);
            sprintf(player_message,"SNG %s\n",plid);

            open_udp(ip,port,&fd,&res);

            n=sendto(fd,player_message,strlen(player_message),0,res->ai_addr,res->ai_addrlen);
            if(n==-1){
                printf(ERROR_MESSAGE_SENDTO);
            }

            addrlen=sizeof(addr);
            n=recvfrom(fd,server_response,sizeof(server_response),0,(struct sockaddr*)&addr,&addrlen);
            if(n==-1){
                printf(ERROR_MESSAGE_RECVFROM);
            }
            parse_rsg(server_response,word);
        }
        else if(strcmp(command,"play")==0 || strcmp(command,"pl")==0){
            char server_response[128], player_message[128];
            trial++;
            sprintf(player_message,"PLG %s %s %d\n",plid,command_input,trial);

            n=sendto(fd,player_message,strlen(player_message),0,res->ai_addr,res->ai_addrlen);
            if(n==-1){
                printf(ERROR_MESSAGE_SENDTO);
            }

            addrlen=sizeof(addr);
            n=recvfrom(fd,server_response,sizeof(server_response),0,(struct sockaddr*)&addr,&addrlen);
            if(n==-1){
                printf(ERROR_MESSAGE_RECVFROM);
            }
        }
        else if(strcmp(command,"guess")==0 || strcmp(command,"gw")==0){
            char server_response[128], player_message[128];
            trial++;
            sprintf(player_message,"PWG %s %s %d\n",plid,command_input,trial);

            n=sendto(fd,player_message,strlen(player_message),0,res->ai_addr,res->ai_addrlen);
            if(n==-1){
                printf(ERROR_MESSAGE_SENDTO);
            }

            addrlen=sizeof(addr);
            n=recvfrom(fd,server_response,sizeof(server_response),0,(struct sockaddr*)&addr,&addrlen);
            if(n==-1){
                printf(ERROR_MESSAGE_RECVFROM);
            }
        }
    }
}