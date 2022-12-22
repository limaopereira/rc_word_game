#ifndef PLAYER_FUNCTIONS_H
#define PLAYER_FUNCTIONS_H

#include "constants.h"

// player parsing

void parse_player_args(int argc, char** argv);
void parse_player_input(int *command, char *keyword);
int parse_player_command(char *cmd_str);

// server parsing

int parse_server_status(char *status_str);

// player udp functions

void open_player_udp_socket();
void close_player_udp_socket();
int player_server_communication_udp(char *player_message, char *server_message);

// player tcp functions

void open_player_tcp_socket();
void close_player_tcp_socket();
void player_server_communication_tcp(char *player_message, char **server_message_ptr);

// player commands functions 

void player_start_game(char *keyword);
void player_play_letter(char *keyword);
void player_guess_word(char *keyword);
void player_get_scoreboard();
void player_get_hint();
void player_get_state();
void player_quit_game();
void player_exit_app();

// player start game auxiliary functions

void parse_rsg(char *response);

// player play letter auxiliary functions

void parse_rlg(char *response, char letter);

// player guess word auxiliary functions

void parse_rwg(char *response, char *word_guessed);

// player get scoreboard auxiliary functions

void parse_rsb(char *response);

// player get hint auxiliary functions

void parse_rhl(char *response);

// player get state auxiliary functions 

void parse_rst(char *response);

// player quit game auxiliary functions

void parse_rqt(char *response);

// validation functions 

int valid_plid(char *keyword);
int valid_ip_address(char *ip_address);
int valid_port(char *port);

int valid_player_input(char* string, int num_cmds);
int valid_server_response(char *response);

int valid_letter(char *keyword);
int valid_word(char *keyword);



// player global auxiliary functions 

int count_spaces(char *response);

void set_read_timer(int fd_socket, int time);

void introduction_text();

#endif