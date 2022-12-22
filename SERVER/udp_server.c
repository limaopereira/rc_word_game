
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <dirent.h> // Folder shenanigans
#include <signal.h> // To catch Ctrl-C

#include <netdb.h>
#include <sys/socket.h>

#include "server.h"

// This is the UDP server socket
int fd_udp_socket;

// These are used for the client connection. They can
// be global because there aren't parallel processes
socklen_t udp_client_addrlen;
struct sockaddr udp_client_addr;

extern int is_verbose;

int random_word = 0;
extern int num_words;

extern char words[MAX_WORDS][MAX_WORD_SIZE];
extern char hints[MAX_WORDS][MAX_HINT_SIZE];

static volatile sig_atomic_t udp_interrupted = 0;

/*
 *	Catches Ctrl-C to safely stop everything
 *
 *	Argument required but isn't used.
 */
void udp_sig_handler(int _) {
	_++;
	udp_interrupted = 1;
}


/*
 *	Configures UDP socket
 *
 *	Inputs: port_num
 *	Returns: fd_udp_socket, exits if error
 */
int udp_server_setup(const char *port_num) {
	fd_udp_socket = server_socket_setup(port_num, SOCK_DGRAM);

	return fd_udp_socket;
}


/*
 *	Calculates max_errors based on word length
 *
 *	Inputs: word length
 *	Returns: max_errors
 */
int max_errors(char *word) {
	size_t length = strlen(word);

	if ( length < 7)
		return 7;
	if ( length < 11 )
		return 8;
	return 9;
}


/*
 *	Saves struct game to a game file
 *
 *	Input: game_path, struct game
 *	Returns: 0 if success
 */
int save_game(const char *game_path, const struct game game) {
	FILE *game_file;
	
	game_file = fopen( game_path, "w" );

	fprintf( game_file, "PLID=%s;\n", game.PLID );
	fprintf( game_file, "word=%s;\n", game.word );
	fprintf( game_file, "hint=%s;\n", game.hint );
	fprintf( game_file, "status=%d;\n", game.status );
	fprintf( game_file, "trials=%d;\n", game.trial );
	fprintf( game_file, "errors=%d;\n", game.errors );
	fprintf( game_file, "current_state=%s;\n", game.current_state );

	fprintf( game_file, "letters_guessed=%s;\n", game.letters_guessed );

	for (int i = 0; i < MAX_TRIES && game.guesses[i][0]; i++)
		fprintf( game_file, "guess=%s;\n", game.guesses[i] );

	fclose(game_file);

	return EXIT_SUCCESS;
}


/*
 *	Loads struct game from a game file
 *
 *	Input: game_path
 *	Returns: struct game
 */
struct game load_game(const char *game_path) {
	struct game game;
	FILE *game_file;
	
	game_file = fopen( game_path, "r" );

	// %[^;]; matches everything except ; and then matches ;
	// This is because letters_guessed can be null
	fscanf( game_file, "PLID=%[^;]; ", game.PLID );
	fscanf( game_file, "word=%[^;]; ", game.word );
	fscanf( game_file, "hint=%[^;]; ", game.hint );
	fscanf( game_file, "status=%d; ", &game.status );
	fscanf( game_file, "trials=%d; ", &game.trial );
	fscanf( game_file, "errors=%d; ", &game.errors );
	fscanf( game_file, "current_state=%[^;]; ", game.current_state );

	fscanf( game_file, "letters_guessed=%s ", game.letters_guessed );
	if ( game.letters_guessed[0] == ';' )
		memset( game.letters_guessed, 0, LETTER_COUNT + 1 );
	if ( game.letters_guessed[strlen(game.letters_guessed) - 1] == ';' )
		game.letters_guessed[strlen(game.letters_guessed) - 1] = '\0';

	// Clean words_guessed array and then match lines
	for (int i = 0; i < MAX_TRIES; i++)
		memset( game.guesses[i], 0, MAX_WORD_SIZE );

	for (int i = 0; i < MAX_TRIES; i++)
		fscanf( game_file, "guess=%[^;]; ", game.guesses[i] );

	fclose(game_file);

	return game;
}


int start_new_game(char *message) {
	struct game game;
	char PLID[PLID_SIZE + 1] = "\0";
	char game_path[16 + PLID_SIZE] = "\0";

	// Test Command
	if ( strncmp(message, "SNG ", 4) ) {
		if ( is_verbose )
			printf("[ERROR] unknown command '%s'\n", strip_message(message));
		return sprintf( message, "ERR\n" );
	}

	// Get message parameters
	if ( sscanf( message, "SNG %6c", PLID ) != 1 ) {
		if ( is_verbose )
			printf("[ERROR] SNG malformed command '%s'\n", strip_message(message));
		return sprintf( message, "RSG ERR\n" );
	}

	// Test PLID
	if ( !is_valid_num(PLID, 1, 999999) ) {
		if ( is_verbose )
			printf("[ERROR] SNG malformed PLID '%s'\n", PLID);
		return sprintf( message, "RSG ERR\n" );
	}
	
	sprintf( game_path, "GAMES/GAME_%s.txt", PLID );
	
	// Check if game exists and is ongoing, NOK if yes to both
	if ( !access(game_path, F_OK) ) {
		game = load_game(game_path);

		if ( !game.status ) {
			if ( is_verbose )
				printf("PLID=%s: SNG active game found\n", PLID);
			return sprintf( message, "RSG NOK\n" );
		}
	}

	// Fill game struct
	game.status = 0;
	game.trial = 1;
	memset(game.letters_guessed, 0, LETTER_COUNT + 1);
	strcpy( game.PLID, PLID );
	strcpy( game.word, words[random_word] );
	strcpy( game.hint, hints[random_word] );
	random_word++;

	game.errors = max_errors(game.word) ;

	memset(game.current_state, 0, MAX_WORD_SIZE);
	for (int i = strlen(game.word) - 1; i >= 0; i--)
		game.current_state[i] = '-';

	for (int i = 0; i < MAX_TRIES; i++) {
		memset(game.guesses[i], 0, MAX_WORD_SIZE);
	}

	// Create game file
	save_game(game_path, game);

	if ( is_verbose )
		printf("PLID=%s: new game; word='%s' (%ld letters)\n", game.PLID, game.word, strlen(game.word));

	return sprintf( message, "RSG OK %ld %d\n", strlen(game.word), game.errors );
}

int player_guess_letter(char *message) {
	struct game game;
	int i, trial;
	char letter;
	char score[4] = "\0";
	char PLID[PLID_SIZE + 1] = "\0";
	char game_path[16 + PLID_SIZE] = "\0";

	// Test Command
	if ( strncmp(message, "PLG ", 4) ) {
		if ( is_verbose )
			printf("[ERROR] unknown command '%s'\n", strip_message(message));
		return sprintf( message, "ERR\n" );
	}

	// Get message parameters
	if ( sscanf( message, "PLG %6c %c %d", PLID, &letter, &trial ) != 3 ) {
		if ( is_verbose )
			printf("[ERROR] PLG malformed command '%s'\n", strip_message(message));
		return sprintf( message, "RLG ERR\n" );
	}

	// Test PLID
	if ( !is_valid_num(PLID, 1, 999999) ) {
		if ( is_verbose )
			printf("[ERROR] PLG malformed PLID '%s'\n", PLID);
		return sprintf( message, "RLG ERR\n" );
	}

	sprintf( game_path, "GAMES/GAME_%s.txt", PLID );

	// Check if game exists, ERR if not
	if ( access(game_path, F_OK) ) {
		if ( is_verbose )
			printf("[ERROR] PLG no game found for PLID '%s'\n", PLID);
		return sprintf( message, "RLG ERR\n" );
	}

	game = load_game(game_path);

	// Check if last game is finished
	if ( game.status ) {
		if ( is_verbose )
			printf("[ERROR] PLG no active game found for PLID '%s'\n", PLID);
		return sprintf( message, "RLG ERR\n" );
	}

	// Check trial num
	if ( trial != game.trial ) {
		// trial is different, check if last letter guessed is same as current
		// if yes, then client repeated last move
		if ( trial + 1 != game.trial || game.letters_guessed[strlen(game.letters_guessed) - 1] != letter ) {
			if ( is_verbose )
				printf("[ERROR] PLG wrong trial number for PLID '%s' - %d instead of %d\n", PLID, trial, game.trial);
			return sprintf( message, "RLG INV\n" );
		}

		// Recalculates position
		game.trial = trial;
		game.letters_guessed[strlen(game.letters_guessed) - 1] = '\0';
	}

	// Checks if letter is dupe
	for ( i = 0; game.letters_guessed[i]; i++ ) {
		if (game.letters_guessed[i] == letter) {
			if ( is_verbose )
				printf("PLID=%s: guess letter \"%c\" - DUP;\n", game.PLID, letter);
			return sprintf( message, "RLG DUP %d\n", trial );
		}
	}

	// Inserts letter into guessed letters and history
	game.letters_guessed[i] = letter;
	game.guesses[game.trial - 1][0] = letter;

	// Find first match between letter guessed and word to guess
	for ( i = 0; game.word[i] != letter && game.word[i]; i++ );

	// Didn't find letter in word
	if ( i == strlen(game.word) ) {
		game.trial++;
		game.errors--;

		if ( game.errors ) {
			if ( is_verbose )
				printf("PLID=%s: play letter \"%c\" - missed; %d tries left\n", game.PLID, letter, game.errors);
			sprintf( message, "RLG NOK %d\n", trial );
		} else {
			if ( is_verbose )
				printf("PLID=%s: play letter \"%c\" - missed; GAME OVER\n", game.PLID, letter);
			game.status = 1; // finished
			sprintf( message, "RLG OVR %d\n", trial );
		}


		return save_game(game_path, game);
	}

	game.current_state[i] = letter;

	int hits = 0;
	sprintf( message, "RLG OK %d", trial );
	for ( ; game.word[i]; i++ ) {
		if ( game.word[i] == letter ) {
			hits++;
			sprintf( message + strlen(message), " %d", i + 1 );
			game.current_state[i] = letter; // Fill out current state with the letter
		}
	}

	// Compare guess state to word, if not match then correct else win
	if ( strcmp( game.current_state, game.word ) ) {
		if ( is_verbose )
			printf("PLID=%s: play letter \"%c\" - %d hits; word not guessed\n", game.PLID, letter, hits);
		game.trial++;
		save_game(game_path, game);
		return sprintf( message + strlen(message), "\n" );
	}

	if ( is_verbose )
		printf("PLID=%s: play letter \"%c\" - %d hits; WIN\n", game.PLID, letter, hits);

	game.status = 1;
	save_game(game_path, game);

	// Save to SCORES, check if previous game and overwrite if better
	struct dirent **namelist;
	i = scandir("SCORES/", &namelist, NULL, alphasort);
	while (i--) {
		if ( !strncmp( namelist[i]->d_name + 4, PLID, PLID_SIZE ) )
			strcpy(score, namelist[i]->d_name);
		free(namelist[i]);
	}
	free(namelist);
	i = ((game.trial - (max_errors(game.word) - game.errors)) * 100) / game.trial;
	if ( is_valid_num(score, 1, 100) < i ) {
		sprintf( game_path, "SCORES/%s_%s.txt", score, PLID );
		remove(game_path);
		sprintf( game_path, "SCORES/%3d_%s.txt", i, PLID );
		save_game(game_path, game);
	}
	save_game(game_path, game);

	return sprintf( message, "RLG WIN %d\n", trial );
}

int player_guess_word(char *message) {
	struct game game;
	int i, trial;
	char score[4] = "\0";
	char word[MAX_WORD_SIZE] = "\0";
	char PLID[PLID_SIZE + 1] = "\0";
	char game_path[16 + PLID_SIZE] = "\0";

	// Test Command
	if ( strncmp(message, "PWG ", 4) ) {
		if ( is_verbose )
			printf("[ERROR] unknown command '%s'\n", strip_message(message));
		return sprintf( message, "ERR\n" );
	}

	// Get message parameters
	if ( sscanf( message, "PWG %6c %s %d", PLID, word, &trial ) != 3 ) {
		if ( is_verbose )
			printf("[ERROR] malformed command '%s'\n", strip_message(message));
		return sprintf( message, "RWG ERR\n" );
	}

	// Test PLID
	if ( !is_valid_num(PLID, 1, 999999) ) {
		if ( is_verbose )
			printf("[ERROR] malformed PLID '%s'\n", PLID);
		return sprintf( message, "RWG ERR\n" );
	}

	sprintf( game_path, "GAMES/GAME_%s.txt", PLID );

	// Check if game exists, ERR if not
	if ( access(game_path, F_OK) ) {
		if ( is_verbose )
			printf("[ERROR] PWG no game found for PLID '%s'\n", PLID);
		return sprintf( message, "RWG ERR\n" );
	}

	game = load_game(game_path);

	// Check if last game is finished
	if ( game.status ) {
		if ( is_verbose )
			printf("[ERROR] PWG no active game found for PLID '%s'\n", PLID);
		return sprintf( message, "RWG ERR\n" );
	}

	// Check trial num
	if ( trial != game.trial ) {
		// trial is different, check if last letter guessed is same as current
		// if yes, then client repeated last move
		if ( trial + 1 != game.trial || strcmp(game.guesses[game.trial - 2], word) ) {
			if ( is_verbose )
				printf("[ERROR] PWG wrong trial number for PLID '%s' - %d instead of %d\n", PLID, trial, game.trial);
			return sprintf( message, "RWG INV\n" );
		}
		
		// Recalculates position
		game.trial = trial;
		game.guesses[game.trial - 1][0] = '\0';
	}

	// Checks if letter is dupe
	for ( i = 0; game.guesses[i][0]; i++ ) {
		if ( !strcmp(game.guesses[i], word) ) {
			if ( is_verbose )
				printf("PLID=%s: guess word \"%s\" - DUP;\n", game.PLID, word);
			return sprintf( message, "RWG DUP %d\n", trial );
		}
	}

	// Inserts word into guessed letters and history
	strcpy(game.guesses[game.trial - 1], word);

	// Didn't find letter in word
	if ( strcmp(game.word, word) ) {
		game.trial++;
		game.errors--;

		if ( game.errors ) {
			if ( is_verbose )
				printf("PLID=%s: guess word \"%s\" - wrong; %d tries left\n", game.PLID, word, game.errors);
			sprintf( message, "RWG NOK %d\n", trial );
		} else {
			if ( is_verbose )
				printf("PLID=%s: guess word \"%s\" - LOSE\n", game.PLID, word);
			game.status = 1; // finished
			sprintf( message, "RWG OVR %d\n", trial );
		}

		return save_game(game_path, game);
	}

	strcpy(game.current_state, game.word);

	if ( is_verbose )
		printf("PLID=%s: guess word \"%s\" - WIN\n", game.PLID, word);
	game.status = 1;
	save_game(game_path, game);


	// Save to SCORES, check if previous game and overwrite if better
	struct dirent **namelist;
	i = scandir("SCORES/", &namelist, NULL, alphasort);
	while (i--) {
		if ( !strncmp( namelist[i]->d_name + 4, PLID, PLID_SIZE ) )
			strcpy(score, namelist[i]->d_name);
		free(namelist[i]);
	}
	free(namelist);
	i = ((game.trial - (max_errors(game.word) - game.errors)) * 100) / game.trial;
	if ( is_valid_num(score, 1, 100) < i ) {
		sprintf( game_path, "SCORES/%s_%s.txt", score, PLID );
		remove(game_path);
		sprintf( game_path, "SCORES/%3d_%s.txt", i, PLID );
		save_game(game_path, game);
	}
	save_game(game_path, game);

	return sprintf( message, "RWG WIN %d\n", trial );
}

int player_quit_game(char *message) {
	struct game game;
	char PLID[PLID_SIZE + 1] = "\0";
	char game_path[16 + PLID_SIZE] = "\0";

	// Test Command
	if ( strncmp(message, "QUT ", 4) ) {
		if ( is_verbose )
			printf("[ERROR] unknown command '%s'\n", strip_message(message));
		return sprintf( message, "ERR\n" );
	}

	// Get message parameters
	if ( sscanf( message, "QUT %6c", PLID ) != 1 ) {
		if ( is_verbose )
			printf("[ERROR] malformed command '%s'\n", strip_message(message));
		return sprintf( message, "RQT ERR\n" );
	}

	// Test PLID
	if ( !is_valid_num(PLID, 1, 999999) ) {
		if ( is_verbose )
			printf("[ERROR] malformed PLID '%s'\n", PLID);
		return sprintf( message, "RQT ERR\n" );
	}

	sprintf( game_path, "GAMES/GAME_%s.txt", PLID );

	// Check if game exists, ERR if not
	if ( access(game_path, F_OK) )
		return sprintf( message, "RQT NOK\n" );

	game = load_game(game_path);

	// Check if last game is finished
	if ( game.status )
		return sprintf( message, "RQT NOK\n" );

	game.status = 1;
	save_game(game_path, game);

	if ( is_verbose )
		printf("PLID=%s: quit game; word='%s' (%d tries)\n", game.PLID, game.word, game.trial);
	return sprintf( message, "RQT OK\n" );
}


int udp_receive_command() {
	char message[MAX_SIZE];

	// Clean previous message
	udp_client_addrlen = sizeof(udp_client_addr);
	memset( &message, 0, MAX_SIZE );

	if ( recvfrom(fd_udp_socket, message, MAX_SIZE, 0, &udp_client_addr, &udp_client_addrlen) == -1 )
		return EXIT_FAILURE;


	// The second letter on any player message is unique
	switch ( message[1] ) {
		case 'N': // SNG
			start_new_game(message);
			break;
		case 'L': // PLG
			player_guess_letter(message);
			break;
		case 'W': // PWG
			player_guess_word(message);
			break;
		case 'U': // QUT
			player_quit_game(message);
			break;
		default:
			strcpy(message, "ERR\n");
	}

	if ( sendto(fd_udp_socket, message, strlen(message), 0, &udp_client_addr, udp_client_addrlen) == -1 )
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}

void udp_server_main(const char *port_num) {
	// Must be done on all processes that need to catch SIGINT
	signal(SIGINT, udp_sig_handler);

	if ( udp_server_setup(port_num) == EXIT_FAILURE )
		return;

	while ( !udp_interrupted ) {
		udp_receive_command();
		sleep(1);
	}

	close(fd_udp_socket);
}
