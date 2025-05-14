#include <stdlib.h>	// to use malloc, realloc, exit, getenv
#include <stdio.h>	// to use getchar, EOF, perror, fflush
#include <string.h>	// to use strlen, strtok, strcmp
#include <unistd.h>     // to use write, getcwd, fork, execvp, chdir, isatty
#include <sys/types.h>	// to use pid_t
#include <sys/wait.h>	// to use wait


#define READ_ERROR 	-1
#define WRITE_ERROR 	-2
#define ARGUMENT_ERROR 	-3
#define GETCWD_ERROR  	-4
#define WAIT_ERROR	-5
#define CHILD_ERROR	-6
#define FORK_ERROR	-7
#define EXEC_ERROR	-8
#define CHDIR_ERROR	-9
#define GETENV_ERROR	-10


#define MAX_ARGS 150


char* read_line();
int echo(int argc, char* argv[]);
int pwd(int argc);
int cd(int argc, char* argv[]);


int picoshell_main(int argc, char *argv[]) {


	int last_status = 0; // Track the last command status

	while (1) {
		int is_interactive = isatty(STDIN_FILENO);

		printf("Pico shell prompt > ");
		fflush(stdout);

		char* input_line = read_line();
		if (input_line == NULL) {
			printf("\n"); // mimic shell behavior on EOF
			break;
		}

		if (strlen(input_line) == 0) {
			if (!is_interactive) {
				printf("\n");
			}
			free(input_line);
			continue;
		}

		if (!is_interactive) {
			printf("\n");
			fflush(stdout);
		}

		char* newArgv[MAX_ARGS];
		int newArgc = 0;

		char* token = strtok(input_line, " ");
		while (token != NULL && newArgc < MAX_ARGS) {
			newArgv[newArgc++] = token;
			token = strtok(NULL, " ");
		}

		newArgv[newArgc] = NULL;

		if (strcmp(newArgv[0], "echo") == 0) {
			int value_returned = echo(newArgc, newArgv);
			if (value_returned < 0) {
				free(input_line);
				exit(value_returned);
			}
			last_status = value_returned;
			free(input_line);
			continue;
		}
		else if (strcmp(newArgv[0], "pwd") == 0) {
			int value_returned = pwd(newArgc);
			if (value_returned < 0) {
				free(input_line);
				exit(value_returned);
			}
			last_status = value_returned;
			free(input_line);
			continue;
		}
		else if (strcmp(newArgv[0], "cd") == 0) {
			int value_returned = cd(newArgc, newArgv);
			if (value_returned < 0 && value_returned != CHDIR_ERROR) {
				free(input_line);
				exit(value_returned);
			}
			last_status = value_returned;
			free(input_line);
			continue;
		}
		else if (strcmp(newArgv[0], "exit") == 0) {
			printf("Good Bye\n");
			free(input_line);
			break;
		}


		pid_t pid = fork();

		if (pid > 0) {
			// parent
			int status;
			if (wait(&status) < 0) {
				perror("Error in wait");
				free(input_line);
				exit(WAIT_ERROR);
			}
			if (WIFEXITED(status)) {
				last_status = WEXITSTATUS(status); // Capture the exit status of the last command
			}
			else {
				char* error_msg = "Child didn't exit normally\n";
				if (write(2, error_msg, strlen(error_msg)) < 0) {
					perror("Error in writing to standard error file");
					free(input_line);
					exit(WRITE_ERROR);
				}
				free(input_line);
				exit(CHILD_ERROR);
			}
		}
		else if (pid == 0) {
			// child
			int exec_return = execvp(newArgv[0], newArgv);
			// if failed
			printf("%s: command not found\n", newArgv[0]);
			free(input_line);
			exit(exec_return);
		}
		else {
			perror("Error in fork");
			free(input_line);
			exit(FORK_ERROR);
		}

		free(input_line);
	}

	return last_status; // Return the status of the last command
}


char* read_line() {
	int size = 64; // initial size
	char* buffer = (char*)malloc(size);
	if (buffer == NULL) {
		perror("Unable to allocate buffer");
		return NULL;
	}

	int c;
	int len = 0;
	while (1) {
		c = getchar();

		// handles end of file
		if (c == EOF) {
			if (len == 0) {
				// nothing read before exit
				free(buffer);
				return NULL;
			}
			else {
				break;
			}
		}

		if (c == '\n') {
			break;
		}

		buffer[len++] = c;

		if (len >= size) {
			// resize
			size *= 2;

			char* new_buffer = (char*)realloc(buffer, size);
			if (new_buffer == NULL) {
				perror("Unable to reallocate buffer");
				free(buffer);
				return NULL;
			}

			buffer = new_buffer;
		}
	}
	buffer[len] = '\0'; // add null character at the end of the string
	return buffer;
}


int echo(int argc, char* argv[]) {
	for (int i = 1; i < argc; i++) {
		// write to stdout
		if (write(1, argv[i], strlen(argv[i])) < 0) {
			perror("Error in writing to stdout file");
			return (WRITE_ERROR);
		}

		// prints sapce between arguments
		if (i != argc - 1) {
			if (write(1, " ", 1) < 0) {
				perror("Error in writing to stdout file");
				return (WRITE_ERROR);
			}
		}
	}

	// prints newline at the end (even if no arguments are passed)
	if (write(1, "\n", 1) < 0) {
		perror("Error in writing to stdout file");
		return (WRITE_ERROR);
	}

	return 0;
}


int pwd(int argc) {
	if (argc > 1) {
		char* error_msg = "Error in calling pwd, can't add arguments more than command name, Usage: pwd \n";
		if (write(2, error_msg, strlen(error_msg)) < 0) {
			perror("Error in writing to standard error file");
			return (WRITE_ERROR);
		}
		return (ARGUMENT_ERROR);
	}

	char* cwd = getcwd(NULL, 0); // using NULL with cwd for automatic allocation (must free cwd)
	if (cwd == NULL) {
		char* error_msg = "Error in calling getcwd function \n";
		if (write(2, error_msg, strlen(error_msg)) < 0) {
			perror("Error in writing to standard error file");
			return (WRITE_ERROR);
		}
		// or simply call perror and it will also print errno (last system call error)
		// perror(error_msg);
		return (GETCWD_ERROR);
	}

	if ((write(1, cwd, strlen(cwd)) < 0) || (write(1, "\n", 1) < 0)) {
		perror("Error in writing to standard output file");
		return (WRITE_ERROR);
	}

	free(cwd);
	return 0;
}

int cd(int argc, char* argv[]) {
	if (argc > 2) {
		char* error_msg = "cd: too many arguments\n";
		if (write(2, error_msg, strlen(error_msg)) < 0) {
			perror("Error in writing to standard error file");
			return (WRITE_ERROR);
		}
		return (ARGUMENT_ERROR);
	}
	if (argc == 1) {
		const char* home = getenv("HOME");
		if (home == NULL) {
			perror("Error in getenv: HOME not set");
			return (GETENV_ERROR);
		}

		if (chdir(home) < 0) {
			perror("Error in chdir (changing cwd)");
			return (CHDIR_ERROR);
		}
		return 0;
	}

	if (chdir(argv[1]) < 0) {
		printf("cd: %s: No such file or directory\n", argv[1]);
		return (CHDIR_ERROR);
	}

	return 0;
}
