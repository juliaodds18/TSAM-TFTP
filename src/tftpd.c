/* your code goes here. */
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <ctype.h>
#include <string.h>
/*
 * Returns a random PORT number between 49152 and 65535
 */

/***********
Member Variables
***********/

/* MIN and MAX portnunbers */
int MINPORT = 49152;
int MAXPORT = 65535;

//Structs for client and server
struct sockaddr_in server, client;

int randomChosenTID(){
    return (rand()%(MAXPORT-MINPORT))+MINPORT;
}

int main(int argc, char *argv[])
{
    if (argc < 1) {
        return 0; 
    } 
 
    int port = randomChosenTID();
    int sockfd;
    //printf("The port number is: %d", port);
	
    //Create and bind an UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0); 
    memset(&server, 0, sizeof(server)); 
    server.sin_family = AF_INET; 
    
    //Convert host byte order to network byte order
    server.sin_addr.s_addr = htonl(INADDR_ANY); 
    server.sin_port = htons(randomChosenTID()); 
    return 0;
}

// The output of the server should list which file is requested from which IP and portb
// // the server should only send files that are inside the directory given as command line argument
// // The RRQ and WRQ have a mode string
