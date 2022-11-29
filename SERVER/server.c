
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main (int argc, char **argv) {
	int verbose		= 0;
	char *word_file	= NULL;
	char *port		= "58076";

	int c;

	while ( (c = getopt(argc, argv, "p:v"))  != -1 )
		switch (c) {
			case 'p':
				port = optarg;
				break;
			case 'v':
				verbose = 1;
				break;
			default:
				fprintf( stderr, "\nUsage: %s word_file [-p GSport] [-v]\n", argv[0] ); 
				return 1;
		}

	if ( !(word_file = argv[optind]) ) {
		fprintf( stderr, "%s: argument is required -- 'word_file'\n"
					"\nUsage: %s word_file [-p GSport] [-v]\n", argv[0], argv[0] );
		return 1;
	}

	printf( "verbose=%d, port=%s, word_file=%s\n", verbose, port, word_file);
	
	//for (c = optind; c < argc; c++)
	//	printf( "Non-option argument %s\n", argv[c] );
	


	return 0;
}
