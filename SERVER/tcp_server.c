
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h> // To catch Ctrl-C

#include <netdb.h>
#include <sys/socket.h>

#include "server.h"

// This is the TCP server socket
int fd_tcp_socket;

// These are used for the client connection.
socklen_t tcp_client_addrlen;
struct sockaddr tcp_client_addr;

extern int is_verbose;

//extern char words[MAX_WORDS][MAX_WORD_SIZE];
//extern char hints[MAX_WORDS][MAX_HINT_SIZE];

static volatile sig_atomic_t tcp_interrupted = 0;

/*
 *	Catches Ctrl-C to safely stop everything
 *
 *	Argument required but isn't used, no need to name it
 */
void tcp_sig_handler(int) {
	tcp_interrupted = 1;
}

/*
 *	Configures TCP socket
 *
 *	Inputs: port_num
 *	Returns: fd_tcp_socket, exits if error
 */
int tcp_server_setup(const char *port_num) {
	int fd_tcp_socket;

	fd_tcp_socket = server_socket_setup(port_num, SOCK_STREAM);
	if ( fd_tcp_socket == EXIT_FAILURE )
		exit(EXIT_FAILURE);

	return fd_tcp_socket;
}

/*
 *	Reads entire message from the socket.
 *
 *	Inputs: socket
 *	Outputs: message
 *	Returns: n bytes read, -1 if error
 */
int read_message(int socket, char message[MAX_SIZE]) {
	ssize_t n = 0, total = 0, end = 0;
	char buffer[MAX_SIZE];

	while ( !end ) {
		memset( buffer, 0, MAX_SIZE );
		n = read(socket, buffer, MAX_SIZE);

		if ( buffer[n - 1] == '\n' )
			end = 1;

		memcpy( message + total, buffer, n);
		total += n;
	}

	return total;
}

/*
 *	Writes entire message from the socket.
 *
 *	Inputs: socket, message
 *	Returns: n bytes written, -1 if error
 */
int write_message(int socket, char *message, ssize_t size) {
	ssize_t total = 0;

	while ( total < size )
		total += write(socket, message, size);

	return total;
}

int send_scoreboard(int socket, char *message) {
	return 0;
	/*
	struct game game;
	char PLID[PLID_SIZE + 1] = "\0";
	char game_path[12 + PLID_SIZE] = "\0";

	// Check if ay games exist, EMPTY if not
	if ( access(game_path, F_OK) )
		return write_message( socket, "RSB EMPTY\n", 10 );

	game = load_game(game_path);

	size_t bytes_written = 0;
	//char *buffer = (char *) calloc( MAX_HINT_SIZE * (MAX_TRIES + 3), sizeof(char) );

	// Check if last game is finished
	//if ( !game.status )

	if ( is_verbose )
		printf("PLID=%s: send scoreboard file 'SCOREBOARD.txt' (%ld bytes)\n", bytes_written);

	write_message( socket, message, strlen(message) );
	return write_message( socket, buffer, bytes_written );
	*/
}

int send_hint(int socket, char *message) {
	long n_bytes;
	FILE *hint_file;
	struct game game;
	char *buffer;
	char PLID[PLID_SIZE + 1] = "\0";
	char game_path[12 + PLID_SIZE] = "\0";
	char hint_path[7 + MAX_HINT_SIZE] = "\0";

	// Get message parameters
	if ( sscanf( message, "GHL %6c", PLID ) != 1 )
		return write_message( socket, "RHL NOK\n", 8 );

	// Test PLID
	if ( !is_valid_num(PLID, 1, 999999) )
		return write_message( socket, "RHL NOK\n", 8 );

	sprintf( game_path, "GAMES/GAME_%s", PLID );

	// Check if game exists, NOK if not
	if ( access(game_path, F_OK) )
		return write_message( socket, "RHL NOK\n", 8 );

	game = load_game(game_path);

	// Check if last game is finished
	if ( game.status )
		return write_message( socket, "RHL NOK\n", 8 );

	sprintf( hint_path, "IMAGES/%s", game.hint );
	hint_file = fopen( hint_path, "r" );

	fseek( hint_file, 0, SEEK_END );
	n_bytes = ftell(hint_file);
	fseek( hint_file, 0, SEEK_SET );

	size_t message_bytes = sprintf( message, "RHL OK %s %ld ", game.hint, n_bytes );
	buffer = (char *) calloc( (n_bytes), sizeof(char) );

	size_t bytes_read = 0;
	while ( bytes_read < n_bytes )
		bytes_read += fread(buffer + bytes_read, sizeof(char), (n_bytes - bytes_read), hint_file);
	
	fclose(hint_file);

	if ( is_verbose )
		printf("PLID=%s: send hint file '%s' (%ld bytes)\n", game.PLID, game.hint, n_bytes);

	write_message( socket, message, message_bytes );
	return write_message( socket, buffer, n_bytes );
}

int send_state(int socket, char *message) {
	struct game game;
	char PLID[PLID_SIZE + 1] = "\0";
	char game_path[12 + PLID_SIZE] = "\0";

	// Get message parameters
	if ( sscanf( message, "STA %6c", PLID ) != 1 )
		return write_message( socket, "RST NOK\n", 8 );

	// Test PLID
	if ( !is_valid_num(PLID, 1, 999999) )
		return write_message( socket, "RST NOK\n", 8 );

	sprintf( game_path, "GAMES/GAME_%s", PLID );

	// Check if game exists, NOK if not
	if ( access(game_path, F_OK) )
		return write_message( socket, "RST NOK\n", 8 );

	game = load_game(game_path);

	size_t bytes_written = 0;
	char *buffer = (char *) calloc( MAX_HINT_SIZE * (MAX_TRIES + 3), sizeof(char) );

	// Check if last game is finished
	if ( !game.status ) {
		bytes_written += sprintf( buffer + bytes_written, "  Active game found for player %s\n", game.PLID );
		bytes_written += sprintf( buffer + bytes_written, "     --- Transactions found: %d ---\n", game.trial - 1 );

		for ( int i = 0; i < game.trial - 1; i++ ) {
			if ( game.guesses[i][1] != '\0' ) {
				bytes_written += sprintf( buffer + bytes_written, "     Word guess: %s\n", game.guesses[i] );
			} else {
				int j;
				for (j = 0; game.letters_guessed[j] && game.letters_guessed[j] != game.guesses[i][0]; j++);

				if ( game.letters_guessed[j] )
					bytes_written += sprintf( buffer + bytes_written, "     Letter trial: %s - TRUE\n", game.guesses[i] );
				else
					bytes_written += sprintf( buffer + bytes_written, "     Letter trial: %s - FALSE\n", game.guesses[i] );
			}
		}

		bytes_written += sprintf( buffer + bytes_written, "     Solved so far: %s\n\n", game.current_state );

		sprintf( message, "RST ACT STATUS_%s.txt %ld ", game.PLID, strlen(buffer));
	} else {
		bytes_written += sprintf( buffer + bytes_written, "  Last finalized game for player %s\n", game.PLID );
		bytes_written += sprintf( buffer + bytes_written, "     Word: %s; Hint file: %s\n", game.word, game.hint );
		bytes_written += sprintf( buffer + bytes_written, "     --- Transactions found: %d ---\n", game.trial - 1 );

		for ( int i = 0; i < game.trial - 1; i++ ) {
			if ( game.guesses[i][1] != '\0' ) {
				bytes_written += sprintf( buffer + bytes_written, "     Word guess: %s\n", game.guesses[i] );
			} else {
				int j;
				for (j = 0; game.letters_guessed[j] && game.letters_guessed[j] != game.guesses[i][0]; j++);

				if ( game.letters_guessed[j] )
					bytes_written += sprintf( buffer + bytes_written, "     Letter trial: %s - TRUE\n", game.guesses[i] );
				else
					bytes_written += sprintf( buffer + bytes_written, "     Letter trial: %s - FALSE\n", game.guesses[i] );
			}
		}

		bytes_written += sprintf( buffer + bytes_written, "     Termination: %s\n\n", strcmp(game.current_state, game.word) ? "QUIT" : "WIN" );

		sprintf( message, "RST FIN STATUS_%s.txt %ld ", game.PLID, strlen(buffer));
	}

	if ( is_verbose )
		printf("PLID=%s: send state file 'STATUS_%s.txt' (%ld bytes)\n", game.PLID, game.hint, bytes_written);

	write_message( socket, message, strlen(message) );
	return write_message( socket, buffer, bytes_written );
}


int tcp_receive_command(int client_sock) {
	char message[MAX_SIZE];

	// Clean previous player message
	memset( &message, 0, MAX_SIZE );
	read_message(client_sock, message);

	// The second letter on any player message is unique
	switch ( message[1] ) {
		case 'S': // GSB
			send_scoreboard(client_sock, message);
			break;
		case 'H': // GHL
			send_hint(client_sock, message);
			break;
		case 'T': // STA
			send_state(client_sock, message);
			break;
		default:
			return write_message( client_sock, "ERR\n", 4 );
	}

	return EXIT_SUCCESS;
}

void tcp_server_main(const char *port_num) {
	pid_t pid;
	int client_sock;

	// Must be done on all processes that need to catch SIGINT
	signal(SIGINT, tcp_sig_handler);

	fd_tcp_socket = tcp_server_setup(port_num);

	while ( !tcp_interrupted ) {
		tcp_client_addrlen = sizeof(tcp_client_addr);

		// Wait for client. If error or timeout, retry
		if ( (client_sock = accept(fd_tcp_socket, &tcp_client_addr, &tcp_client_addrlen)) == -1 )
			continue;

		// fork, parent closes client sock and goes back to waiting
		// Child closes parent sock, communicates with client and exits
		if ( (pid = fork()) == 0 ) {
			close(fd_tcp_socket);
			tcp_receive_command(client_sock);
			close(client_sock);
			exit(EXIT_SUCCESS);
		}

		close(client_sock);
	}

	close(fd_tcp_socket);
	exit(EXIT_SUCCESS);
}
