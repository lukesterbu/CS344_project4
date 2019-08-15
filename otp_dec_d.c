#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_SIZE 1000

// FUNCTION PROTOTYPES
char* decrypt(char*, char*);
void error(const char*);
int checkConnection(int);

int main(int argc, char *argv[])
{
	int listenSocketFD, establishedConnectionFD, portNumber, charsRead;
	socklen_t sizeOfClientInfo;
	char* key; // will allocate dynamically later
	char* plaintext; // will allocate dynamically later
	char* ciphertext; // will allocate dynamically later
	char buffer[MAX_SIZE];
	struct sockaddr_in serverAddress, clientAddress;
	pid_t spawnPid = -5;
	int childExitStatus = -5;
	long cipherLength = -5;
	long keyLength = -5;
	
	if (argc < 2) {
		fprintf(stderr,"USAGE: %s port\n", argv[0]);
		exit(1);
	} // Check usage & args

	// Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

	// Set up the socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listenSocketFD < 0) 
		error("ERROR opening socket");

	// Enable the socket to begin listening
	// Connect socket to port
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
		error("ERROR on binding");
	listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections

	// Accept a connection, blocking if one is not available until one connects
	sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
	
	while(1) { // Infinite loop
		// Accept
		establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo);
		if (establishedConnectionFD < 0) error("ERROR on accept");

		spawnPid = fork();
		switch (spawnPid) {
			// Error
			case -1:
				perror("Hull Breach!");
				exit(1);
				break;
			// Child process
			case 0:
				// Check if we're connected to otp_dec
				if (checkConnection(establishedConnectionFD) == 0) exit(2);
				
				// Get the length of ciphertext from the client
				charsRead = recv(establishedConnectionFD, &cipherLength, sizeof(cipherLength), 0);
				if (charsRead < 0) error("ERROR reading from socket");
				
				ciphertext = malloc(sizeof(char) * (cipherLength + 1)); // allocate ciphertext
				memset(ciphertext, '\0', cipherLength); // clear ciphertext
				int totalRead = 0; // Reset for loop
				// Will receive ciphertext in chunks if necessary
				while (totalRead <= cipherLength) {
					// Get the ciphertext from the client
					memset(buffer, '\0', MAX_SIZE); // clear buffer
					charsRead = recv(establishedConnectionFD, &buffer, MAX_SIZE, 0);
					if (charsRead < 0) error("ERROR reading from socket");
					strncat(ciphertext, buffer, charsRead); // concat string
					totalRead += (charsRead - 1);
				}

				// Get the length of key from the client
				charsRead = recv(establishedConnectionFD, &keyLength, sizeof(keyLength), 0);
				if (charsRead < 0) error("ERROR reading from socket");
				
				key = malloc(sizeof(char) * (keyLength + 1)); // allocate key
				memset(key, '\0', keyLength); // clear key
				totalRead = 0; // Reset for loop
				// Will receive key in chunks if necessary
				while (totalRead <= keyLength) {
					// Get the key from the client
					memset(buffer, '\0', MAX_SIZE); // clear buffer
					charsRead = recv(establishedConnectionFD, &buffer, MAX_SIZE, 0);
					if (charsRead < 0) error("ERROR reading from socket");
					strncat(key, buffer, charsRead); // concat string
					totalRead += (charsRead - 1);
				}

				// Decrypt the ciphertext using the key
				plaintext = decrypt(ciphertext, key);

				// Send a Success message back to the client
				// may need to send this in chunks
				int charsWritten = 0;
				int totalWritten = 0;
				while (totalWritten <= cipherLength) {
					memset(buffer, '\0', MAX_SIZE); // clear buffer
					// Copies from where last iteration left off
					strncpy(buffer, &plaintext[totalWritten], MAX_SIZE - 1);
					// Send to client
					charsWritten = send(establishedConnectionFD, &buffer, sizeof(buffer), 0);	
					if (charsRead < 0) error("ERROR writing to socket");
					totalWritten += charsWritten - 1;
				}
				// Clean up
				free(plaintext);
				free(key);
				free(ciphertext);
				
				exit(0);
				break;
			// Parent process
			default:
				close(establishedConnectionFD);
				do {
					waitpid(spawnPid, &childExitStatus, 0);
				} while(!WIFEXITED(childExitStatus) && !WIFSIGNALED(childExitStatus));
				break;
		}
	}
	close(listenSocketFD);
	return 0; 
}

// Returns ciphertext given plaintext and a key
char* decrypt(char* ciphertext, char* key) {
	char* plaintext = malloc(sizeof(char) * strlen(ciphertext));
	int i;
	// Clear out the ciphertext variable
	memset(plaintext, '\0', sizeof(plaintext));
	for (i = 0; i < strlen(ciphertext); i++) {
		int a = 0, b = 0, c = 0;
		// This is the plaintext letter
		if (((int)(ciphertext[i])) == 32) // if equal to space
			a = 26;
		// If it is one of the capital letters of the alphabet
		else if (((int)(ciphertext[i])) >= 65 && ((int)(ciphertext[i])) <= 90)
			a = (int)(ciphertext[i]) - 65; // zero index A
		// This is the key letter
		if (((int)(key[i])) == 32) // if equal to space
			b = 26;
		// If it is one of the capital letters of the alphabet
		else if (((int)(key[i])) >= 65 && ((int)(key[i])) <= 90) // if not equal to space
			b = (int)(key[i]) - 65;	
		// Calculate new letter
		c = a - b;
		// Mod by 27 depending on c's value
		if (c > 26)
			c -= 27;
		else if (c < 0)
			c += 27;
		// Convert back to ASCII
		if (c == 26)
			c = 32;
		else if (c >= 0 && c <= 25)
			c += 65;
		plaintext[i] = (char)(c);			
	}
	return plaintext;
}

// Error function used for reporting issues
void error(const char *msg) {
	perror(msg);
	exit(1);
}

// Returns 1 if connection is with otp_enc and 0 otherwise
int checkConnection(int connection) {
	int charsRead = -1;
	char outgoing[] = "otp_dec";
	char incoming[100];
	memset(incoming, '\0', 100);
	charsRead = send(connection, outgoing, strlen(outgoing), 0);
	if (charsRead < 0) error("ERROR writing to socket");
	// Check if connections equals otp_enc
	charsRead = recv(connection, incoming, 99, 0);
	if (charsRead < 0) error("ERROR reading from socket");
	if (strcmp(incoming, outgoing) == 0) {
		return 1;
	} 
	return 0;
}
