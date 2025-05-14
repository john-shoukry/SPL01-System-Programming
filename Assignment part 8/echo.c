
#include <unistd.h>   // for write
#include <string.h>   // for strlen
#include <stdio.h>    // for perror

#define WRITE_ERROR -1

int echo_main(int argc, char *argv[]) {
    for (int i = 1; i < argc; ++i) {
        // Write the argument
        if (write(STDOUT_FILENO, argv[i], strlen(argv[i])) < 0) {
            perror("echo: write failed");
            return WRITE_ERROR;
        }

        // Write a space after each word except the last one
        if (i < argc - 1) {
            if (write(STDOUT_FILENO, " ", 1) < 0) {
                perror("echo: write failed");
                return WRITE_ERROR;
            }
        }
    }

    // Write a newline at the end
    if (write(STDOUT_FILENO, "\n", 1) < 0) {
        perror("echo: write failed");
        return WRITE_ERROR;
    }

    return 0;
}
