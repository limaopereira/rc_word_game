#include <stdio.h>
#include <string.h>
#include "udp_client.h"

#define DEFAULT_PORT "58000" // + GN
#define MAX_LINE 37
#define MAX_COMMAND 10
#define MAX_WORD 30
#define ERROR_MESSAGE_COMMAND "ERROR COMMAND\n"
#define ERROR_MESSAGE_PLID "ERROR PLID\n"
#define ERROR_MESSAGE_PLAYER_UDP "ERROR PLAYER UDP"


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
    char *ip, *port, line[MAX_LINE],command[MAX_COMMAND],command_input[MAX_LINE];
    const char s[2] =" ";

    ip=NULL;
    port=DEFAULT_PORT;

    for(int i=2;i<argc;i++){    
        if(strcmp(argv[i-1],"-n")==0)
            ip=argv[i];
        else if(strcmp(argv[i-1],"-p")==0)
            port=argv[i];
    }

    printf("ip=%s port=%s\n",ip,port);

    while(read_command(command,command_input)!=-1){
        if(strcmp(command,"start")==0 || strcmp(command,"sg")==0){
            char server_response[128], player_message[12];
    
            printf("PLID=%s\n", command_input);
            if(check_plid(command_input)==-1){
                printf(ERROR_MESSAGE_PLID);
                // Falta tratar os erros
            }
            
            strcpy(player_message,"SNG ");
            strcat(player_message,command_input);
            player_message[10]='\n';

            if(udp_player_server_com(ip,port,player_message,server_response)==-1){
                printf(ERROR_MESSAGE_PLAYER_UDP);
            }
            
        }
    }
}