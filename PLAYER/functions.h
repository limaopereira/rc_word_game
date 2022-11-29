#ifndef FUNCTIONS_H
#define FUNCTIONS_H

int valid_ip_address(char *ip_address);
int valid_port(char *port);
int valid_player_input(char* string, int num_cmds);
int valid_server_response(char *response);
int parse_player_command(char *cmd_str);
int parse_server_status(char *status_str);
int valid_plid(char *keyword);
int valid_letter(char *keyword);
int valid_word(char *keyword);

#endif