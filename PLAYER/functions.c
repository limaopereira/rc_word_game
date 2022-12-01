#include <string.h>
#include <stdio.h>

#include "constants.h"
#include "functions.h"


int valid_ip_address(char *ip_address){
    return 1;
}

int valid_port(char *port){
    return 1;
}

int valid_player_input(char* string, int num_cmds){
    int size = strlen(string);
    int spaces = 0;
    int diff;

    for(int i = 0; i<size;i++)
        if(string[i]==' ')
            spaces++;


    diff = num_cmds - spaces -1;
    if(num_cmds > 0 && diff==0)
        return 1;
    return 0;
}

int valid_server_response(char *response){
    return 1; // falta validar a resposta do servidor
}


int parse_player_command(char *cmd_str){
    if(strcmp(cmd_str,"start")==0 || strcmp(cmd_str,"sg")==0)
        return START;
    else if(strcmp(cmd_str,"play")==0 || strcmp(cmd_str,"pl")==0)
        return PLAY;
    else if(strcmp(cmd_str,"guess")==0 || strcmp(cmd_str,"gw")==0)
        return GUESS;
    else if(strcmp(cmd_str,"scoreboard")==0 || strcmp(cmd_str,"sb")==0)
        return SCOREBOARD;
    else if(strcmp(cmd_str,"hint")==0 || strcmp(cmd_str,"h")==0)
        return HINT; 
    else if(strcmp(cmd_str,"state")==0 || strcmp(cmd_str,"st")==0)
        return STATE;
    else if(strcmp(cmd_str,"quit")==0)
        return QUIT;
    else if(strcmp(cmd_str,"exit")==0)
        return EXIT;
    else
        return INVALID_CMD;
}

int parse_server_status(char *status_str){
    if(strcmp(status_str,"OK")==0)
        return OK;
    else if(strcmp(status_str,"WIN")==0)
        return WIN;
    else if(strcmp(status_str,"DUP")==0)
        return DUP;
    else if(strcmp(status_str,"NOK")==0)
        return NOK;
    else if(strcmp(status_str,"OVR")==0)
        return OVR;
    else if(strcmp(status_str,"INV")==0)
        return INV;
    else if(strcmp(status_str,"EMPTY")==0)
        return EMPTY;
    else
        return ERR;
}

int valid_plid(char *keyword){
    int size = strlen(keyword);
    if(size == 6){
        for(int i=0;i<size;i++){
            if(!(keyword[i]>='0' && keyword[i]<='9'))
                return 0;
        }
        return 1;
    }
    return 0;
}

int valid_letter(char *keyword){
    return 1;
}

int valid_word(char *keyword){
    return 1;
}


