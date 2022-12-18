#ifndef PLAYER_FUNCTIONS_H
#define PLAYER_FUNCTIONS_H

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

void server_scoreboard();

void server_hint(char *values);

void server_state(char *values);

void server_error_tcp();

#endif