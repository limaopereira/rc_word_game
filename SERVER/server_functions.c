#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <time.h>

#include "server_functions.h"
#include "../PLAYER/constants.h" // atenção a organização dos ficheiros
 
int verbose = 0;
int fd_socket_udp;
struct sockaddr_in addr_udp;
struct addrinfo hints_udp,*res_udp;
socklen_t addrlen_udp;


char GSport[256]="58076";
char word_filename[24];

char words[MAX_WORDS][MAX_WORD_SIZE];
char hints[MAX_WORDS][MAX_HINT_SIZE];


int valid_port(char *port){
    return 1;
}

int valid_filename(char *filename){
    return 1;
}

void parse_server_args(int argc, char **argv){
    int c, filename_exists;

    filename_exists = 0;
    while ( (c = getopt(argc, argv, "p:v"))  != -1 ){
		switch (c) {
			case 'p':
				if (!valid_port(optarg) ) {
					fprintf(stderr,"ERROR: Invalid GS port. Please try again.\n");
                    exit(EXIT_FAILURE);
				}
                else{
                    strcpy(GSport,optarg);
                }
				break;
			case 'v':
				verbose = 1;
				break;
			default:
                fprintf(stderr,"ERROR: Invalid server application arguments.\nUsage: ./GS word_file [-p GSport] [-v]\n");
                exit(EXIT_FAILURE);
				//fprintf( stderr, "\nUsage: %s word_file [-p GSport] [-v]\n", argv[0] ); 
	    }
    }
    if(optind>=argc){
        fprintf(stderr,"ERROR: Invalid server application arguments.\nUsage: ./GS word_file [-p GSport] [-v]\n");
        exit(EXIT_FAILURE);
    }
    while(optind<argc){
        if (filename_exists == 0 && valid_filename(argv[optind])){
            strcpy(word_filename,argv[optind]);
            filename_exists=1;
        }
        else{
            fprintf(stderr,"ERROR: Invalid server application arguments.\nUsage: ./GS word_file [-p GSport] [-v]\n");
            exit(EXIT_FAILURE);
            // fprintf( stderr, "%s: argument is required -- 'word_file'\n"
            //             "\nUsage: %s word_file [-p GSport] [-v]\n", argv[0], argv[0] );
        }
        optind++;
    }
    printf("word_file=%s GSport=%s verbose=%d\n",word_filename,GSport,verbose);
}

void load_word_file(){
    int num_words = 0;
    int max_line_size = MAX_WORD_SIZE + MAX_HINT_SIZE;
    char line[max_line_size];
    
    FILE *fp = fopen(word_filename,"r");
    if (fp==NULL){
        fprintf(stderr, "ERROR: Word file failed to load. Please try again.\n");
        exit(EXIT_FAILURE);
    }
    while (fgets(line,max_line_size,fp) != NULL){
        strcpy(words[num_words],strtok(line, " "));
        strcpy(hints[num_words],strtok(NULL, " "));
        num_words++;
    }
    fclose(fp);
}


void open_server_udp_socket(){
    int errcode;

    fd_socket_udp=socket(AF_INET,SOCK_DGRAM,0);//UDP socket
    if(fd_socket_udp==-1){
        fprintf(stderr,"ERROR: Server UDP socket failed to create. Please try again.\n"); // create ou open?
        exit(EXIT_FAILURE);
    }

    memset(&hints_udp,0,sizeof(hints_udp));
    hints_udp.ai_family=AF_INET;//IPv4
    hints_udp.ai_socktype=SOCK_DGRAM;//UDP socket
    hints_udp.ai_flags=AI_PASSIVE;

    errcode=getaddrinfo(NULL,GSport,&hints_udp,&res_udp);
    if(errcode!=0){
        close(fd_socket_udp);
        fprintf(stderr, "ERROR: Failed on UDP address translation. Please try again\n"); // address translation? 
        exit(EXIT_FAILURE);
    }

    errcode=bind(fd_socket_udp,res_udp->ai_addr, res_udp->ai_addrlen);
    if(errcode==-1){
        perror("Failed to bind.\n");
        fprintf(stderr,"ERROR: Failed on UDP binding. Please try again\n");
        freeaddrinfo(res_udp);
        close(fd_socket_udp);
        exit(EXIT_FAILURE);
    }

    addrlen_udp=sizeof(addr_udp);
}

void close_server_udp_socket(){
    freeaddrinfo(res_udp);
    close(fd_socket_udp);
}

int valid_player_message(char *player_message){
    return 1;
}

int parse_server_command_udp(char *command){
    if(strcmp(command,"SNG")==0)
        return START;
    else if(strcmp(command,"PLG")==0)
        return PLAY;
    else if(strcmp(command,"PWG")==0)
        return GUESS;
    else if(strcmp(command,"QUT")==0)
        return QUIT;
    else
        return ERR;
}

// void get_player_message_udp(int *command, char *player_message){
//     int n;
//     // 
//     n = recvfrom(fd_socket_udp,player_message,MAX_SIZE,0,(struct sockaddr*)&addr_udp,&addrlen_udp);
//     if(!valid_player_message(player_message) || n==-1){
//         *command = ERR;
//     }
//     else{
//         *command = parse_server_command_udp(player_message);
//     }
// }

void handle_server_udp_requests(){
    int n, command,bytes_readed;
    char player_message[MAX_SIZE], cmd_str[MAX_SIZE], values[MAX_SIZE];
    while(1){
        n = recvfrom(fd_socket_udp,player_message,MAX_SIZE,0,(struct sockaddr*)&addr_udp,&addrlen_udp);        
        if(n==-1)
            command = ERR;
        else{
            n = sscanf(player_message, "%s %s %n\n",cmd_str,values,&bytes_readed);
            if(n==-1)
                command = ERR;
            else
                command = parse_server_command_udp(cmd_str);
        }
        switch(command){
            case START:
                server_start_game(values);
                break;
            case PLAY:
                server_play_letter(values);
                break;
            case GUESS:
                server_guess_word(values);
                break;
            case QUIT:
                server_quit_game(values);
                break;
            case ERR:
                server_error();
                break;
        }
    }
}

int valid_sng_msg(char *values, char *plid, int bytes_readed){
    return 1;
}

int game_status(char *plid, char *word){
    int num_lines;
    char line[MAX_WORD_SIZE + MAX_HINT_SIZE], filename[PLID_SIZE+12];
    FILE* fp;

    sprintf(filename,"GAMES/GAME_%s",plid);
    fp = fopen(filename,"r");

    if(fp){
        num_lines = 0;
        while(fgets(line,sizeof(line),fp)!=NULL){
            num_lines++;
            if(num_lines>1){
                fclose(fp);
                return ON_GOING;
            }       
        }
        
        sscanf(line,"%s", word); // o que fazer em caso de erro?
        fclose(fp);
        return NOT_PLAYED;
        
    }
    else
        return NOT_STARTED;
}

void select_word(char *filename, char *word, char *hint){
    int r;
    srand (time(NULL));
    r = rand() % (MAX_WORDS);
    printf("r=%d\n",r);
    strcpy(word,words[r]);
    strcpy(hint,hints[r]);
}




void get_word_info(char *word, int *size, int *errors){
    *size = strlen(word);
    if(*size <= 6 )
        *errors = 7;
    else if(*size <= 10)
        *errors = 8;
    else if(*size >11)
        *errors = 9;
}

void create_player_game_file(char *plid, char *word, char *hint){
    char filename[PLID_SIZE+12]; // 12 = strlen("/GAMES/GAME_")
    FILE *fp;
    
    
    sprintf(filename,"GAMES/GAME_%s",plid);
    printf("filename=%s\n",filename);

    fp = fopen(filename,"w"); // o que fazer em caso de erro?
    
    fprintf(fp,"%s %s",word,hint);
    fclose(fp);
}


void server_start_game(char *values){
    int bytes_readed, word_size, max_errors,status;
    char plid[PLID_SIZE], server_message[MAX_SIZE], word[MAX_WORD_SIZE], hint[MAX_HINT_SIZE];
    
    sscanf(values,"%s %n\n",plid,&bytes_readed);

    if(valid_sng_msg(values,plid,bytes_readed)){
        status = game_status(plid,word);
        if(status==ON_GOING)
            sprintf(server_message,"SNG NOK\n");
        
        else{
            if(status==NOT_STARTED){
                select_word(word_filename,word,hint); // talvez verificar erros aqui?
                create_player_game_file(plid,word,hint); // Qual deve ser o comportamento para os erros?
            }
            get_word_info(word,&word_size,&max_errors);
            sprintf(server_message,"SNG OK %d %d\n",word_size,max_errors);
            printf("word=%s hint=%s\n", word, hint);
        }
    }
    else
        sprintf(server_message,"SNG ERR\n");
    bytes_readed = sendto(fd_socket_udp,server_message,strlen(server_message),0,(struct sockaddr*)&addr_udp,addrlen_udp);
    if (bytes_readed == -1){
        // fazer o que?
    }
}

void server_play_letter(char *values){

}

void server_guess_word(char *values){

}

void server_quit_game(){
    
}

void server_error(){

}