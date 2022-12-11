#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "constants.h"
#include "functions.h"
#include "player_functions.h"



char GSIP[MAX_SIZE] = GS_DEFAULT_HOSTNAME;
char GSport[MAX_SIZE] = GS_DEFAULT_PORT;
char PLID[PLID_SIZE];
char word[MAX_WORD_SIZE];
int fd_socket_udp, fd_socket_tcp, trials;
struct addrinfo hints_udp, hints_tcp, *res_udp, *res_tcp; // Necessário declarar hints_udp aqui?
struct sockaddr_in addr_udp;
socklen_t addrlen_udp;


void parse_player_args(int argc, char **argv){
    if(argc == 1 || argc == 3 || argc == 5){
        for(int i=2; i<argc; i+=2){
            if(strcmp(argv[i-1],"-n")==0){
                if(valid_ip_address(argv[i])){
                    strcpy(GSIP, argv[i]);
                }
                else{
                    fprintf(stderr,"ERROR: Invalid GS hostname/IP address. Please try again.\n");
                    exit(EXIT_FAILURE);
                }
            }
            else if(strcmp(argv[i-1],"-p")==0){
                if(valid_port(argv[i])){
                    strcpy(GSport,argv[i]);
                }
                else{
                    fprintf(stderr,"ERROR: Invalid GS port. Please try again.\n");
                    exit(EXIT_FAILURE);
                }       
            }
            else{
                fprintf(stderr,"ERROR: Invalid player application arguments.\nUsage: ./player [-n GSIP] [-p GSport]\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    else{
        fprintf(stderr,"ERROR: Invalid player application arguments.\nUsage: ./player [-n GSIP] [-p GSport]\n"); 
        exit(EXIT_FAILURE);
    }
}

void parse_player_input(int *command, char* keyword){
    char line[MAX_SIZE], cmd_str[MAX_SIZE];
    int n;
    if(fgets(line,MAX_SIZE,stdin)){
        n = sscanf(line,"%s %s\n", cmd_str,keyword);
        if(valid_player_input(line,n)){
            *command=parse_player_command(cmd_str);
            return; // talvez melhorar?
        }
    }
    fprintf(stderr,"ERROR: Invalid player input. Please try again.\n");
    exit(EXIT_FAILURE);
    
}

void open_player_udp_socket(){
    int errcode;

    fd_socket_udp = socket(AF_INET,SOCK_DGRAM,0);
    if(fd_socket_udp==-1){
        fprintf(stderr,"ERROR: Player UDP socket failed to create. Please try again.\n"); // create ou open?
        exit(EXIT_FAILURE);
    }
        
    memset(&hints_udp,0,sizeof(hints_udp));
    hints_udp.ai_family=AF_INET;
    hints_udp.ai_socktype=SOCK_DGRAM;

    errcode = getaddrinfo(GSIP,GSport,&hints_udp,&res_udp);
    if(errcode!=0){
        close(fd_socket_udp);
        fprintf(stderr, "ERROR: Failed on UDP address translation. Please try again\n"); // address translation? 
        exit(EXIT_FAILURE);
    }

    addrlen_udp=sizeof(addr_udp); // existe algum problema em definir logo aqui?
}

void close_player_udp_socket(){
    freeaddrinfo(res_udp);
    close(fd_socket_udp);
    
}

void open_player_tcp_socket(){
    int errcode;

    fd_socket_tcp = socket(AF_INET,SOCK_STREAM,0);
    if(fd_socket_tcp == -1){
        fprintf(stderr, "ERROR: Player TCP socket failed to create. Please try again.\n");
        exit(EXIT_FAILURE);
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


void player_server_communication_udp(char *player_message, char *server_message){
    int n;
    
    open_player_udp_socket();
    // setsockopt para timers
    // outra opção para timers é SIGNALS ou usar o select


    n = sendto(fd_socket_udp,player_message,strlen(player_message),0,res_udp->ai_addr,res_udp->ai_addrlen);
    if(n==-1){
        close_player_udp_socket();
        fprintf(stderr,"ERROR: Failed to send UDP message. Please try again.\n"); 
        exit(EXIT_FAILURE); //faz sentido fechar logo o socket e sair do programa?
    }

    n = recvfrom(fd_socket_udp,server_message,MAX_SIZE,0,(struct sockaddr*)&addr_udp,&addrlen_udp);
    if(n==-1){ // tanto no sendto temos de verificar o tipo de erros possíveis
        close_player_udp_socket();
        fprintf(stderr,"ERROR: Failed to receive UDP message. Please try again.\n"); 
        exit(EXIT_FAILURE); //faz sentido fechar logo o socket e sair do programa?
    }

    close_player_udp_socket();   
}

void player_server_communication_tcp(char *player_message, char **server_message_ptr){
    int n, bytes_readed;
    char buffer[512];

    open_player_tcp_socket();

    n=connect(fd_socket_tcp, res_tcp->ai_addr, res_tcp->ai_addrlen);
    if(n==-1){
        close_player_tcp_socket();
        fprintf(stderr, "ERROR: Failed to connect to game server TCP socket. Please try again.\n");
        exit(EXIT_FAILURE);
    }

    
    n=write(fd_socket_tcp,player_message,strlen(player_message));
    if(n==-1){
        close_player_tcp_socket();
        fprintf(stderr,"ERROR: Failed to send TCP message. Please try again.\n");
        exit(EXIT_FAILURE);
    }

    
    bytes_readed = 0;
    while((n=read(fd_socket_tcp,buffer,512))>0){
        bytes_readed+=n;
        *server_message_ptr = realloc(*server_message_ptr,bytes_readed);
        
        memcpy(*server_message_ptr+bytes_readed-n,buffer,n);
        //printf("response=%s\n",server_message);
    }
    if(n==-1){
        close_player_tcp_socket();
        fprintf(stderr, "ERROR: Failed to receive TCP message. Please try again.\n");
    }

    close_player_tcp_socket();
}


void parse_rsg(char *response){ //talvez inserir no functions?
    char status[4];
    int i, num_keys, word_size, max_errors, bytes_readed;

    num_keys = sscanf(response,"RSG %s %d %d %n\n", status,&word_size,&max_errors,&bytes_readed);
    if(valid_server_response(response)){
        for(i=0;i<word_size;i++)
            word[i]='_';
        word[i]='\0';
        printf("New game started (max %d errors): %s\n",max_errors,word);
    }
    else
        fprintf(stderr,"ERROR: Invalid game server response. Please try again.\n");
}

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
                printf("Yes, '%c' is part of the word: %s\n",letter,word);
                break;
            case WIN:
                for(int i=0;i<strlen(word);i++){
                    if(word[i]=='_')
                        word[i]=letter;
                }
                printf("WELL DONE! You guessed: %s\n",word);
                break;
            case DUP:
                trials--;
                printf("Already guessed '%c' letter. Please try again with another letter.\n",letter);
                break;
            case NOK:
                printf("No, '%c' is not part of the word: %s\n",letter,word);
                break;
            case OVR:
                printf("GAME OVER!\n");
                break;
            case INV:
                printf("Invalid trial number\n");
                break;
            case ERR:
                printf("ERROR\n"); // talvez arranjar um forma de especificar este erro
                break;                
            default:
                break;
        }  
    }
    else
        fprintf(stderr,"ERROR: Invalid game server response. Please try again.\n");
}

void parse_rwg(char *response, char *word_guessed){
    char status_str[4];
    int status, trial, num_keys, bytes_readed;
    
    num_keys = sscanf(response, "RWG %s %d %n\n",status_str,&trial,&bytes_readed);

    if(valid_server_response(response)){
        status = parse_server_status(status_str);
        switch (status){
            case WIN:
                printf("WELL DONE! You guessed: %s\n",word_guessed);
                break;
            case NOK:
                printf("No, '%s' is not the correct word.\n",word_guessed);
                break;
            case OVR:
                printf("GAME OVER\n");
                break;
            case INV:
                printf("Invalid trial number\n"); // não tenho a certeza se só acontece quando o trial number não é o correcto
                break;
            case ERR:
                printf("ERROR\n");
                break;
            default:
                break;
        }
    }
}

void parse_rsb(char *response){
    const char s[2]=" ";
    char status_str[6],fname[24], *fdata;
    char score_sb[4],plid_sb[PLID_SIZE],word_sb[MAX_WORD_SIZE],trial_sb[3],size_sb[3];
    int status, fsize, num_keys, bytes_readed, line;
    FILE *fp = NULL;

    num_keys = sscanf(response, "RSB %s %n\n",status_str, &bytes_readed);
    if(valid_server_response(response)){ // pensar melhor nesta verificação
        status = parse_server_status(status_str);
        switch (status){
            case OK:
                response+=bytes_readed;
                sscanf(response, "%s %d %n",fname,&fsize,&bytes_readed);
                fp = fopen(fname,"w");
                line = 0;
                while(fsize>0){ // o que é suposto fazer quando o ficheiro tem mais bytes do que é suposto?
                    response += bytes_readed;
                    sscanf(response, "%s %s %s %s %s %n\n",score_sb,plid_sb,word_sb,trial_sb,size_sb,&bytes_readed);
                    printf("%d - player %s with %s trials for the %s letter word %s\n",++line,plid_sb,trial_sb,size_sb,word_sb);
                    if(fsize>=bytes_readed)
                        fwrite(response,1,bytes_readed,fp);
                    else
                        fwrite(response,1,fsize,fp);
                    fsize-=bytes_readed;
                }
                fclose(fp);

                break;
            case EMPTY:
                break;
        }
    }
}

void parse_rhl(char *response){
    char status_str[4], fname[24];
    int status,fsize, num_keys, bytes_readed;
    FILE *fp = NULL;

    num_keys = sscanf(response, "RHL %s %n\n", status_str, &bytes_readed);
    if(valid_server_response(response)){
        status = parse_server_status(status_str);
        switch (status){
            case OK:
                response += bytes_readed;
                sscanf(response, "%s %d %n\n",fname,&fsize,&bytes_readed);
                fp = fopen(fname,"w");
                while(fsize>0){ // falta verificar erros?
                    response += bytes_readed;
                    bytes_readed = fwrite(response,1,fsize,fp);
                    fsize-=bytes_readed;
                }
                fclose(fp);
                break;
            case NOK:
                break;
        }
    }
}

void parse_rst(char *response){
    char status_str[4], fname[24];
    int status, fsize, num_keys, bytes_readed;
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
                while(fsize>0){
                    response += bytes_readed;
                    bytes_readed = fwrite(response,1,fsize,fp);
                    printf("%.*s", bytes_readed, response);
                    fsize -= bytes_readed;
                }
                fclose(fp);
                break;
            case NOK:
                break;
        }
    }

}


void player_start_game(char *keyword){
    char player_message[MAX_SIZE], server_message[MAX_SIZE];

    if(!valid_plid(keyword))
        fprintf(stderr,"ERROR: Invalid PLID. PLID needs to be a 6-digit IST student number. Please try again.\n");
    else{
        strcpy(PLID,keyword);
        sprintf(player_message,"SNG %s\n",PLID);
    }
    player_server_communication_udp(player_message,server_message);
    parse_rsg(server_message);
    trials=0;
}

void player_play_letter(char *keyword){
    char player_message[MAX_SIZE], server_message[MAX_SIZE];
    if(!valid_letter(keyword))
        fprintf(stderr, "ERROR: Invalid player play letter command keyword. Keyword needs to be a letter. Please try again.\n");
    trials++;
    sprintf(player_message,"PLG %s %s %d\n",PLID,keyword,trials);
    player_server_communication_udp(player_message,server_message);
    parse_rlg(server_message,keyword[0]);
}

void player_guess_word(char *keyword){
    char player_message[MAX_SIZE], server_message[MAX_SIZE];
    if(!valid_word(keyword))
        fprintf(stderr, "ERROR: Invalid player guess word command. Keyword needs to be a word. Please try again.\n");
    trials++;
    sprintf(player_message,"PWG %s %s %d\n",PLID,keyword,trials);
    player_server_communication_udp(player_message,server_message);
    parse_rwg(server_message,keyword);
}

void player_get_scoreboard(){ // falta fazer free na memoria alocada
    char player_message[MAX_SIZE], *server_message, **server_message_ptr;
    server_message=NULL;
    server_message_ptr = &server_message;
    sprintf(player_message,"GSB\n");
    player_server_communication_tcp(player_message,server_message_ptr);
    parse_rsb(server_message); // provavelmente não podemos fazer isto assim, não sabemos inicialmente o tamanho máximo da resposta
}

void player_get_hint(){
    char player_message[MAX_SIZE], *server_message, **server_message_ptr;
    server_message = NULL;
    server_message_ptr = &server_message;
    sprintf(player_message,"GHL %s\n", PLID);
    player_server_communication_tcp(player_message,server_message_ptr);
    parse_rhl(server_message); 
}

void player_get_state(){
    char player_message[MAX_SIZE], *server_message, **server_message_ptr;
    server_message = NULL;
    server_message_ptr = &server_message;
    sprintf(player_message,"STA %s\n",PLID);
    player_server_communication_tcp(player_message, server_message_ptr);
    parse_rst(server_message);
}

void player_quit_game(){

}

void player_exit_app(){

}
