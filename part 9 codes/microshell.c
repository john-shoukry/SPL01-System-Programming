#include <stdlib.h>	// to use malloc, realloc, exit, getenv, putenv
#include <stdio.h>	// to use getchar, EOF, perror, fflush, fprintf, stderr
#include <string.h>	// to use strlen, strtok, strcmp, strchr, strcpy, strncpy
#include <unistd.h>     // to use write, getcwd, fork, execvp, chdir, isatty, dup, dup2, close
#include <sys/types.h>	// to use pid_t
#include <sys/wait.h>	// to use wait
#include <stddef.h>	// to use size_t
#include <fcntl.h>	// to use open
#include <stdbool.h>	// to use bool


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
#define MALLOC_ERROR	-11
#define REALLOC_ERROR	-12
#define PUTENV_ERROR	-13
#define DUP_ERROR	-14
#define CLOSE_ERROR	-15


#define MAX_ARGS 150


char* read_line();
int echo(int argc, char* argv[]);
int pwd(int argc);
int cd(int argc, char* argv[]);
int matchesEqualPattern(char* str);
int my_export(int argc, char* argv[], char** localVars, int lengthLocalVars);
char* getValueByKey(char** localVars, int lengthLocalVars, char* requiredKey);
char* replaceByPointers(char* pre_str, char* replace_start, char* post_str);


int microshell_main(int argc, char *argv[]) {

	int last_status = 0; // Track the last command status

	int sizeLocalVars = 64; // initial array size
	char** localVars = (char**)malloc(sizeLocalVars * sizeof(char*));
	if (localVars == NULL) {
		perror("Unable to allocate memory");
		exit(MALLOC_ERROR);
	}
	int lengthLocalVars = 0;

	// save stdin, stdout, stderr to restore after execution
	int saved_stdin = dup(0);
	int saved_stdout = dup(1);
	int saved_stderr = dup(2);

	while (1) {
		int is_interactive = isatty(STDIN_FILENO);

		printf("Micro shell prompt > ");
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

		// check for $ and replace with variable value
		for (int i = 0; i < newArgc; i++) {
			char* replace_start = strchr(newArgv[i], '$');
			if (replace_start != NULL) {
				// $ found
				char* requiredKey = replace_start + 1; // skip '$'
				char* value = getValueByKey(localVars, lengthLocalVars, requiredKey);
				if (value != NULL) {
					// key found, so replace by value
					newArgv[i] = replaceByPointers(newArgv[i], replace_start, value);
				}
				else {
					// key not found, so replace by nothing (empty string) as shell behaviour
					newArgv[i] = replaceByPointers(newArgv[i], replace_start, "");
				}
				free(value);
			}
		}

		// check for IO and stderr Redirection
		bool successFlag = true;
		for (int i = 0; i < newArgc; i++) {
			if (strcmp(newArgv[i], "<") == 0) {
				int j = i + 1;
				if (j >= newArgc) {
					fprintf(stderr, "syntax error near unexpected token `newline'\n");
					successFlag = false;
					last_status = -1;
					break;
				}
				// input redirection
				int fd = open(newArgv[j], O_RDONLY);
				if (fd < 0) {
					fprintf(stderr, "cannot access %s: No such file or directory\n", newArgv[j]);
					successFlag = false;
					last_status = -1;
					break;
				}
				// redirect input
				if (dup2(fd, 0) < 0) {
					perror("Error in dup2 --> failed to duplicate input fd to point to stdin");
					free(input_line);
					exit(DUP_ERROR);
				}

				if (close(fd) < 0) {
					perror("Error in closing source file descriptor");
					free(input_line);
					exit(CLOSE_ERROR);
				}
			}
			else if (strcmp(newArgv[i], ">") == 0) {
				int j = i + 1;
				if (j >= newArgc) {
					fprintf(stderr, "syntax error near unexpected token `newline'\n");
					successFlag = false;
					last_status = -1;
					break;
				}
				// output redirection
				int fd = open(newArgv[j], O_WRONLY | O_CREAT | O_TRUNC, 0644);
				if (fd < 0) {
					fprintf(stderr, "%s: Permission denied\n", newArgv[j]);
					successFlag = false;
					last_status = -1;
					break;
				}
				// redirect output
				if (dup2(fd, 1) < 0) {
					perror("Error in dup2 --> failed to duplicate input fd to point to stdin");
					free(input_line);
					exit(DUP_ERROR);
				}

				if (close(fd) < 0) {
					perror("Error in closing source file descriptor");
					free(input_line);
					exit(CLOSE_ERROR);
				}
			}
			else if (strcmp(newArgv[i], "2>") == 0) {
				int j = i + 1;
				if (j >= newArgc) {
					fprintf(stderr, "syntax error near unexpected token `newline'\n");
					successFlag = false;
					last_status = -1;
					break;
				}
				// stderr redirection
				int fd = open(newArgv[j], O_WRONLY | O_CREAT | O_TRUNC, 0644);
				if (fd < 0) {
					fprintf(stderr, "Permission denied\n");
					successFlag = false;
					last_status = -1;
					break;
				}
				// redirect input
				if (dup2(fd, 2) < 0) {
					perror("Error in dup2 --> failed to duplicate input fd to point to stdin");
					free(input_line);
					exit(DUP_ERROR);
				}

				if (close(fd) < 0) {
					perror("Error in closing source file descriptor");
					free(input_line);
					exit(CLOSE_ERROR);
				}
			}
		}

		if (!successFlag) {
			free(input_line);
			dup2(saved_stdin, 0);
			dup2(saved_stdout, 1);
			dup2(saved_stderr, 2);
			continue;
		}

		// delete IO, and stderr redirections from newArgv array
		for (int i = 0; i < newArgc - 1; i++) {
			if ((strcmp(newArgv[i], "<") == 0) || (strcmp(newArgv[i], ">") == 0) || (strcmp(newArgv[i], "2>") == 0)) {
				// Shift all args after i+2 to the left by 2 positions
				for (int j = i; j < newArgc - 2; ++j) {
					newArgv[j] = newArgv[j + 2];
				}

				newArgc -= 2;
				newArgv[newArgc] = NULL;
				newArgv[newArgc + 1] = NULL;

				i--; // Re-check this index in case another redirection follows
			}
		}


		if (strcmp(newArgv[0], "echo") == 0) {
			int value_returned = echo(newArgc, newArgv);
			if (value_returned < 0) {
				free(input_line);
				exit(value_returned);
			}
			last_status = value_returned;
			free(input_line);
			// Restore the original descriptors
			dup2(saved_stdin, 0);
			dup2(saved_stdout, 1);
			dup2(saved_stderr, 2);
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
			// Restore the original descriptors
			dup2(saved_stdin, 0);
			dup2(saved_stdout, 1);
			dup2(saved_stderr, 2);
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
			// Restore the original descriptors
			dup2(saved_stdin, 0);
			dup2(saved_stdout, 1);
			dup2(saved_stderr, 2);
			continue;
		}
		else if (strcmp(newArgv[0], "exit") == 0) {
			printf("Good Bye\n");
			free(input_line);
			break;
		}
		else if ((matchesEqualPattern(newArgv[0])) && (newArgc == 1)) {
			// Key=Value
			// save the variable
			// deep copy
			char* copy = (char*)malloc(strlen(newArgv[0]) + 1);
			if (copy == NULL) {
				perror("Unable to allocate memory");
				exit(MALLOC_ERROR);
			}
			strcpy(copy, newArgv[0]);

			localVars[lengthLocalVars++] = copy;

			if (lengthLocalVars >= sizeLocalVars) {
				sizeLocalVars *= 2;
				char** new_localVars = (char**)realloc(localVars, sizeLocalVars * sizeof(char*));
				if (new_localVars == NULL) {
					perror("Unable to reallocate memory");
					free(localVars);
					exit(REALLOC_ERROR);
				}
				localVars = new_localVars;
			}

			free(input_line);
			// Restore the original descriptors
			dup2(saved_stdin, 0);
			dup2(saved_stdout, 1);
			dup2(saved_stderr, 2);
			continue;
		}
		else if (strcmp(newArgv[0], "export") == 0) {
			int value_returned = my_export(newArgc, newArgv, localVars, lengthLocalVars);
			if (value_returned < 0) {
				free(input_line);
				exit(value_returned);
			}
			last_status = value_returned;

			free(input_line);
			// Restore the original descriptors
			dup2(saved_stdin, 0);
			dup2(saved_stdout, 1);
			dup2(saved_stderr, 2);
			continue;
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

		// Restore the original descriptors
		dup2(saved_stdin, 0);
		dup2(saved_stdout, 1);
		dup2(saved_stderr, 2);
	}

	close(saved_stdin);
	close(saved_stdout);
	close(saved_stderr);

	for (int i = 0; i < lengthLocalVars; i++) {
		free(localVars[i]);
	}
	free(localVars);
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


int matchesEqualPattern(char* str) {
	char* occurrence_of_equal = strchr(str, '=');

	if (occurrence_of_equal == NULL) {
		return 0; // no '=' found
	}
	if (occurrence_of_equal == str) {
		return 0; // '=' is the first char (2 pointers are pointing to the beginning of str)
	}
	if (*(occurrence_of_equal + 1) == '\0') {
		return 0; // '=' is the last char followed by the null character
	}

	return 1; // Valid: has characters before and after '='
}


int my_export(int argc, char* argv[], char** localVars, int lengthLocalVars) {
	if (argc == 1) {
		char* error_msg = "export: No variables passed\n";
		if (write(2, error_msg, strlen(error_msg)) < 0) {
			perror("Error in writing to standard error file");
			return (WRITE_ERROR);
		}
		return (ARGUMENT_ERROR);
	}

	for (int i = 1; i < argc; i++) {
		// search for argv[i] in localVars
		for (int j = 0; j < lengthLocalVars; j++) {
			///////////////////// get key (variable before equal)
			int size = 128;
			char* key = (char*)malloc(size);
			if (key == NULL) {
				perror("Unable to allocate memory");
				return (MALLOC_ERROR);
			}
			int len = 0;

			for (int k = 0; localVars[j][k] != '='; k++) {
				key[len++] = localVars[j][k];

				if (len >= size) {
					// resize
					size *= 2;

					char* new_key = (char*)realloc(key, size);
					if (new_key == NULL) {
						perror("Unable to reallocate buffer");
						free(key);
						return (REALLOC_ERROR);
					}

					key = new_key;
				}
			}
			////////////////////////////////////////////////////

			if (strcmp(argv[i], key) == 0) {
				// add to path variables
				if (putenv(localVars[j]) < 0) {
					perror("Error in putenv");
					return (PUTENV_ERROR);
				}
			}
		}
	}
	return 0;
}


char* getValueByKey(char** localVars, int lengthLocalVars, char* requiredKey) {
	for (int j = 0; j < lengthLocalVars; j++) {
		///////////////////// get key (variable before equal)
		int size = 128;
		char* key = (char*)malloc(size);
		if (key == NULL) {
			perror("Unable to allocate memory");
			return (NULL);
		}
		int len = 0;

		for (int k = 0; localVars[j][k] != '='; k++) {
			key[len++] = localVars[j][k];

			if (len >= size) {
				// resize
				size *= 2;

				char* new_key = (char*)realloc(key, size);
				if (new_key == NULL) {
					perror("Unable to reallocate buffer");
					free(key);
					return (NULL);
				}

				key = new_key;
			}
		}

		key[len] = '\0';
		// compare key with required
		if (strcmp(requiredKey, key) == 0) {
			char* value = strchr(localVars[j], '=');
			value++; // as not to take '=' itself

			char* result = (char*)malloc(strlen(value) + 1); // +1 for null terminator
			if (result == NULL) {
				perror("Unable to allocate memory");
				return (NULL);
			}

			strcpy(result, value);
			return result; // return value of the variable if found
		}
		free(key);
	}
	return NULL; // not found
}


char* replaceByPointers(char* pre_str, char* replace_start, char* post_str) {
	size_t pre_length = replace_start - pre_str;
	size_t post_length = strlen(post_str);

	char* result = (char*)malloc(pre_length + post_length + 1); // +1 for null terminator
	if (result == NULL) {
		perror("Unable to allocate memory");
		return (NULL);
	}

	// copy the prefix of pre_str
	strncpy(result, pre_str, pre_length);

	// copy the suffix from post_str
	strcpy(result + pre_length, post_str);

	return result;
}
