#include <stdio.h>		// to use printf, perror, fgets
#include <stdlib.h>		// to use exit
#include <string.h>		// to use strcspn, strtok

#define BUFFER_SIZE 10240
#define READ_ERROR -1

int femtoshell_main(int argc, char *argv[]) {

	char input[BUFFER_SIZE];
    int last_status=0;
	while (1) {
		printf("%s$ ", argv[0]);
		if (fgets(input, BUFFER_SIZE, stdin) == NULL) {
			if (feof(stdin)) {
				printf("\n"); // mimic shell behavior on EOF
				break;
			}
			else {
				perror("Error reading input");
				exit(READ_ERROR);
			}
		}
		
        printf("\n");
		// remove trailing newline
		input[strcspn(input, "\n")] = 0;

		char* command = strtok(input, " "); // split using space into tokens
		if (command == NULL) {
			continue; // empty input
		}

		if (strcmp(command, "exit") == 0) {
			printf("Good Bye\n");
			break;
		}
		else if (strcmp(command, "echo") == 0) {
			char* argument = strtok(NULL, " "); // get next token
			while (argument != NULL) {
				printf("%s", argument);
				char* next_argument = strtok(NULL, " "); // get next token
				if (next_argument != NULL) {
					printf(" ");
				}
				argument = next_argument;
			}
			printf("\n");
			last_status=0; // echo succeeded
		}
		else {
			printf("Invalid command\n");
			last_status=1; // failure
		}
	}

	return last_status;
}
