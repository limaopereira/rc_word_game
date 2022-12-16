
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h> // To catch Ctrl-C

#include "server_functions.h"
#include "../PLAYER/constants.h" // falta mudar a disposição dos ficheiros

static volatile sig_atomic_t interrupted = 0;

/*
 *	Catches Ctrl-C to safely stop everything
 *
 *	Argument required but isn't used, no need to name it
 */


void sig_handler() {
	close_server_udp_socket();
    // close_server_tcp_socket();
    exit(EXIT_SUCCESS);
}

/*
 *	Converts string into number, errors on any non-number or if above 65535
 *
 *	Inputs: port
 *	Returns: 0 if error, 1 otherwise
 */

// int is_valid_port(char *port) {
// 	int n = strlen(port);
// 	int i, num = 0;

// 	if (n > 5)
// 		return 0;
	
// 	for (i = 0; i < n; i++) {
// 		if (port[i] < '0' || port[i] > '9')
// 			return 0;

// 		num = num * 10 + (port[i] - '0');
// 	}

// 	// ports can only be between 1 and 65535
// 	// ports between 1 and 1023 are privileged
// 	if (num < 1 || num > 65535)
// 		return 0;

// 	return 1;
// }

/*
 *	Inputs: argc, argv
 *	Outputs: verbose, word_file, port
 *	Returns: 1 if error, 0 otherwise
 */

// int parse_args(int argc, char **argv, int *verbose, char **word_file, char **port) {
// 	int c;

// 	while ( (c = getopt(argc, argv, "p:v"))  != -1 )
// 		switch (c) {
// 			case 'p':
// 				if ( ! is_valid_port(optarg) ) {
// 					fprintf( stderr, "%s: port number must be between 1 and 65535 -- '%s'\n"
// 						"\nUsage: %s word_file [-p GSport] [-v]\n", argv[0], optarg, argv[0] );
// 					return 1;
// 				}
// 				*port = optarg;
// 				break;
// 			case 'v':
// 				*verbose = 1;
// 				break;
// 			default:
// 				fprintf( stderr, "\nUsage: %s word_file [-p GSport] [-v]\n", argv[0] ); 
// 				return 1;
// 	}

// 	if ( !(*word_file = argv[optind]) ) {
// 		fprintf( stderr, "%s: argument is required -- 'word_file'\n"
// 					"\nUsage: %s word_file [-p GSport] [-v]\n", argv[0], argv[0] );
// 		return 1;
// 	}

// 	return 0;
// }




/*{ // UDP SERVER 


file_descriptor_udp_socket = socket(AF_INET, SOCK_DGRAM, 0); //UDP socket

if ( file_descriptor_udp_socket == -1 ) exit(1); // socket failed to open


memset( &hints, 0, sizeof hints );
hints.ai_family		= AF_INET;		// Sets IPv4
hints.ai_socktype	= SOCK_DGRAM;	// Sets UDP
hints.ai_flags		= AI_PASSIVE;	// Sets Server mode (maybe)


errcode = getaddrinfo( NULL, PORT, &hints, &res);
if ( errcode != 0 ) exit(1); // failed to get address


n = bind( fd, res->ai_addr, res->ai_addrlen );
if ( n == -1 ) exit(1); // failed to set socket to address


while (1){
addrlen=sizeof(addr);
n=recvfrom(fd,buffer,128,0,
(struct sockaddr*)&addr,&addrlen);
if(n==-1) exit(1);
write(1,"received: ",10);write(1,buffer,n);
n=sendto(fd,buffer,n,0,
(struct sockaddr*)&addr,addrlen);
if(n==-1) exit(1);
}


freeaddrinfo(res); // releases address
close(fd); // closes socket


}*/





int main (int argc, char **argv) {
	// int is_verbose	= 0;
	// char *word_file	= "(NULL)";
	// char *port_num	= "58076";
	// //pid_t pid;

	signal(SIGINT, sig_handler);

	// if ( parse_args(argc, argv, &is_verbose, &word_file, &port_num) )
	// 	return 1;

	// printf("is_verbose=%s, port=%s, word_file=%s\n", (is_verbose ? "True" : "False" ), port_num, word_file);


	// while ( !interrupted ) {
	// 	sleep(1);
	// }

	// /*
	// for (int i = 0; i < 10; i++) {
	// 	if( (pid=fork()) == -1 ) exit(1);
	// 	else if ( !pid ) {
	// 		exit(0);
	// 	}
	// }
	// */
    pid_t pid;

    parse_server_args(argc,argv);
    load_word_file();
    
    pid = fork();

    if(pid == 0){
        // open_server_tcp_socket();
        // handle_server_tcp_requests();
    }
    else if (pid > 0){
        open_server_udp_socket();
        handle_server_udp_requests();
        close_server_udp_socket();
    }
    else{
        fprintf(stderr,"ERROR: Failed to fork GSport. Please try again\n");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);

}
