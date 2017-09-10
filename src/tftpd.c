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

// Character array for message;
char message[516];
FILE *file;
unsigned int blockNumber;
unsigned int sockfd;
int dataSendSize;
sendData d;
/* * * *  * * * * * *
        Functions
 * * * * * * * * * * * * */
void send_error(char *errMsg, uint16_t errCode)
{
    if(file != NULL) {
        fclose(file);
        file = NULL;
    }
    sendError e;
    e.opcode = htons(ERROR);
    e.errorCode = htons(errCode);
    unsigned int sizeOfMessage = sizeof(errMsg);
    memcpy(e.errMsg, errMsg, sizeOfMessage);
    e.nullTerm = 0;
    sendto(sockfd, &e, sizeOfMessage+5, 0, (struct sockaddr *) &client, (socklen_t) sizeof(client));
}

void send_data()
{
    // sendData d;
    sendData d = {0};
    d.opcode = htons(DATA);
    d.block = htons(blockNumber);
    dataSendSize = fread(d.data, 1, sizeof(d.data), file);
   

//fprintf(stdout, "The size of the messagesToSend%d\n", (int)dataSendSize);

    sendto(sockfd, &d, dataSendSize+4, 0, (struct sockaddr *) &client, (socklen_t) sizeof(client));
   
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

    unsigned int port, modeptr;
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
        send_error("The transefer ID does not match", unknownTID);
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

        unsigned int opcode = message[1];
	unsigned int ackBlockNumber;

        switch (opcode) {
            case RRQ:
                // Should we send ACK packet now? Establish connection???

                //Loop through message, in order to get to the string denoting mode
                for (modeptr = 2; message[modeptr] != '\0'; modeptr++) {}
                modeptr++;
		mode = &message[modeptr];
                filename = &message[2];
		fprintf(stdout, "Mode is: %s\n", mode);
		if(strcmp(mode, "netascii") == 0 && strcmp(mode, "octat") == 0) {
		    send_error("Illegal mode",illegalOp);
                }

		//Get the path to the file the client is attempting to fetch
                memset(filepath, 0, sizeof(filepath));
                strncpy(filepath, directory, strlen(directory));
                strcat(filepath, "/");
                strncat(filepath, filename, sizeof(filepath) - strlen(filename));

                realpath(filepath, actualpath);

                // Check if the path is under the directory
                if (actualpath == NULL) {
                    send_error("File not found", fileNotFound);
                    break;
                }
	 	int fourthId = client.sin_addr.s_addr >> 24;
		int thirdId = (client.sin_addr.s_addr >> 16) & 0xff;
		int secondId = (client.sin_addr.s_addr >> 8) & 0xff;
		int firstId = client.sin_addr.s_addr & 0xff;
	
		fprintf(stdout, "file \"%s\" requested from %d.%d.%d.%d:%d\n", filename, firstId, secondId, thirdId, fourthId, port);
                fflush(stdout);
		blockNumber = 1;
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
		// Get the block number of the ack
   	        ackBlockNumber = (((uint8_t*)message)[2] << 8) + ((uint8_t*)message)[3];		
		// If the last ack block number doesn't mach the last block number, send the packet again
		if(ackBlockNumber != blockNumber) {
		    sendto(sockfd, &d, dataSendSize+4, 0, (struct sockaddr *) &client, (socklen_t) sizeof(client));
		}
		// If the file is not empty send next  packet
                if(file != NULL){
		    blockNumber++;
                    send_data();
                }
                
                break;

            case ERROR:
                fprintf(stdout, "Error message from client: %s\n", &message[4]);
                if (file != NULL) { //Close datastream, if file is open.
                    fclose(file);
                    file = NULL;
                }
                break;
            default:
                send_error("Illegal opcode", notDefined);
        }
    }
    return 0;

}
// The output of the server should list which file is requested from which IP and portb
// // the server should only send files that are inside the directory given as command line argument
// // The RRQ and WRQ have a mode string
