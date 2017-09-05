/* your code goes here. */
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <ctype.h>
#include <string.h>


/* * * * * * * * * * * *
    Member Variables
 * * * * * * * * * * * */

// MIN and MAX portnunbers 
int MINPORT = 49152;
int MAXPORT = 65535;

// Structs for client and server
struct sockaddr_in server, client;

// Character array for message; 
char message[512]; 

/* * * * * * * * * * * *
        Functions
 * * * * * * * * * * * * */


int main(int argc, char *argv[])
{
    // If parameters are strictly less than one, there are no userful parameters 
    // and nothing to be done.
    if (argc < 1) {
        return 0; 
    } 

    int port;  
    sscanf(argv[1], "%d", &port);
    int sockfd;
    // printf("The port number is: %d", port);
	
    // Create and bind an UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0); 
    memset(&server, 0, sizeof(server)); 
    // Address family = IPv4, type of addresses that socket can communicate with
    server.sin_family = AF_INET; 
    
    // Convert host byte order to network byte order
    server.sin_addr.s_addr = htonl(INADDR_ANY); 
    server.sin_port = htons(port);
    bind(sockfd, (struct sockaddr *) &server, (socklen_t) sizeof(server));  
    
    for (;;) {

        // Receive up to one byte less than declared, will be NULL-terminated later
        socklen_t len = (socklen_t) sizeof(client); 
        ssize_t n = recvfrom(sockfd, message, sizeof(message) - 1, 
                             0, (struct sockaddr *) &client, &len); 
        message[n] = '\0'; 
        
        
    }


    return 0;
}

// The output of the server should list which file is requested from which IP and portb
// // the server should only send files that are inside the directory given as command line argument
// // The RRQ and WRQ have a mode string
