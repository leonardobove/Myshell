#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

//Get current working path and print it to stdout: returns 0 if it succeded, EOF = -1 otherwise

int main(int argc, char *argv[]) {

    char* current_path = NULL;

    if ((current_path = getcwd(NULL, 0)) == NULL) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    printf("%s\n", current_path);
    free(current_path);

    return EXIT_SUCCESS;
}
