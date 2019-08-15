#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#define ALLOWED_SIZE 27

// Function prototypes
void printCharArray(char*);
void fillKey(char*, int, char*);

int main(int argc, char* argv[]) {	
	if (argc > 2) { // Check for appropriate number of arguements
		perror("You entered in too many arguements!\n");
		exit(1);
	}
	char allowedChars[ALLOWED_SIZE] = {
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K',
		'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
		'W', 'Y', 'X', 'Z', ' '};	
	int length = atoi(argv[1]);
	if (length < 1) {
		perror("Enter a key length greater than 0!\n");
		exit(1);
	}
	char key[length + 1]; // Length of key array is in argv[1]
	memset(key, '\0', sizeof(key)); // Clear out the key array

	fillKey(key, length, allowedChars);
	//printCharArray(key);
	printf("%s\n", key);
	return 0;
} 

// Prints out each character of the passed in array on its own line
// Used for testing
void printCharArray(char* arr) {
	int i;
	for (i = 0; i < strlen(arr); i++) {
		printf("%c\n", arr[i]);
	}
}

// Fills the key with random letters
void fillKey(char* key, int length, char* allowedChars) {
	srand(time(0)); // Seeds the random number generator
	int i;
	for (i = 0; i < length; i++) {
		key[i] = allowedChars[rand() % 27];
	}
}
