#ifndef PLAYER_FUNCTIONS_H
#define PLAYER_FUNCTIONS_H

#include "../PLAYER/constants.h"

typedef struct{
    int score[10];
    char plid[10][MAX_WORD_SIZE];
    char word[10][MAX_WORD_SIZE];
    int n_succ[10];
    int n_tot[10];
} SCORELIST;

void parse_server_args(int argc, char **argv);
void load_word_file();
void open_server_udp_socket();
void open_server_tcp_socket();
void close_server_udp_socket();
void close_server_tcp_socket();

// void get_player_message_udp(int *command, char *player_message);

void handle_server_udp_requests();
void handle_server_tcp_requests();

void server_start_game(char *values);

void server_play_letter(char *values);

void server_guess_word(char *values);

void server_quit_game();

void server_error();

void server_scoreboard(int new_fd);

void server_hint(int new_fd, char *values);

void server_state(int new_fd, char *values);

void server_error_tcp(int new_fd);

#endif 