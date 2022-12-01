#ifndef PLAYER_FUNCTIONS_H
#define PLAYER_FUNCTIONS_H

#include "constants.h"

//extern char GSIP[MAX_SIZE];
//extern char GSport[MAX_SIZE];

void parse_player_args(int argc, char** argv);
void parse_player_input(int *command, char *keyword);

void open_player_udp_socket();
void close_player_udp_socket();
void player_server_communication_udp(char *player_message, char *server_message);

void parse_rsg();
void parse_rlg(char *response, char letter);
void parse_rwg(char *response, char *word_guessed);
void parse_rsb(char *response);

void player_start_game(char *keyword);
void player_play_letter(char *keyword);
void player_guess_word(char *keyword);
void player_get_scoreboard();
void player_get_hint();
void player_get_state();
void player_quit_game();
void player_exit_app();

#endif