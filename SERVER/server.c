#include <stdio.h>
#include <string.h>

#define DEFAULT_PORT "58000" // + GN

int main(int argc, char **argv){
    char *filename, *port;
    int verbose;

    filename = argv[1];
    port = DEFAULT_PORT;
    verbose = 0;

    for(int i=3;i<argc;i++){    
        if(strcmp(argv[i],"-v")==0)
            verbose=1;
        else if(strcmp(argv[i-1],"-p")==0)
            port=argv[i];
    }

    printf("filename=%s port=%s verbose=%d\n",filename,port,verbose);

}