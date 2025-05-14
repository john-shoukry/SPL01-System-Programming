#include <unistd.h>	// to use write, read, close
#include <string.h>	// to use strlen
#include <stdio.h>	// to use perror
#include <stdlib.h>	// to use exit
#include <fcntl.h>	// to use open

#define ARGUMENT_ERROR -1
#define WRITE_ERROR    -2
#define OPEN_ERROR     -3
#define READ_ERROR     -4
#define CLOSE_ERROR    -5

#define COUNT 100


int cp_main(int argc, char *argv[]) {



	if (argc == 1) {
		// missing source file operand
		char* error_msg = ": missing file operand\n";
		// write to std error file --> "%s: missing file operand\n", argv[0]
		if ((write(2, argv[0], strlen(argv[0])) < 0) || (write(2, error_msg, strlen(error_msg)) < 0)) {
			perror("Error in writing to standard error file");
			exit(WRITE_ERROR);
		}
		exit(ARGUMENT_ERROR);
	}
	else if (argc == 2) {
		// missing destination file operand
		char* error_msg = ": missing destination file operand after '";
		// write to std error file --> "%s: missing destination file operand after '%s'\n", argv[0], argv[1]
		if ((write(2, argv[0], strlen(argv[0])) < 0) || (write(2, error_msg, strlen(error_msg)) < 0)
			|| (write(2, argv[1], strlen(argv[1])) < 0) || (write(2, "'\n", 2) < 0)) {
			perror("Error in writing to standard error file");
			exit(WRITE_ERROR);
		}
		exit(ARGUMENT_ERROR);
	}
	// copying multiple files into a directory isn't supported

	int fd_src = open(argv[1], O_RDONLY);
	if (fd_src < 0) {
		char* error_msg_1 = ": cannot stat '";
		char* error_msg_2 = "': No such file or directory\n";
		// write to std error file --> "%s: cannot stat '%s': No such file or directory\n", argv[0], argv[1]
		if ((write(2, argv[0], strlen(argv[0])) < 0) || (write(2, error_msg_1, strlen(error_msg_1)) < 0)
			|| (write(2, argv[1], strlen(argv[1])) < 0) || (write(2, error_msg_2, strlen(error_msg_2)) < 0)) {
			perror("Error in writing to standard error file");
			exit(WRITE_ERROR);
		}
		// to print errno
		// perror("Error in opening source file");
		exit(OPEN_ERROR);
	}

	int fd_dest = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd_dest < 0) {
		perror("Error in opening or creating destination file");
		exit(OPEN_ERROR);
	}

	char buf[COUNT];
	int num_bytes_read;
	do {
		num_bytes_read = read(fd_src, buf, COUNT);
		if (num_bytes_read < 0) {
			perror("Error in reading from source file");
			exit(READ_ERROR);
		}
		if (num_bytes_read > 0) {
			if (write(fd_dest, buf, num_bytes_read) < 0) {
				perror("Error in writing to destination file");
				exit(WRITE_ERROR);
			}
		}
	}
	while (num_bytes_read > 0);


	if (close(fd_src) < 0) {
		perror("Error in closing source file descriptor");
		exit(CLOSE_ERROR);
	}
	if (close(fd_dest) < 0) {
		perror("Error in closing destination file descriptor");
		exit(CLOSE_ERROR);
	}
	return 0;

}
