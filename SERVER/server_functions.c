#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>


#include "server_functions.h"
#include "../PLAYER/constants.h" // atenção a organização dos ficheiros
 
int verbose = 0;
int fd_socket_udp, fd_socket_tcp;
struct sockaddr_in addr_udp, addr_tcp;
struct addrinfo hints_udp, hints_tcp, *res_udp, *res_tcp;
socklen_t addrlen_udp, addrlen_tcp;


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
        //perror("Failed to bind.\n");
        fprintf(stderr,"ERROR: Failed on UDP binding. Please try again\n");
        freeaddrinfo(res_udp);
        close(fd_socket_udp);
        exit(EXIT_FAILURE);
    }

    addrlen_udp=sizeof(addr_udp);
}

void open_server_tcp_socket(){
    int errcode;

    fd_socket_tcp = socket(AF_INET,SOCK_STREAM,0);
    if(fd_socket_tcp==-1){
        fprintf(stderr,"ERROR: Server TCP socket failed to create. Please try again.\n"); // create ou open?
        exit(EXIT_FAILURE);
    }

    memset(&hints_tcp,0,sizeof(hints_tcp));
    hints_tcp.ai_family=AF_INET;
    hints_tcp.ai_socktype=SOCK_STREAM;
    hints_tcp.ai_flags=AI_PASSIVE;

    errcode=getaddrinfo(NULL,GSport,&hints_tcp,&res_tcp);
     if(errcode!=0){
        close(fd_socket_tcp);
        fprintf(stderr, "ERROR: Failed on TCP address translation. Please try again\n"); // address translation? 
        exit(EXIT_FAILURE);
    }

    errcode=bind(fd_socket_tcp,res_tcp->ai_addr, res_tcp->ai_addrlen);
    if(errcode==-1){
        perror("Failed to bind.\n");
        fprintf(stderr,"ERROR: Failed on TCP binding. Please try again\n");
        freeaddrinfo(res_tcp);
        close(fd_socket_tcp);
        exit(EXIT_FAILURE);
    }

    errcode = listen(fd_socket_tcp,5);
    if(errcode==-1){
        fprintf(stderr,"ERROR: Failed on TCP listen. Please try again\n");
        freeaddrinfo(res_tcp);
        close(fd_socket_tcp);
        exit(EXIT_FAILURE);
    }
}



void close_server_udp_socket(){
    freeaddrinfo(res_udp);
    close(fd_socket_udp);
}

void close_server_tcp_socket(){
    freeaddrinfo(res_tcp);
    close(fd_socket_tcp);
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

int parse_server_command_tcp(char *command){
    if(strcmp(command,"GSB")==0)
        return SCOREBOARD;
    else if(strcmp(command,"GHL")==0)
        return HINT;
    else if(strcmp(command,"STA")==0)
        return STATE;
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
    char player_message[MAX_SIZE], *cmd_str, *values;
    while(1){
        n = recvfrom(fd_socket_udp,player_message,MAX_SIZE,0,(struct sockaddr*)&addr_udp,&addrlen_udp);        
        if(n==-1)
            command = ERR;
        else{
            cmd_str = strtok(player_message, " ");
            values = strtok(NULL, "");
            //n = sscanf(player_message, "%s %s %n\n",cmd_str,values,&bytes_readed);
            
            // talvez verificar se o strtok nao vai dar erro?
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

void handle_server_tcp_requests(){ // talvez dividir um bocadinho mais esta funcao
    int new_fd, n, command,end_msg;
    char buffer[PLID_SIZE], player_message[PLID_SIZE*2], *msg_ptr,*cmd_str, *values;
    pid_t pid;

    while(1){
        addrlen_tcp = sizeof(addr_tcp);
        if((new_fd=accept(fd_socket_tcp,(struct sockaddr*)&addr_tcp,&addrlen_tcp))==-1){
            // fazer o que?
        }
        pid = fork();
        if(pid == 0){
            close(fd_socket_tcp);
            end_msg = FALSE;
            msg_ptr = player_message;
            while(end_msg==FALSE){
                memset(buffer,0,sizeof(buffer));
                n = read(new_fd,buffer,sizeof(buffer));
                if(buffer[n-1]=='\n'){
                    end_msg=TRUE;
                    buffer[n-1]='\0';
                }
                memcpy(msg_ptr,buffer,n);
                msg_ptr+=n;   
            }
            if(n<0){
                // fazer o que?
            }
            else{
                cmd_str = strtok(player_message, " ");
                values = strtok(NULL, "");
                command = parse_server_command_tcp(cmd_str);
                switch (command){
                    case SCOREBOARD:
                        server_scoreboard(new_fd);
                        break;
                    case HINT:
                        server_hint(new_fd,values);
                        break;
                    case STATE:
                        server_state(new_fd,values);
                        break;
                    case ERR:
                        server_error_tcp(new_fd);
                        break;
                }
            }
            
            close(new_fd);
            exit(EXIT_SUCCESS); // necessário?
        }
        else if(pid > 0){
            close(new_fd);
        }
        else{
            // fazer o que?
        }
        
    }
}


int valid_sng_msg(char *values, char *plid, int bytes_readed){
    return 1;
}

int valid_plg_msg(char *values, char *plid, int bytes_readed){
    return 1;
}

int get_game_status(char *plid, char *word, char *hint){
    int num_lines;
    char line[MAX_WORD_SIZE + MAX_HINT_SIZE], filename[PLID_SIZE+12];
    FILE* fp;

    sprintf(filename,"GAMES/GAME_%s",plid);
    printf("filename=%s\n",filename);
    fp = fopen(filename,"r");
    printf("fp=%p\n",fp);

    if(fp){
        num_lines = 0;
        while(fgets(line,sizeof(line),fp)!=NULL){
            num_lines++;
            if(num_lines>1){
                fclose(fp);
                return ON_GOING;
            }       
        }
        
        sscanf(line,"%s %s\n", word,hint); // o que fazer em caso de erro?
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
    else if(*size > 10)
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
    int bytes_readed, word_size, max_errors,game_status;
    char plid[PLID_SIZE], server_message[MAX_SIZE], word[MAX_WORD_SIZE], hint[MAX_HINT_SIZE];
    
    sscanf(values,"%s %n\n",plid,&bytes_readed);

    if(valid_sng_msg(values,plid,bytes_readed)){
        game_status = get_game_status(plid,word,hint);
        if(game_status==ON_GOING)
            sprintf(server_message,"RSG NOK\n");
        
        else{
            if(game_status==NOT_STARTED){
                select_word(word_filename,word,hint); // talvez verificar erros aqui?
                create_player_game_file(plid,word,hint); // Qual deve ser o comportamento para os erros?
            }
            get_word_info(word,&word_size,&max_errors);
            sprintf(server_message,"RSG OK %d %d\n",word_size,max_errors);
            printf("word=%s hint=%s\n", word, hint);
        }
    }
    else
        sprintf(server_message,"RSG ERR\n");
    bytes_readed = sendto(fd_socket_udp,server_message,strlen(server_message),0,(struct sockaddr*)&addr_udp,addrlen_udp);
    if (bytes_readed == -1){
        // fazer o que?
    }
}

int get_game_info(char *plid, char *word, char words_guessed[][MAX_WORD_SIZE], char *letters_guessed, int *trial, int *errors){
    int num_lines,num_letters, num_words, result;
    char line[MAX_WORD_SIZE + MAX_HINT_SIZE], filename[PLID_SIZE+12];
    char code,play[MAX_WORD_SIZE];
    FILE* fp;

    sprintf(filename,"GAMES/GAME_%s",plid);
    fp = fopen(filename,"r");

    if(fp){
        num_lines = 0;
        num_letters = 0;
        num_words = 0;
        *errors = 0;
        while(fgets(line,sizeof(line),fp)!=NULL){
            if(num_lines == 0)
                sscanf(line,"%s",word);
            else{
                sscanf(line, "%c %s %d\n",&code,play,&result);
                if(result==FALSE)
                    (*errors)++;
                switch (code){
                    case 'T':
                        letters_guessed[num_letters++]=play[0];
                        break;
                    case 'G':
                        strcpy(words_guessed[num_words++],play);
                        break;
                }
            }
            num_lines++;      
        }
        letters_guessed[num_letters]='\0';
        *trial = num_lines;
        fclose(fp);
        return ON_GOING;
    }
    else
        return NOT_STARTED;

}

int get_positions_left(char *word, char *letters_guessed){
    int num_positions_left;

    num_positions_left = strlen(word);
    for(int i = 0; i < strlen(word); i++){
        for(int j = 0; j < strlen(letters_guessed); j++){
            if(letters_guessed[j]==word[i]){
                num_positions_left--;
                break;
            }
        }
    }
    return num_positions_left;
}

int get_letter_status(char *word, char letter, char *letters_guessed, int attempts_left, int *num_positions){
    int num_letters, num_positions_left, word_size;

    num_letters = strlen(letters_guessed);
    for(int i = 0; i < num_letters; i++){
        if(letters_guessed[i]==letter)
            return DUP;
    }
    word_size = strlen(word); // talvez não seja necessário? podia ser passado logo como argumento
    num_positions_left = get_positions_left(word,letters_guessed);
    *num_positions= 0;
    for(int i = 0; i < word_size; i++){
        if(word[i]==letter)
            (*num_positions)++;
    }
    if(*num_positions>0){
        if(*num_positions==num_positions_left)
            return WIN;
        else
            return OK;
    }
    else{
        if(attempts_left==0)
            return OVR;
        else
            return NOK;
    }
}

void get_letter_positions(char *word,char letter,int num_positions, char *positions){
    char buffer[4]; // no maximo pos de tamanho 2 mais 1 espaço mais \0 
    
    positions[0]='\0';
    sprintf(buffer,"%d",num_positions);
    strcat(positions,buffer);
    
    for(int i = 0 ; i < strlen(word); i++){
        if(word[i]==letter){
            sprintf(buffer, " %d",i);
            strcat(positions,buffer);
        }            
    }
}

void write_play_to_file(char *plid, int code, char *play, int correct){
    char filename[PLID_SIZE+12];
    FILE* fp;

    sprintf(filename,"GAMES/GAME_%s",plid);
    fp = fopen(filename,"a");
    if(code==CODE_TRIAL)
        fprintf(fp,"%c %c %d\n",code,play[0],correct);
    else{
        fprintf(fp,"%c %s %d\n",code,play,correct);
    }
    fclose(fp);
}

void get_timestamp(char  *timestamp){
    struct tm *timeinfo;
    time_t rawtime;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    sprintf(timestamp,"%d%02d%02d_%02d%02d%02d",timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
           timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
}

void transfer_file_to_player_dir(char *plid, int code, char *timestamp){
    char filename_src[PLID_SIZE+12];
    char filename_dst[PLID_SIZE+28]; // GAMES/ = 6, YYYYMMDD =8, _HHMMSS_ = 8 W.txt\0 = 6 
    char dir_name[PLID_SIZE+6]; // GAMES/ = 6    
    char code_str;
    struct stat st;
    //struct tm *timeinfo;
    //time_t rawtime;

    sprintf(dir_name,"GAMES/%s",plid);
    if(stat(dir_name,&st)!=0){
        // fazer o que?
    }
    if(!S_ISDIR(st.st_mode)){
        if (mkdir(dir_name, 0777) != 0) {
            // fazer o que?
        }
    }

    //time(&rawtime);
    //timeinfo = localtime(&rawtime);
    if(code==WIN)
        code_str='W';
    else if(code==OVR)
        code_str='F';
    else
        code_str='Q';
    sprintf(filename_dst,"%s/%s_%c.txt",dir_name,timestamp,code_str);
    sprintf(filename_src,"GAMES/GAME_%s",plid);
    if(rename(filename_src, filename_dst) != 0) {
        // fazer o que?
    }
}

void create_game_scoreboard_file(char *plid, char *timestamp, char *word, int errors, int n_trials){
    int n_succ = n_trials-errors;
    int score;
    char filename[PLID_SIZE+31];  // SCORES/=7 score=3 _DDMMYYYY_HHMMSS.txt=21
    FILE *fp;

    score = (n_succ/(double)n_trials)*100;
    sprintf(filename,"SCORES/%03d_%s_%s.txt",score,plid,timestamp);

    fp = fopen(filename,"w");
    fprintf(fp,"%03d %s %s %d %d", score,plid,word,n_succ,n_trials);
    fclose(fp);
}


void server_play_letter(char *values){
    int bytes_readed, trial_player, game_status, letter_status, word_size, max_errors, attempts_left, num_positions;
    int game_info, trial_server, num_errors;
    char letter, plid[PLID_SIZE], server_message[MAX_SIZE]; // atenção aqui no max size, porque podemos ter varias posicoes
    char word[MAX_WORD_SIZE],letters_guessed[MAX_WORD_SIZE],words_guessed[MAX_ERRORS][MAX_WORD_SIZE];
    char positions[MAX_WORD_SIZE*2+MAX_WORD_SIZE]; // pensar um bocadinho melhor nisto, mas a ideia é MAX_WORD_SIZE*2 porque no maximo as posicoes ocupam 2 chars e somamos MAX_WORD_SIZE para o numero maximo de espacos
    char timestamp[16]; // talvez acrescentar TIMESTAMP_SIZE no constants.h

    sscanf(values,"%s %c %d %n\n",plid,&letter,&trial_player,&bytes_readed);

    if(valid_plg_msg(values,plid,bytes_readed)){
        game_status = get_game_info(plid,word,words_guessed,letters_guessed,&trial_server,&num_errors);
    
        if(game_status == NOT_STARTED)
            sprintf(server_message,"RLG ERR\n");
        else if(trial_server!=trial_player)
            sprintf(server_message,"RLG INV %d\n",trial_server-1);
        else{
            get_word_info(word,&word_size,&max_errors);
            printf("word info=%s\n",word);
            attempts_left = max_errors-num_errors;
            printf("max_errors=%d num_errors=%d\n",max_errors,num_errors);
            printf("attempts_left=%d\n",attempts_left);
            letter_status = get_letter_status(word,letter,letters_guessed,attempts_left,&num_positions);

            switch (letter_status){
                case OK:
                    get_letter_positions(word,letter,num_positions,positions);
                    write_play_to_file(plid,CODE_TRIAL,&letter,TRUE);
                    sprintf(server_message,"RLG OK %d %s\n",trial_server,positions);
                    break;
                case WIN:
                    get_timestamp(timestamp);
                    write_play_to_file(plid,CODE_TRIAL,&letter,TRUE);
                    transfer_file_to_player_dir(plid,WIN,timestamp);
                    create_game_scoreboard_file(plid,timestamp,word,num_errors,trial_server);
                    sprintf(server_message,"RLG WIN\n");
                    break;
                case DUP:
                    sprintf(server_message,"RLG DUP\n");
                    break;
                case NOK:
                    write_play_to_file(plid,CODE_TRIAL,&letter,FALSE);
                    sprintf(server_message,"RLG NOK\n");
                    break;
                case OVR:
                    write_play_to_file(plid,CODE_TRIAL,&letter,FALSE);
                    transfer_file_to_player_dir(plid,OVR,timestamp);
                    sprintf(server_message,"RLG OVR\n");
                    break;
            }
        }
    }
    else
        sprintf(server_message,"RLG ERR\n");
    bytes_readed = sendto(fd_socket_udp,server_message,strlen(server_message),0,(struct sockaddr*)&addr_udp,addrlen_udp);
    if (bytes_readed == -1){
        // fazer o que?
    }
}


int valid_pwg_msg(char *values, char *plid, int bytes_readed){
    return 1;
}

int get_word_status(char *word, char *word_guess, char words_guessed[][MAX_WORD_SIZE], int attempts_left){
    for(int i = 0;  words_guessed[i][0]!='\0' ; i++){
        if(strcmp(word_guess,words_guessed[i])==0)
            return DUP;
    }
    if(strcmp(word,word_guess)==0)
        return WIN;
    else if(attempts_left==0)
        return OVR;
    else
        return NOK;
}

void server_guess_word(char *values){
    int bytes_readed, trial_player, game_status, word_status, word_size, max_errors, attempts_left;
    int trial_server, num_errors;
    char word_guess[MAX_WORD_SIZE], plid[PLID_SIZE], server_message[MAX_SIZE];
    char word[MAX_WORD_SIZE],letters_guessed[MAX_WORD_SIZE],words_guessed[MAX_ERRORS][MAX_WORD_SIZE];
    char timestamp[16];

    sscanf(values,"%s %s %d %n\n",plid,word_guess,&trial_player,&bytes_readed);

    if(valid_pwg_msg(values,plid,bytes_readed)){
        game_status = get_game_info(plid,word,words_guessed,letters_guessed,&trial_server,&num_errors);
        if(game_status == NOT_STARTED)
            sprintf(server_message,"RWG ERR\n");
        else if(trial_server!=trial_player)
            sprintf(server_message,"RWG INV %d\n",trial_server-1);
        else{
            get_word_info(word,&word_size,&max_errors);
            printf("word info=%s\n",word);
            attempts_left = max_errors-num_errors;
            printf("max_errors=%d num_errors=%d\n", max_errors,num_errors);
            printf("attempts_left=%d\n",attempts_left);
            word_status = get_word_status(word,word_guess,words_guessed,attempts_left);
            switch (word_status){
                case WIN:
                    get_timestamp(timestamp);
                    write_play_to_file(plid,CODE_GUESS,word_guess,TRUE);
                    transfer_file_to_player_dir(plid,WIN,timestamp);
                    create_game_scoreboard_file(plid,timestamp,word,num_errors,trial_server);
                    sprintf(server_message,"RWG WIN\n");
                    break;
                case DUP:
                    sprintf(server_message,"RWG DUP\n");
                    break;
                case NOK:
                    write_play_to_file(plid,CODE_GUESS,word_guess,FALSE);
                    sprintf(server_message,"RWG NOK\n");
                    break;
                case OVR:
                    get_timestamp(timestamp);
                    write_play_to_file(plid,CODE_GUESS,word_guess,FALSE);
                    transfer_file_to_player_dir(plid,OVR,timestamp);
                    sprintf(server_message,"RLG OVR\n");
                    break;
            }
        }
    }
    else
        sprintf(server_message,"RWG ERR\n");
    bytes_readed = sendto(fd_socket_udp,server_message,strlen(server_message),0,(struct sockaddr*)&addr_udp,addrlen_udp);
    if (bytes_readed == -1){
        // fazer o que?
    }
}

int valid_qut_msg(char * values, char *plid, int bytes_readed){
    return 1;
}

void server_quit_game(char *values){
    int bytes_readed, game_status;
    char word[MAX_WORD_SIZE],hint[MAX_HINT_SIZE],plid[PLID_SIZE],server_message[MAX_SIZE];
    char timestamp[16];

    sscanf(values, "%s %n\n",plid,&bytes_readed);

    if(valid_qut_msg(values,plid,bytes_readed)){
        game_status = get_game_status(plid,word,hint); // pensar se faz sentido usar todas as variaveis
        if(!(game_status == NOT_STARTED)){
            get_timestamp(timestamp);
            transfer_file_to_player_dir(plid,QUIT,timestamp);
            sprintf(server_message,"RQT OK\n");
        }
        else
            sprintf(server_message,"RQT NOK\n");
    }
    else
        sprintf(server_message,"RQT ERR\n");
    bytes_readed = sendto(fd_socket_udp,server_message,strlen(server_message),0,(struct sockaddr*)&addr_udp,addrlen_udp);
    if (bytes_readed == -1){
        // fazer o que?
    }
}


void server_error(){ // Pensar melhor nisto
    int bytes_readed;
    char server_message[MAX_SIZE];
    sprintf(server_message,"ERR\n");
    bytes_readed = sendto(fd_socket_udp,server_message,strlen(server_message),0,(struct sockaddr*)&addr_udp,addrlen_udp);
    if (bytes_readed == -1){
        // fazer o que?
    }

}

int find_top_scores(SCORELIST *list){//SCORELIST *list){
    struct dirent **filelist;
    int n_entries, i_file, entry;
    int score, n_succ, n_tot;
    char plid[PLID_SIZE], word[MAX_WORD_SIZE];
    char fname[263]; // tinhamos 50 mas para evitar o warning usamos 263
    FILE *fp;

    n_entries = scandir("SCORES/",&filelist,0,alphasort);

    i_file=0;
    if(n_entries<0)
        return 0;
    else{
        entry = n_entries;
        while(entry && entry-- > n_entries-10){ // 10 devido a querer apenas os 10 melhores
            //printf("entries=%d entry=%d filelist[n_entries]=%s\n",n_entries,entry,filelist[3]->d_name);
            if(filelist[entry]->d_name[0]!='.'){
                sprintf(fname,"SCORES/%s",filelist[entry]->d_name);
                fp = fopen(fname,"r");
                if(fp!=NULL){
                    //fscanf(fp,"%d %s %s %d %d",&score,plid,word,&n_succ,&n_tot);
                    fscanf(fp,"%d %s %s %d %d",&list->score[i_file], list->plid[i_file], list->word[i_file], &list->n_succ[i_file], &list->n_tot[i_file]);
                }
                fclose(fp);
                ++i_file;
            }
            free(filelist[n_entries]);
        }
        free(filelist);
    }
    //list->n_scores=i_file;
    return i_file;
}

int create_scoreboard(SCORELIST list, int num_games_won, char **scoreboard){
    int num_bytes;
    char title[]="-------------------------------- TOP 10 SCORES --------------------------------\n\n";
    char header[]="    SCORE PLAYER     WORD                             GOOD TRIALS  TOTAL TRIALS\n\n";
    char line[129]; // 81 mas para evitar warnings do compilador metemos 129

    num_bytes = sizeof(title) + sizeof(header)-2;
    *scoreboard = malloc(num_bytes);
    strcat(*scoreboard,title);
    strcat(*scoreboard,header);

    for(int i = 0; i < num_games_won; i++){
        //num_bytes+=sizeof(line);
        num_bytes+=sprintf(line,"%2d - %d  %s  %-39s %-13d %-2d\n",i+1,list.score[i],list.plid[i],list.word[i],list.n_succ[i],list.n_tot[i]);
        *scoreboard = realloc(*scoreboard,num_bytes);
        strcat(*scoreboard,line);
    }
    num_bytes+=3;
    *scoreboard = realloc(*scoreboard,num_bytes);
    strcat(*scoreboard,"\n\n\n");
    return num_bytes;
}

    

void server_scoreboard(int new_fd){ 
    int n, num_games_won, scoreboard_size, bytes_left;
    char *server_message,*scoreboard, fname[22];
    pid_t pid;
    SCORELIST list;
    
    // falta verificar a mensagem do player?

    num_games_won = find_top_scores(&list);
    if (num_games_won == 0){
        server_message = malloc(11);
        sprintf(server_message,"RSB EMPTY\n");
    }
    else {
        scoreboard_size = create_scoreboard(list,num_games_won,&scoreboard);
        pid = getpid();
    
        sprintf(fname,"TOPSCORES_%d.txt",pid);
        server_message = malloc(7+strlen(fname)+1+sizeof(scoreboard_size)+1+scoreboard_size);
        // é necessário criar o ficheiro no server?
        sprintf(server_message,"RSB OK %s %d %s",fname,scoreboard_size,scoreboard);
    }
    bytes_left = strlen(server_message);
    printf("bytes_left=%d\n",bytes_left);
    while(bytes_left>0){
        n = write(new_fd,server_message,bytes_left);
        bytes_left-=n;
    }
}

int valid_ghl_msg(char *values, char *plid, int bytes_readed){
    return 1;
}

long get_hint_file_data(char *hint, char **hint_file_data){
    long num_bytes, bytes_readed;
    char filename[MAX_HINT_SIZE+9]; // imagens/ = 9
    FILE *fp;

    sprintf(filename,"imagens/%s",hint);
    fp = fopen(filename,"r");
    printf("filename=%s\n",filename);
                
    if(fp){
        fseek(fp,0,SEEK_END);
        num_bytes = ftell(fp);
        rewind(fp);
        *hint_file_data = malloc(num_bytes);
        bytes_readed = 0;
        while(bytes_readed<num_bytes)
            bytes_readed += fread(*hint_file_data+bytes_readed, sizeof(char), num_bytes-bytes_readed,fp);
        //(*hint_file_data)[bytes_readed]='\0';
        fclose(fp);
        return num_bytes;
    }
    else{
        // fazer o que?
    }

    



}

void server_hint(int new_fd, char *values){
    int n, bytes_readed, game_status, bytes_left;
    long hint_file_size;
    char plid[PLID_SIZE],word[MAX_WORD_SIZE], hint[MAX_HINT_SIZE], *server_message;
    char *hint_file_data;

    
    sscanf(values,"%s %n\n",plid,&bytes_readed);

    if(valid_ghl_msg(values,plid,bytes_readed)){
        game_status = get_game_status(plid,word,hint);
        if(game_status==NOT_STARTED){
            server_message = malloc(9);
            sprintf(server_message,"RHL NOK\n");
            bytes_left = strlen(server_message);
        }
        else{
            hint_file_size = get_hint_file_data(hint,&hint_file_data);
            server_message = malloc(7+strlen(hint)+1+sizeof(hint_file_size)+1+hint_file_size);
            sprintf(server_message,"RHL OK %s %ld ",hint,hint_file_size); 
            memcpy(server_message+strlen(server_message)-1,hint_file_data,hint_file_size);
            //strncat(server_message,hint_file_data,hint_file_size);
            bytes_left = strlen(server_message)+hint_file_size;
        }
    }
    else{
        server_message = malloc(9);
        sprintf(server_message,"RHL NOK\n");
        bytes_left=strlen(server_message);
    }
    
    while(bytes_left>0){
        n = write(new_fd,server_message,bytes_left);
        bytes_left-=n;
    }


}

void server_state(int new_fd, char *values){

}

void server_error_tcp(int new_fd){

}
