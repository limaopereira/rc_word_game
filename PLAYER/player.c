#include <stdio.h>
#include "player_functions.h"

int main(int argc, char **argv){

    int command;
    char keyword[MAX_SIZE];

    parse_player_args(argc,argv);
    //open_player_udp_socket(); // faz sentido aqui ou só no start?
    while(1){
        parse_player_input(&command,keyword);
        switch (command){
            case START:
                player_start_game(keyword);
                break;
            case PLAY:
                player_play_letter(keyword);
                break;
            case GUESS:
                player_guess_word(keyword);
                break;
            case SCOREBOARD:
                player_get_scoreboard();
                break;
            case HINT:
                player_get_hint();
                break;
            case STATE:
                player_get_state();
                break;
            case QUIT:
                player_quit_game();
                break;
            case EXIT:
                player_exit_app();
                break;
            case INVALID_CMD:
                fprintf(stderr,"ERROR: Invalid player command. Please try again.\n");
                break;
        }
    }
}
