#include <unistd.h>    // for getcwd
#include <stdlib.h>    // for exit, free
#include <string.h>    // for strlen
#include <stdio.h>     // for perror

#define WRITE_ERROR    -1
#define GETCWD_ERROR   -2
int pwd_main() {



    char* cwd = getcwd(NULL, 0);  // Let getcwd allocate the buffer
    if (cwd == NULL) {
        perror("getcwd");
        exit(GETCWD_ERROR);
    }

    // Write the current directory followed by a newline
    size_t len = strlen(cwd);
    if (write(STDOUT_FILENO, cwd, len) != (ssize_t)len) {
        perror("write");
        free(cwd);
        exit(WRITE_ERROR);
    }
    if (write(STDOUT_FILENO, "\n", 1) != 1) {
        perror("write");
        free(cwd);
        exit(WRITE_ERROR);
    }

    free(cwd);
    return 0;
}
