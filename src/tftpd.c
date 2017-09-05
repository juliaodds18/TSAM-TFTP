/* your code goes here. */
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
/*
 * Returns a random PORT number between 49152 and 65535
 */

/* MIN and MAX portnunbers */
int MINPORT = 49152;
int MAXPORT = 65535;

int randomChosenTID(){
        return (rand()%(MAXPORT-MINPORT))+MINPORT;
}

int main(int argc, char *argv[])
{
	int port = randomChosenTID();
        int sockfd;
	//printf("The port number is: %d", port);
	return 0;
}

// The output of the server should list which file is requested from which IP and portb
// // the server should only send files that are inside the directory given as command line argument
// // The RRQ and WRQ have a mode string
