#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "constants.h"
#include "player_functions.h"


char GSIP[MAX_SIZE] = GS_DEFAULT_HOSTNAME;
char GSport[MAX_SIZE] = GS_DEFAULT_PORT;
char PLID[PLID_SIZE];
char word[MAX_WORD_SIZE];
int fd_socket_udp, fd_socket_tcp, trials, num_errors,max_errors;
struct addrinfo hints_udp, hints_tcp, *res_udp, *res_tcp; // Necessário declarar hints_udp aqui?
struct sockaddr_in addr_udp;
socklen_t addrlen_udp;


// player parsing

void parse_player_args(int argc, char **argv){
    if(argc == 1 || argc == 3 || argc == 5){
        for(int i=2; i<argc; i+=2){
            if(strcmp(argv[i-1],"-n")==0){
                if(valid_ip_address(argv[i])){
                    strcpy(GSIP, argv[i]);
                }
                else{
                    fprintf(stderr,"\n\nERROR: Invalid GS hostname/IP address. Please try again.\n\n\n");
                    exit(EXIT_FAILURE);
                }
            }
            else if(strcmp(argv[i-1],"-p")==0){
                if(valid_port(argv[i])){
                    strcpy(GSport,argv[i]);
                }
                else{
                    fprintf(stderr,"\n\nERROR: Invalid GS port. Please try again.\n\n\n");
                    exit(EXIT_FAILURE);
                }       
            }
            else{
                fprintf(stderr,"\n\nERROR: Invalid player application arguments.\nUsage: ./player [-n GSIP] [-p GSport]\n\n\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    else{
        fprintf(stderr,"\n\nERROR: Invalid player application arguments.\nUsage: ./player [-n GSIP] [-p GSport]\n\n\n"); 
        exit(EXIT_FAILURE);
    }
}


void parse_player_input(int *command, char* keyword){
    char line[MAX_SIZE], cmd_str[MAX_SIZE];
    int n;

    if(strlen(PLID)==0)
        printf("$ ");
    else
        printf("~/PLID/%s$ ",PLID);
    if(fgets(line,MAX_SIZE,stdin)){
        n = sscanf(line,"%s %s\n", cmd_str,keyword);
        if(valid_player_input(line,n)){
            *command=parse_player_command(cmd_str);
            return; // talvez melhorar?
        }
    }
    *command=INVALID_INPUT;
    //exit(EXIT_FAILURE);
    
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


// server parsing 

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
    else if(strcmp(status_str,"ACT")==0)
        return ACT;
    else if(strcmp(status_str,"FIN")==0)
        return FIN;
    else
        return ERR;
}


// player udp functions

void open_player_udp_socket(){
    int errcode;

    fd_socket_udp = socket(AF_INET,SOCK_DGRAM,0);
    if(fd_socket_udp==-1){
        fprintf(stderr,"\n\nERROR: Player UDP socket failed to create. Please try again.\n\n\n"); // create ou open?
        //exit(EXIT_FAILURE);
    }
        
    memset(&hints_udp,0,sizeof(hints_udp));
    hints_udp.ai_family=AF_INET;
    hints_udp.ai_socktype=SOCK_DGRAM;

    errcode = getaddrinfo(GSIP,GSport,&hints_udp,&res_udp);
    if(errcode!=0){
        close(fd_socket_udp);
        fprintf(stderr, "\n\nERROR: Failed on UDP address translation. Please try again\n\n\n"); // address translation? 
        //exit(EXIT_FAILURE);
    }

    addrlen_udp=sizeof(addr_udp); // existe algum problema em definir logo aqui?
}

void close_player_udp_socket(){
    freeaddrinfo(res_udp);
    close(fd_socket_udp);
    
}


int player_server_communication_udp(char *player_message, char *server_message){
    int n;
    
    open_player_udp_socket();
    // setsockopt para timers
    // outra opção para timers é SIGNALS ou usar o select
    n = sendto(fd_socket_udp,player_message,strlen(player_message),0,res_udp->ai_addr,res_udp->ai_addrlen);
    if(n==-1){
        //close_player_udp_socket();
        fprintf(stderr,"\n\nERROR: Failed to send UDP message. Please try again.\n\n\n"); 
        close_player_udp_socket();
        return -1;
        //exit(EXIT_FAILURE); //faz sentido fechar logo o socket e sair do programa?
    }
    else{
        set_read_timer(fd_socket_udp,MAX_TIME_READ_UDP);

        n = recvfrom(fd_socket_udp,server_message,MAX_SIZE,0,(struct sockaddr*)&addr_udp,&addrlen_udp);
        if(n==-1){ // tanto no sendto temos de verificar o tipo de erros possíveis
            fprintf(stderr,"\n\nERROR: Failed to receive UDP message. Please try again.\n\n\n"); 
            close_player_udp_socket();
            return -1;
        //close_player_udp_socket();
        //exit(EXIT_FAILURE); //faz sentido fechar logo o socket e sair do programa?
        }
        server_message[n]='\0';
    }
    close_player_udp_socket();
    return 0;   
}


// player tcp functions

void open_player_tcp_socket(){
    int errcode;

    fd_socket_tcp = socket(AF_INET,SOCK_STREAM,0);
    if(fd_socket_tcp == -1){
        fprintf(stderr, "\n\nERROR: Player TCP socket failed to create. Please try again.\n\n\n");
        //exit(EXIT_FAILURE);
    }
    memset(&hints_tcp,0,sizeof(hints_tcp));
    hints_tcp.ai_family=AF_INET;
    hints_tcp.ai_socktype=SOCK_STREAM;

    errcode = getaddrinfo(GSIP,GSport,&hints_tcp, &res_tcp);
    
}


void close_player_tcp_socket(){
    freeaddrinfo(res_tcp);
    close(fd_socket_tcp);
}


void player_server_communication_tcp(char *player_message, char **server_message_ptr){
    int n, bytes_readed, bytes_written, player_message_size;
    char buffer[512];

    open_player_tcp_socket();

    n=connect(fd_socket_tcp, res_tcp->ai_addr, res_tcp->ai_addrlen);
    if(n==-1){
        close_player_tcp_socket();
        fprintf(stderr, "\n\nERROR: Failed to connect to game server TCP socket. Please try again.\n\n\n");
        //exit(EXIT_FAILURE);
    }

    bytes_written = 0;
    player_message_size = strlen(player_message);
    while(bytes_written<player_message_size){
        n = write(fd_socket_tcp,player_message+bytes_written,player_message_size-bytes_written);
        bytes_written+=n;
    }
    //while((n=write(fd_socket_tcp,player_message,strlen(player_message)))>0);
    if(n==-1){
        close_player_tcp_socket();
        fprintf(stderr,"\n\nERROR: Failed to send TCP message. Please try again.\n\n\n");
        //exit(EXIT_FAILURE);
    }

    
    bytes_readed = 1;
    while((n=read(fd_socket_tcp,buffer,512))>0){
        bytes_readed+=n;
        *server_message_ptr = realloc(*server_message_ptr,bytes_readed);
        
        memcpy(*server_message_ptr+bytes_readed-n-1,buffer,n);
        //printf("response=%s\n",server_message);
    }
    (*server_message_ptr)[bytes_readed-1]='\0';
    if(n==-1){
        close_player_tcp_socket();
        fprintf(stderr, "\n\nERROR: Failed to receive TCP message. Please try again.\n\n\n");
    }

    close_player_tcp_socket();
}


//player commands functions 

void player_start_game(char *keyword){
    char player_message[MAX_SIZE], server_message[MAX_SIZE];

    if(!valid_plid(keyword))
        fprintf(stderr,"\n\nERROR: Invalid PLID. PLID needs to be a 6-digit IST student number. Please try again.\n\n\n");
    else{
        strcpy(PLID,keyword);
        sprintf(player_message,"SNG %s\n",PLID);
    }
    if(player_server_communication_udp(player_message,server_message)!=-1){
        parse_rsg(server_message);
    }
    
}


void player_play_letter(char *keyword){
    char player_message[MAX_SIZE], server_message[MAX_SIZE];
    if(!valid_letter(keyword))
        fprintf(stderr, "\n\nERROR: Invalid player play letter command keyword. Keyword needs to be a letter. Please try again.\n\n\n");
    trials++;
    sprintf(player_message,"PLG %s %s %d\n",PLID,keyword,trials);
    if(player_server_communication_udp(player_message,server_message)!=-1){
        parse_rlg(server_message,keyword[0]);
    }
    else
        trials--;    
}


void player_guess_word(char *keyword){
    char player_message[MAX_SIZE], server_message[MAX_SIZE];
    if(!valid_word(keyword))
        fprintf(stderr, "\n\nERROR: Invalid player guess word command. Keyword needs to be a word. Please try again.\n\n\n");
    trials++;
    sprintf(player_message,"PWG %s %s %d\n",PLID,keyword,trials);
    if(player_server_communication_udp(player_message,server_message)!=-1){
        parse_rwg(server_message,keyword);
    }
    else
        trials--;
}


void player_get_scoreboard(){ // falta fazer free na memoria alocada
    char player_message[MAX_SIZE], *server_message;
    server_message=NULL;
    //server_message_ptr = &server_message;
    sprintf(player_message,"GSB\n");
    player_server_communication_tcp(player_message,&server_message);
    parse_rsb(server_message); // provavelmente não podemos fazer isto assim, não sabemos inicialmente o tamanho máximo da resposta
    //printf("server_message=%s",server_message);
    free(server_message);
}


void player_get_hint(){
    char player_message[MAX_SIZE], *server_message;
    server_message = NULL;
    //server_message_ptr = &server_message;
    sprintf(player_message,"GHL %s\n", PLID);
    player_server_communication_tcp(player_message,&server_message);
    parse_rhl(server_message); 
    free(server_message);
}


void player_get_state(){
    char player_message[MAX_SIZE], *server_message;
    server_message = NULL;
    //server_message_ptr = &server_message;
    sprintf(player_message,"STA %s\n",PLID);
    player_server_communication_tcp(player_message, &server_message);
    parse_rst(server_message);
    free(server_message);
}


void player_quit_game(){
    char player_message[MAX_SIZE], server_message[MAX_SIZE];
    sprintf(player_message,"QUT %s\n",PLID);
    player_server_communication_udp(player_message,server_message);
    parse_rqt(server_message);
}


void player_exit_app(){
    player_quit_game();
    printf("Closing application...\n");
    exit(EXIT_SUCCESS);
}






// player start game auxiliary functions


void parse_rsg(char *response){ //talvez inserir no functions?
    char status_str[4];
    int i, num_keys, word_size, bytes_readed,status, spaces;

    spaces = count_spaces(response);
    num_keys = sscanf(response,"RSG %s %d %d %n\n", status_str,&word_size,&max_errors,&bytes_readed);
    if(spaces!=num_keys || (num_keys!=1 && num_keys!=3))
        status = ERR;
    else if(response[strlen(response)-1]!='\n')
        status = ERR;
    else
        status = parse_server_status(status_str);
    switch (status){
        case OK:
            if(num_keys==1)
                fprintf(stderr,"\n\nERROR: Invalid game server response. Please try again.\n\n\n");    
            else{
                for(i=0;i<word_size;i++)
                    word[i]='_';
                word[i]='\0';
                trials = 0;
                num_errors = 0;
                printf("\n\nNew Game Successfully Started!\n\nWord:%s\nCurrent trial:%d\nNumber of Errors:%d/%d\n\n",word,trials+1,num_errors,max_errors);
            }
            break;
        
        case NOK:
            if(num_keys==3)
                fprintf(stderr,"\n\nERROR: Invalid game server response. Please try again.\n\n\n");
            else            
                fprintf(stderr,"\n\nERROR: Player %s already has a game in progress\n\n\n",PLID);
            break;
        case ERR:
            fprintf(stderr,"\n\nERROR: Invalid game server response. Please try again.\n\n\n");
            break;     
    }
}


// player play letter auxiliary functions

void parse_rlg(char *response, char letter){
    char status_str[4];
    int status,trial, num_keys, num_letters, bytes_readed, pos;
    num_keys = sscanf(response,"RLG %s %d %d %n",status_str,&trial,&num_letters,&bytes_readed);
    if(valid_server_response(response)){
        status = parse_server_status(status_str);
        switch (status){
            case OK:
                for(int i=0;i<num_letters;i++){
                    response+=bytes_readed;
                    sscanf(response,"%d %n",&pos,&bytes_readed);
                    word[pos-1] = letter;
                }
                printf("\n\nYes, '%c' is part of the word\n",letter);
                printf("\nWord:%s\nCurrent trial:%d\nNumber of Errors:%d/%d\n\n",word,trials+1,num_errors,max_errors);
                break;
            case WIN:
                for(int i=0;i<strlen(word);i++){
                    if(word[i]=='_')
                        word[i]=letter;
                }
                printf("\n\nWIN!\n");
                printf("\nWord:%s\nCurrent trial:%d\nNumber of Errors:%d/%d\n\n",word,trials+1,num_errors,max_errors);
                break;
            case DUP:
                trials--;
                printf("\n\nAlready guessed '%c' letter. Please try again with another letter.\n",letter);
                printf("\nWord:%s\nCurrent trial:%d\nNumber of Errors:%d/%d\n\n",word,trials+1,num_errors,max_errors);
                break;
            case NOK:
                printf("\n\nNo, '%c' is not part of the word\n",letter);
                printf("\nWord:%s\nCurrent trial:%d\nNumber of Errors:%d/%d\n\n",word,trials+1,++num_errors,max_errors);
                
                break;
            case OVR:
                printf("\n\nGAME OVER!\n");
                printf("\nWord:%s\nCurrent trial:%d\nNumber of Errors:%d/%d\n\n",word,trials+1,++num_errors,max_errors);
                break;
            case INV:
                printf("\n\nInvalid trial number\n");
                printf("\nWord:%s\nCurrent trial:%d\nNumber of Errors:%d/%d\n\n",word,trials+1,num_errors,max_errors);
                break;
            case ERR:
                fprintf(stderr,"\n\nERROR: Invalid game server response. Please try again.\n\n\n"); // talvez arranjar um forma de especificar este erro
                //printf("Word:%s\nCurrent trial:%d\nNumber of Errors:%d/%d\n\n",word,trials+1,num_errors,max_errors);
                break;                
        }  
    }
    else
        fprintf(stderr,"\n\nERROR: Invalid game server response. Please try again.\n\n\n");
}         


// player guess word auxiliary functions

void parse_rwg(char *response, char *word_guessed){
    char status_str[4];
    int status, trial, num_keys, bytes_readed;
    
    num_keys = sscanf(response, "RWG %s %d %n\n",status_str,&trial,&bytes_readed);

    if(valid_server_response(response)){
        status = parse_server_status(status_str);
        switch (status){
            case WIN:
                printf("\n\nWIN!\n");
                printf("\nWord:%s\nCurrent trial:%d\nNumber of Errors:%d/%d\n\n",word_guessed,trials+1,num_errors,max_errors);
                break;
            case DUP:
                printf("\n\nAlready guessed '%s'. Please try again with another word.\n",word_guessed);
                printf("\nWord:%s\nCurrent trial:%d\nNumber of Errors:%d/%d\n\n",word,trials+1,num_errors,max_errors);
                break;
            case NOK:
                printf("\n\nNo, '%s' is not the correct word.\n",word_guessed);
                printf("\nWord:%s\nCurrent trial:%d\nNumber of Errors:%d/%d\n\n",word,trials+1,++num_errors,max_errors);
                break;
            case OVR:
                printf("\n\nGAME OVER\n");
                printf("\nWord:%s\nCurrent trial:%d\nNumber of Errors:%d/%d\n\n",word,trials+1,++num_errors,max_errors);
                break;
            case INV:
                printf("\n\nInvalid trial number\n"); // não tenho a certeza se só acontece quando o trial number não é o correcto
                printf("\nWord:%s\nCurrent trial:%d\nNumber of Errors:%d/%d\n\n",word,trials+1,num_errors,max_errors);
                break;
            case ERR:
                fprintf(stderr,"\n\nERROR: Invalid game server response. Please try again.\n\n\n");
                break;
        }
    }
    else
        fprintf(stderr,"\n\nERROR: Invalid game server response. Please try again.\n\n\n");
}


// player get scoreboard auxiliary functions

void parse_rsb(char *response){
    const char s[2]=" ";
    char status_str[6],fname[24], *fdata;
    char score_sb[4],plid_sb[PLID_SIZE],word_sb[MAX_WORD_SIZE],trial_sb[3],size_sb[3];
    int status, fsize, num_keys, bytes_readed, line, total_bytes_readed;
    FILE *fp = NULL;

    num_keys = sscanf(response, "RSB %s %n\n",status_str, &bytes_readed);
    if(valid_server_response(response)){ // pensar melhor nesta verificação
        status = parse_server_status(status_str);
        switch (status){
            case OK:
                response+=bytes_readed;
                sscanf(response, "%s %d %n",fname,&fsize,&bytes_readed);
                fp = fopen(fname,"w");
                total_bytes_readed = fsize;
                while(fsize>0){
                    response += bytes_readed;
                    bytes_readed = fwrite(response,1,fsize,fp);
                    printf("\n\n%.*s\n", bytes_readed, response);
                    fsize -= bytes_readed;
                }
                fclose(fp);
                printf("Received scoreboard file: %s (%d bytes)\n\n\n",fname,total_bytes_readed);
                break;
            case EMPTY:
                printf("Scoreboard is still empty!\n\n\n");
                break;
        }
    }
    else
        fprintf(stderr,"\n\nERROR: Invalid game server response. Please try again.\n\n\n");
}


// player get hint auxiliary functions

void parse_rhl(char *response){
    char status_str[4], fname[24];
    int status,fsize, num_keys, bytes_readed, total_bytes_readed;
    FILE *fp = NULL;

    num_keys = sscanf(response, "RHL %s %n\n", status_str, &bytes_readed);
    if(valid_server_response(response)){
        status = parse_server_status(status_str);
        switch (status){
            case OK:
                response += bytes_readed;
                sscanf(response, "%s %d %n\n",fname,&fsize,&bytes_readed);
                fp = fopen(fname,"w");
                total_bytes_readed = fsize;
                while(fsize>0){ // falta verificar erros?
                    response += bytes_readed;
                    bytes_readed = fwrite(response,1,fsize,fp);
                    fsize-=bytes_readed;
                }
                fclose(fp);
                printf("\n\nReceived hint file: %s (%d bytes)\n\n\n",fname,total_bytes_readed);
                break;
            case NOK:
                printf("There is no hint file to send.\n");
                break;
        }
    }
    else
        fprintf(stderr,"\n\nERROR: Invalid game server response. Please try again.\n\n\n");
}


// player get state auxiliary functions 

void parse_rst(char *response){
    char status_str[4], fname[24];
    int status, fsize, num_keys, bytes_readed, total_bytes_readed;
    FILE *fp = NULL;

    num_keys = sscanf(response, "RST %s %n\n", status_str, &bytes_readed);
    if(valid_server_response(response)){
        status = parse_server_status(status_str);
        switch(status){// pensar melhor para evitar repetir código
            case ACT:
            case FIN:
                response += bytes_readed;
                sscanf(response, "%s %d %n\n",fname,&fsize,&bytes_readed);
                fp = fopen(fname,"w");
                total_bytes_readed = fsize;
                while(fsize>0){
                    response += bytes_readed;
                    bytes_readed = fwrite(response,1,fsize,fp);
                    printf("\n\n%.*s\n", bytes_readed, response);
                    fsize -= bytes_readed;
                }
                fclose(fp);
                printf("Received state file: %s (%d bytes)\n\n\n", fname, total_bytes_readed);
                break;
            case NOK:
                printf("\n\nThere is no active or finished games for Player %s\n\n\n",PLID);
                break;
        }
    }
    else{
        fprintf(stderr,"\n\nERROR: Invalid game server response. Please try again.\n\n\n");
    }

}


// player quit game auxiliary functions

void parse_rqt(char *response){
    char status_str[4];
    int status, num_keys, bytes_readed;

    num_keys = sscanf(response, "RQT %s %n\n",status_str, &bytes_readed);
    if(valid_server_response(response)){
        status = parse_server_status(status_str);
        switch (status){
        case OK:
            printf("\n\nCurrent Game Terminated!\n\n\n");
            break;
        case NOK:
            printf("\n\nThere is no ongoing game.\n\n\n");
            break;
        case ERR:
            fprintf(stderr,"\n\nERROR: Invalid game server response. Please try again.\n\n\n");
            break;
        }
    }
    else{
        fprintf(stderr,"\n\nERROR: Invalid game server response. Please try again.\n\n\n");
    }
}


// validation functions

int valid_ip_address(char *ip_address){
    char *ptr;
    int num, dots = 0;

    ptr = strtok(ip_address, ".");
    if (ptr == NULL) 
        return 0;

    while (ptr) {
        num = atoi(ptr);
        if (num < 0 || num > 255) 
            return 0;
        ptr = strtok(NULL, ".");
        dots++;
        if (dots != 4) 
            return 0;
    }
    return 1;
}


int valid_port(char *port_str){
    int port = atoi(port_str);
    if(port > 0 && port < 65536)
        return 1;
    return 0;
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

    
// player global auxiliary functions 


int count_spaces(char *response){
    int spaces = 0;
    for(int i=0;i<strlen(response);i++){
        if(response[i]==' ')
            spaces++;
    }
    return spaces;
}


void set_read_timer(int fd_socket, int time){
    struct timeval timeout;
    
    timeout.tv_sec = time;
    timeout.tv_usec = 0;
    if(setsockopt(fd_socket,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout))==-1){
        // fazer o que?
    }
}

void introduction_text(){
    char welcome[]="\n\nWelcome to Hangman! In this game, you will try to guess the secret\nword by suggesting letters.You have a limited number of\nchances, so you need to be careful not to make too many mistakes.\n\n\n";
    char available_commands[]="COMMAND                    KEYWORD                     FUNCTION         \nstart/sg - - - - - - - - - -PLID - - - - - - - - - - Starts a New Game  \nplay/pl - - - - - - - - - -letter - - - - - - - - - -Guess a Letter     \nguess/gw - - - - - - - - - -word - - - - - - - - - - Guess a word       \nscoreboard/sb - - - - - - - - - - - - - - - - - - - -Get Scoreboard     \nhint/h - - - - - - - - - - - - - - - - - - - - - - - Get Word Hint      \nstate/st - - - - - - - - - - - - - - - - - - - - - - Get Game State     \nquit - - - - - - - - - - - - - - - - - - - - - - - - Quit Game          \nexit - - - - - - - - - - - - - - - - - - - - - - - - Exit Application   \n\n\n\n";
    printf("%s%s",welcome,available_commands);
}