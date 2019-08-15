#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h> 

#define MAX_SIZE 1000

// Function prototypes
char* readFile(long*, char*);
void error(const char*);
int checkConnection(int);

int main(int argc, char *argv[])
{
	int socketFD, portNumber, charsWritten, charsRead;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	char* plaintext; // will allocate dynamically later
	char* key; // will allocate dynamically later
	long plainLength = -5;
	long keyLength = -5;    

	if (argc < 4) {
		fprintf(stderr,"USAGE: %s plaintext key port\n", argv[0]);
		exit(0); 
	} // Check usage & args

	// Set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverHostInfo = gethostbyname("localhost"); // Convert the machine name into a special form of address
	if (serverHostInfo == NULL) {
		fprintf(stderr, "CLIENT: ERROR, no such host\n");
		exit(0);
	}
	// Copy in the address
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length);

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0) 
		error("CLIENT: ERROR opening socket");
	
	// Connect to server and connect socket to address
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0)
		error("CLIENT: ERROR connecting");

	// Check to make sure connection with otp_enc_d
	if (checkConnection(socketFD) == 0) {
		fprintf(stderr, "Error: could not contact otp_enc_d on port %s\n", argv[3]);
		exit(2);
	}

	// Get the plaintext from the plaintext file
	plaintext = readFile(&plainLength, argv[1]);
	
	// Get the key from the key file
	key = readFile(&keyLength, argv[2]);
	
	// Check if plaintext is longer than key
	if (plainLength > keyLength) {
		fprintf(stderr, "Error: key \'%s\' is too short\n", argv[2]);
		exit(1);
	}
	
	// Send the plaintext length to the server
	charsWritten = send(socketFD, &plainLength, sizeof(plainLength), 0);
	if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
	int totalWritten = 0; // reset for loop		
	// Will send plaintext in chunks if necessary
	while (totalWritten <= plainLength) {
		char copy[MAX_SIZE];
		memset(copy, '\0', sizeof(copy)); // Clear out copy array
		// Copies from where last iteration left off
		strncpy(copy, &plaintext[totalWritten], MAX_SIZE - 1);
		// Send plaintext to server
		charsWritten = send(socketFD, &copy, sizeof(copy), 0); // Write to the server
		if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
		totalWritten += charsWritten - 1;
	}

	//sleep(1); // NEED TO FIND A BETTER WAY TO WAIT IN BETWEEN THESE
	
	// Send the key length to the server
	charsWritten = send(socketFD, &keyLength, sizeof(keyLength), 0);
	if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
	totalWritten = 0; // reset for loop			
	// Will send key in chunks if necessary
	while (totalWritten <= keyLength) {
		char copy[MAX_SIZE];
		memset(copy, '\0', sizeof(copy)); // Clear out copy array
		// Copies from where last iteration left off
		strncpy(copy, &key[totalWritten], MAX_SIZE - 1);
		// Send the key to the server
		charsWritten = send(socketFD, &copy, sizeof(copy), 0); // Write to the server
		if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
		totalWritten += charsWritten - 1;
	}

	// Get ciphertext from server
	char ciphertext[plainLength];
	memset(ciphertext, '\0', sizeof(ciphertext)); // Clear out the buffer again for reuse
	// Read data from socket leaving \0 at the end
	int totalRead = 0;
	// Will receive in chunks if necessary
	while (totalRead <= plainLength) {
		char buffer[MAX_SIZE];
		memset(buffer, '\0', MAX_SIZE); // clear buffer
		charsRead = recv(socketFD, &buffer, MAX_SIZE, 0);
		if (charsRead < 0) error("CLIENT: ERROR reading from socket");
		strncat(ciphertext, buffer, charsRead); // concat string
		totalRead += (charsRead - 1);
	}
	printf("%s\n", ciphertext); // print out ciphertext to stdout

	// Do clean up
	close(socketFD); // Close the socket
	free(plaintext);
	free(key);

	return 0;
}

// Returns the first line of the file referred to by fileName
char* readFile(long* fileLength, char* fileName) {
	int file = open(fileName, O_RDONLY);
	// Check to see if file was opened
	if (file == -1) {
		fprintf(stderr, "Could not open %s\n", fileName);
		exit(1);
	}
	// Get the length of the file
	*fileLength = lseek(file, 0, SEEK_END);
	// Reset file pointer to beginning of file
	lseek(file, 0, SEEK_SET); 
	// Dynamically allocate buffer based on file length
	char* buffer = malloc(sizeof(char) * (*fileLength));
	// Read the first line of the file
	if (read(file, buffer, *fileLength) == -1) {
		fprintf(stderr, "There is no content in file %s\n", fileName);
		exit(1);
	}
	buffer[(*fileLength) - 1] = '\0'; // Remove the trailing \n that fgets adds
	// Close the file
	close(file);
	// Check buffer for invalid characters
	int i = 0;
	while (buffer[i] != '\0') {
		if (((int)(buffer[i])) < 65 && ((int)(buffer[i])) != 32) {
			perror("otp_enc error: input contains bad characters\n");
			exit(1);	
		}
		else if (((int)(buffer[i])) > 90) {
			perror("otp_enc error: input contains bad characters\n");
			exit(1);
		}
		i++;
	}
	return buffer;
}

// Error function used for reporting issues
void error(const char *msg) {
	perror(msg);
	exit(0);
}

// Returns 1 if connections is with otp_enc_d and 0 otherwise
int checkConnection(int connection) {
	int charsWritten = -1;
	char outgoing[] = "otp_enc";
	char incoming[100];
	memset(incoming, '\0', 100);
	charsWritten = send(connection, outgoing, strlen(outgoing), 0);
	if (charsWritten < 0) error("ERROR writing to socket");
	// Check if connection is to otp_enc
	charsWritten = recv(connection, incoming, 99, 0);
	if (charsWritten < 0) error("ERROR reading from socket");
	if (strcmp(incoming, outgoing) == 0) {
		return 1;
	}	
	return 0;
}
