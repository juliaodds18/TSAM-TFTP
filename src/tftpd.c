/* your code goes here. */
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>

/* * * * * * * * * * * *
    Member Variables
 * * * * * * * * * * * */

// Macros for opcodes
#define RRQ   1
#define WRQ   2
#define DATA  3
#define ACK   4
#define ERROR 5

// Structs for client and server
struct sockaddr_in server, client;

// Character array for message; 
char message[516];
int RETRY = 4;
char messagesToSend[516];

/* * * * * * * * * * * *
        Functions
 * * * * * * * * * * * * */
int send_data(int sockfd)
{	 
    
     // int n = send data
     // if(n < 0) { return -1}
     return -1;
}

void read_data(char *filename, int sockfd, socklen_t len)
{
    // Open the file that the client asked for
    FILE *file;
    file = fopen(filename, "r");

    int done = 0;
    ssize_t checkIfError;
    int blockNumber = 0;
    int blockOfData;
    // loops while there is something to read
    while(done == 0) {
	// Read from the file that the client asked for
	blockNumber++;
        // Buffer starts reading in block 4 because of the header 
        int dataSendSize = 4;       

        while(dataSendSize < 516){
	    blockOfData = fgetc(file);
	    if(blockOfData == EOF) {
		break;
	    }
	    messagesToSend[dataSendSize] = (char)blockOfData;
	    dataSendSize++;
	}
	fprintf(stdout, "The size of the messagesToSend%d\n", dataSendSize);
	
        if(dataSendSize < 516) {
	     done = 1;
	}
	// If there is not ACk send the data again, but only 4 times
	for(int check = RETRY; check > 0; check--) {	
	    checkIfError = sendto(sockfd, messagesToSend, dataSendSize, 0, (struct sockaddr *) &client, (socklen_t) sizeof(client));
	
	    // If send_data returns -1 the data are not send
	    if(checkIfError < 0) {
		// send error
	    }
	    // If refrom returns -1 there is no ACK, try to send again or kill the connection
            checkIfError = recvfrom(sockfd, message, sizeof(message) - 1,
                             0, (struct sockaddr *) &client, &len);
	    fprintf(stdout, "Opcode: %u\n", message[1]);
   	    // If checkIfError returns ACK go out of the for loop
	    if(checkIfError <= 4) {
		break;
	    }
	    else {
	        // send error packet to client
	    }
	}	
	
    }

}


int main(int argc, char *argv[])
{
    // If parameters are strictly less than one, there are no userful parameters 
    // and nothing to be done.
    if (argc < 1) {
	fprintf(stdout, "Wrong number of parameters, must be: %s, <port_number>, <directory_name>", argv[0]); 
        return 0; 
    } 

    int port, modeptr, sockfd;  
    char *mode, *filename, *directory, *fileptr; 
    char filepath[255], actualpath[PATH_MAX];
    // Get the port number from parameters
    sscanf(argv[1], "%d", &port);
    
    //Get the name of the directory from parameters 
    directory = argv[2]; 
    fprintf(stdout, "Directory name: %s\n", directory); 
    fflush(stdout); 
	
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
        if(n < 0) {
	    // send error messages !!! 
	}

        message[n] = '\0'; 
        fprintf(stdout, "Received: \n%s\n", message);
        fflush(stdout);       

	unsigned int opcode = message[1]; 
        fprintf(stdout, "Opcode: %u\n", opcode);
        fflush(stdout);  

        switch (opcode) {
            case RRQ:  
		// Should we send ACK packet now? Establish connection???

		//Loop through message, in order to get to the string denoting mode  
		for (modeptr = 2; message[modeptr] != '\0'; modeptr++) {}
		modeptr++; 
		mode = &message[modeptr];
		filename = &message[2]; 
		
		//Get the path to the file the client is attempting to fetch 
		memset(filepath, 0, sizeof(filepath)); 		
		strncpy(filepath, directory, strlen(directory));
		strcat(filepath, "/"); 
		strncat(filepath, filename, sizeof(filepath) - strlen(filename)); 

		fprintf(stdout, "Mode: %s\n", mode); 
		fflush(stdout); 
 		
		fileptr = realpath(filepath, actualpath); 
		
		if (actualpath == NULL) {

		    fprintf(stdout, "Hello from realpath == null");
		    fflush(stdout); 
		    break; 
		}

		fprintf(stdout, "SUCCESS"); 
		fflush(stdout); 		
 		read_data(filepath, sockfd, len);	
		break; 
	
	    case WRQ: 
		// Write requests forbidden, send error message
		fprintf(stdout, "Illegal operation: Cannot write to server\n");
		 
		break; 

	    case DATA: 
		// Illegal operation, cannot upload to server, send error message 
		break; 

	    case ACK: 
		fprintf(stdout, "IM IN ACK");
	        // Received ACK message pack 	
		break; 

	    case ERROR: 

		break; 

        }	
    }
    return 0;

}
// The output of the server should list which file is requested from which IP and portb
// // the server should only send files that are inside the directory given as command line argument
// // The RRQ and WRQ have a mode string
