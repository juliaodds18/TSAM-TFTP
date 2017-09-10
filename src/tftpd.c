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
#include <signal.h>


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

// Struct for sending data
typedef struct {
    uint16_t opcode;
    uint16_t block;
    uint8_t data[512];
} sendData;

// Struct to sending error
typedef struct {
    uint16_t opcode;
    uint16_t errorCode;
    uint8_t errMsg[512];
    uint8_t nullTerm;
} sendError;

// Enum for error codes
typedef enum {
    notDefined = 0,
    fileNotFound,
    accessViolation,
    allocExceeded,
    illegalOp,
    unknownTID,
    fileExists,
    noSuchUser
} errorCode;

// Variables 
char message[516];
FILE *file;
unsigned int blockNumber;
unsigned int sockfd;
int dataSendSize;
sendData d;


/* * * * * * * * * * * * *
        Functions
 * * * * * * * * * * * * */

void send_error(char *errMsg, uint16_t errCode)
{
    // If the filestream is open, close it 
    if(file != NULL) {
        fclose(file);
        file = NULL;
    }

    // set the opcode, the error code, the error message and the null byte in the end
    sendError e;
    e.opcode = htons(ERROR);
    e.errorCode = htons(errCode);
    unsigned int sizeOfMessage = sizeof(errMsg);
    memcpy(e.errMsg, errMsg, sizeOfMessage);
    e.nullTerm = 0;

    // Send the error message to the client
    sendto(sockfd, &e, sizeOfMessage+5, 0, (struct sockaddr *) &client, (socklen_t) sizeof(client));
}

void send_data()
{
    // Empty the struct 
    // sendData d = {0};

    // set the opcode and the block number of the packet
    d.opcode = htons(DATA);
    d.block = htons(blockNumber);

    // Read max 512 bytes from the file
    dataSendSize = fread(d.data, 1, sizeof(d.data), file);
   
    // Send the packet to the client
    sendto(sockfd, &d, dataSendSize+4, 0, (struct sockaddr *) &client, (socklen_t) sizeof(client));
   
    // Check if it was the last packet, close the filestream
    if (dataSendSize < 512) {
        fclose(file);
        file = NULL;
    }
}

// A signal handler, in order to properly close any open file 
void signal_handler(int sig) {
    if (sig == SIGINT) {
	fprintf(stdout, "Caught SIGINT, shutting down the connection\n"); 
	fflush(stdout);

	if (file != NULL) {
	    fclose(file); 
	    file = NULL; 
	}
	exit(0); 
    }
}


int main(int argc, char *argv[])
{
    // If parameters are strictly less than one, there are no userful parameters and nothing to be done.
    if (argc != 3) {
        fprintf(stdout, "Wrong number of parameters, must be: %s, <port_number>, <directory_name>", argv[0]);
        return 0;
    }
    
    //Create a signal handler, in order to close the file in case of an interrupt 
    if (signal(SIGINT, signal_handler) == SIG_ERR) {
	fprintf(stdout, "Cannot catch SIGINT\n");
	fflush(stdout); 
    }

    unsigned int port, modeptr;
    char *mode, *filename, *directory, *checkFile;
    char filepath[255], actualpath[PATH_MAX];
    
    // Get the port number from parameters
    sscanf(argv[1], "%d", &port);

    // Get the name of the directory from parameters
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
 
    // Check if the bindin is successful, else send error message to client
    if (bind(sockfd, (struct sockaddr *) &server, (socklen_t) sizeof(server)) < 0) {
        send_error("The transefer ID does not match", unknownTID);
    }

    
    for (;;) {

        // Receive up to one byte less than declared, will be NULL-terminated later
        socklen_t len = (socklen_t) sizeof(client);
    
	ssize_t n = recvfrom(sockfd, message, sizeof(message) - 1,
                             0, (struct sockaddr *) &client, &len);
        
	// Check if the revfrom was successful
	if(n < 0) {
	    send_error("Could not receive messages", notDefined);    	    
        }

        message[n] = '\0';

	 
        unsigned int opcode = message[1];
	unsigned int ackBlockNumber;

        switch (opcode) {
            case RRQ:
       		// If the server receives a read request, find the path from where to get the file, read it and send it over to the client.

                // Loop through message, in order to get to the string denoting mode
                for (modeptr = 2; message[modeptr] != '\0'; modeptr++) {}
                modeptr++;
		mode = &message[modeptr];

		// Get the filename the client is attempting to fetch
                filename = &message[2];

		// Check if the mode is right, else send an error message 
		if(strcmp(mode, "netascii") == 0 && strcmp(mode, "octat") == 0) {
		    send_error("Illegal mode",illegalOp);
                }

		//Get the path to the file the client is attempting to fetch
                memset(filepath, 0, sizeof(filepath));
                strncpy(filepath, directory, strlen(directory));
                strcat(filepath, "/");
                strncat(filepath, filename, sizeof(filepath) - strlen(filename));
                checkFile = realpath(filepath, actualpath);

                // Check if the path is under the directory, if not, send error message
                if (checkFile == NULL) {
                    send_error("File not found", fileNotFound);
                    break;
                }

		// Get the IP from the client
	 	int fourthIp = client.sin_addr.s_addr >> 24;
		int thirdIp = (client.sin_addr.s_addr >> 16) & 0xff;
		int secondIp = (client.sin_addr.s_addr >> 8) & 0xff;
		int firstIp = client.sin_addr.s_addr & 0xff;
	
		// Print the information from the client
		fprintf(stdout, "file \"%s\" requested from %d.%d.%d.%d:%d\n", filename, firstIp, secondIp, thirdIp, fourthIp, port);
                fflush(stdout);

		// Set the block number for the first packet
		blockNumber = 1;

		// Open the file that the client requested
		file = fopen(filepath, "r");
		send_data();
                break;

            case WRQ:
                // Write requests forbidden, send error message
                send_error("Write requests are forbidden", accessViolation);

                break;

            case DATA:
                // Illegal operation, cannot upload to server, send error message
            	send_error("Illegal operation, uploading is not allowed", accessViolation);
                break;

            case ACK:
		// Get the block number of the ack message
   	        ackBlockNumber = (((uint8_t*)message)[2] << 8) + ((uint8_t*)message)[3];		
		
		// Check if the ack block number and the last block number match. If not, send the packet again
		if(ackBlockNumber != blockNumber) {
		    sendto(sockfd, &d, dataSendSize+4, 0, (struct sockaddr *) &client, (socklen_t) sizeof(client));
		}

		// If the file is not empty send next packet
                if(file != NULL){
		    blockNumber++;
                    send_data();
                }
                
                break;

            case ERROR:
		// Print the error message from the clinet
                fprintf(stdout, "Error message from client: %s\n", &message[4]);
		fflush(stdout);

		// Close the file stream if it is still open 
                if (file != NULL) { 
                    fclose(file);
                    file = NULL;
                }
                break;

            default:
		// Send error messaeg if the opcode is wrong
                send_error("Illegal opcode", notDefined);
        }
    }
    return 0;

}

