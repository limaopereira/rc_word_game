
#ifndef SERVER_H
#define SERVER_H

#define TIMEOUT_MS 100 // 0.1 seconds

#define LETTER_COUNT 26 // How many letters in the alphabet
#define MAX_SIZE 128
#define MAX_WORDS 26
#define MAX_WORD_SIZE 31
#define MAX_HINT_SIZE 51
#define MAX_ERRORS 9
#define PLID_SIZE 6
#define MAX_TRIES MAX_ERRORS + LETTER_COUNT


struct game {
	int status;
	int trial;
	int errors;
	char PLID[PLID_SIZE + 1];
	char word[MAX_WORD_SIZE];
	char hint[MAX_HINT_SIZE];
	char current_state[MAX_WORD_SIZE];
	char letters_guessed[LETTER_COUNT + 1];
	char guesses[MAX_TRIES][MAX_WORD_SIZE];
};

typedef struct{
	int score[10];
	char plid[10][PLID_SIZE];
	char word[10][MAX_WORD_SIZE];
	int n_succ[10];
	int n_tot[10];
} SCORELIST;

// Checks snum for valid num inside [start, end]
int is_valid_num(const char *snum, const int start, const int end);

// Loads game file required for both UDP and TCP
struct game load_game(const char *game_path);

// Configures socket based on mode
int server_socket_setup(const char *, const int);

// Calls server_socket_setup with TCP mode
void tcp_server_main(const char *);

// Calls server_socket_setup with UDP mode
void udp_server_main(const char *);

#endif