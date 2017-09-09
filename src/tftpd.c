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

typedef struct {
    uint16_t opcode;
    uint16_t block;
    uint8_t data[512];
} sendData;

// Character array for message; 
char message[516];
int RETRY = 4;
char messagesToSend[516];
FILE *file;
int blockNumber;
int sockfd;
/* * * *  * * * * * *
        Functions
 * * * * * * * * * * * * */
void send_data()
{
    int dataSendSize;
    // loops while there is something to read
    // Increase the block number
    blockNumber++;
    sendData d;
    d.opcode = htons(DATA);
    d.block = htons(blockNumber);

    // Read 512 instances of byte size  
    //fprintf(stdout, "The size of the file is: %d\n", (int)file);
    dataSendSize = fread(d.data, 1, sizeof(d.data), file);
    //fprintf(stdout, "The size of the messagesToSend%d\n", (int)dataSendSize);
      
    int checkIfError = sendto(sockfd, &d, dataSendSize+4, 0, (struct sockaddr *) &client, (socklen_t) sizeof(client));
    //fprintf(stdout, "return from sendto is: %d\n", checkIfError); 
    if(checkIfError < 0) {
        // send error
    }
    if (dataSendSize < 512) {
	fclose(file);
	file = NULL;
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

    int port, modeptr;  
    char *mode, *filename, *directory; 
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
   // bind(sockfd, (struct sockaddr *) &server, (socklen_t) sizeof(server));  
    if (bind(sockfd, (struct sockaddr *) &server, (socklen_t) sizeof(server)) < 0) {
	perror("Bind failed. Error");
	return 1;
    } 
    for (;;) {

        // Receive up to one byte less than declared, will be NULL-terminated later
        socklen_t len = (socklen_t) sizeof(client); 
        ssize_t n = recvfrom(sockfd, message, sizeof(message) - 1, 
                             0, (struct sockaddr *) &client, &len); 
        if(n < 0) {
	    // send error messages !!! 
	}

        message[n] = '\0'; 
        // fprintf(stdout, "Received: \n%s\n", message);
        fflush(stdout);       

	unsigned int opcode = message[1]; 
    //    fprintf(stdout, "Size of revfrom: %u\n", n);
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
 		
		realpath(filepath, actualpath); 
		
		if (actualpath == NULL) {

		    fprintf(stdout, "Hello from realpath == null");
		    fflush(stdout); 
		    break; 
		}

		fprintf(stdout, "SUCCESS"); 
		fflush(stdout); 		
		file = fopen(filepath, "r");
 		send_data();	
		break; 
	
	    case WRQ: 
		// Write requests forbidden, send error message
		fprintf(stdout, "Illegal operation: Cannot write to server\n");
		 
		break; 

	    case DATA: 
		// Illegal operation, cannot upload to server, send error message 
		break; 

	    case ACK:
		if(file != NULL){ 
		    send_data();
		}
	//	fprintf(stdout, "IM IN ACK");
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
