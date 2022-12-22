
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <netdb.h>
#include <sys/socket.h>

#include <sys/wait.h>	// To let main wait for all children to stop
#include <sys/time.h>

#include "server.h"

// Set 1 for server to output every request
int is_verbose	= 1;

int num_words = 0;
char words[MAX_WORDS][MAX_WORD_SIZE];
char hints[MAX_WORDS][MAX_HINT_SIZE];


/*
 *	Converts string into number, errors on any
 *	non-number or if not within [start, end]
 *
 *	Inputs: snum, start, end
 *	Returns: 0 if error, num otherwise
 */
int is_valid_num(const char *snum, const int start, const int end) {
	int n = strlen(snum);
	int i, num = 0;

	for (i = 0; i < n; i++) {
		if (snum[i] < '0' || snum[i] > '9')
			return 0;

		num = num * 10 + (snum[i] - '0');
	}

	if (num < start || num > end)
		return 0;

	return num;
}


/*
 *	Removes newlines from message
 *
 *	Inputs: message
 */
char* strip_message(char *message) {
	int i, len = strlen(message);

	for (i = 0; i < len; i++) {
		if (message[i] == '\n') {
			message[i] = '\\';
			break;
		}
	}

	return message;
}


/*
 *	Inputs: argc, argv
 *	Outputs: verbose, word_file, port
 *	Returns: 1 if error, 0 otherwise
 */
int parse_args(const int argc, char **argv, int *verbose, char **word_file, char **port) {
	int c;
	int error = 0;

	//	Loops through all -(dash) arguments
	while ( (c = getopt(argc, argv, "p:v"))  != -1 )
	switch (c) {
		case 'p':
			*port = optarg;
			break;

		case 'v':
			*verbose = 1;
			break;

		default:
			error = 1;
	}

	//	Checks given port
	if ( !error && ! is_valid_num(*port, 1, 65535) ) {
		fprintf( stderr, "%s: port number must be between 1 and 65535 -- '%s'\n", argv[0], *port);
		error = 1;
	}

	//	Gets first non -(dash) argument
	if ( !error && !(*word_file = argv[optind]) ) {
		fprintf( stderr, "%s: argument is required -- 'word_file'\n", argv[0] );
		error = 1;
	}

	if (error)
		fprintf( stderr, "\nUsage: %s word_file [-p GSport] [-v]\n", argv[0] ); 

	return error;
}


/*
 *	Inputs: word_file
 *	Returns: 1 if error, 0 otherwise
 */
int setup_server_files(const char *word_file) {
	int max_line_size = MAX_WORD_SIZE + MAX_HINT_SIZE;
	char line[max_line_size];

	FILE *word_file_pointer = fopen( word_file, "r" );
	if ( !word_file_pointer ) {
		fprintf(stderr, "[ERROR]: Word file failed to load. Please try again.\n");
		exit(EXIT_FAILURE); 
	}

	while ( fgets( line, max_line_size, word_file_pointer ) ){
		char hint_path[7 + MAX_HINT_SIZE];
		char *word = strtok(line, " ");
		char *hint = strtok(NULL, " ");
		hint[strlen(hint) - 1] = '\0';

		strcpy( hint_path, "IMAGES/" );
		strcat( hint_path, hint );

		if ( access(hint_path, F_OK) ) {
			printf( "[ERROR] Word file contains errors. Hint file \"%s\" does NOT exist.\n", hint );
			exit(EXIT_FAILURE);
		}

		strcpy( words[num_words], word );
		strcpy( hints[num_words], hint );

		num_words++;
	}

	fclose( word_file_pointer );

	return 0;
}


/*
 *	Configures socket based on mode
 *
 *	Inputs: port_num, mode
 *	Returns: server_socket, exits if error
 */
int server_socket_setup(const char *port_num, const int socket_mode) {
	int fd_server_socket;
	char *mode = (socket_mode == SOCK_STREAM) ? "TCP" : "UDP";
	struct addrinfo hinted_addr, *found_addr;

	fd_server_socket = socket(AF_INET, socket_mode, 0);
	if ( fd_server_socket == -1 ) {
		fprintf( stderr, "[ERROR]: Failed to create %s socket. Please try again.\n", mode );
		return EXIT_FAILURE;
	}

	struct timeval tv;
	if ( socket_mode == SOCK_STREAM ) { // TCP
		tv.tv_sec = TIMEOUT_MS / 20; // instead of 0.1s of timeout, do 5s of timeout
		tv.tv_usec = 0;
	} else { // UDP
		tv.tv_sec = TIMEOUT_MS / 1000;
		tv.tv_usec = (TIMEOUT_MS % 1000) * 1000;
	}

	if ( setsockopt( fd_server_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0 ) {
		fprintf( stderr, "[ERROR]: Failed to set %s socket timeout. Please try again.\n", mode );
		return EXIT_FAILURE;
	}

	// Set up desired server address
	memset( &hinted_addr, 0, sizeof(hinted_addr) );	// clean hinted address
	hinted_addr.ai_family = AF_INET;				// IPv4
	hinted_addr.ai_socktype = socket_mode;			// mode
	hinted_addr.ai_flags = AI_PASSIVE;				// bind(able)

	// Find appropriate server address
	int err;
	if ( (err = getaddrinfo( NULL, port_num, &hinted_addr, &found_addr ) )) {
		fprintf( stderr, "[ERROR]: Failed %s address search. Please try again, errno=%d\n", mode, err );
		close(fd_server_socket);
		return EXIT_FAILURE;
	}

	// Bind the UDP socket to the server address found
	if ( bind( fd_server_socket, found_addr->ai_addr, found_addr->ai_addrlen ) == -1 ) {
		fprintf( stderr, "[ERROR]: Failed to bind %s address. Please try again\n", mode );
		close(fd_server_socket);
		return EXIT_FAILURE;
	}

	freeaddrinfo(found_addr); // Only needed for bind

	if ( (socket_mode == SOCK_STREAM) && (listen(fd_server_socket, MAX_SIZE) == -1) ) {
		fprintf( stderr, "[ERROR]: Failed to set TCP socket as passive. Please try again\n" );
		close(fd_server_socket);
		return EXIT_FAILURE;
	}

	//printf("%s socket all working\n", mode);

	return fd_server_socket;
}


int main(int argc, char **argv) {
	pid_t pid, wpid;
	char *word_file	= "(NULL)";
	char *port_num	= "58076";


	if ( parse_args(argc, argv, &is_verbose, &word_file, &port_num) )
		return 1;

	if ( is_verbose )
		printf("is_verbose=%s, port=%s, word_file=%s\n", (is_verbose ? "True" : "False" ), port_num, word_file);


	// Load and check word_file
	setup_server_files(word_file);


	// forks server to run TCP connections
	if ( (pid = fork()) == 0 )
		tcp_server_main(port_num);

	// runs UDP server
	else if (pid > 0)
		udp_server_main(port_num);

	else {
		fprintf( stderr, "[ERROR]: fork failed unexpectedly.\n" );
		exit(EXIT_FAILURE);
	}

	// Waits for all the child processes to finish
	while ( (wpid = wait(NULL)) > 0 );

	exit(EXIT_SUCCESS);
}
