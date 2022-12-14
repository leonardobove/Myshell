#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

//Get current working path and print it to stdout: returns 0 if it succeded, EOF = -1 otherwise

/**
 *  \brief          This function gets the current path and prints it to stdout.
 *  \return         Returns 0 on success, 1 on error.
 */
int
main(int argc, char *argv[]) {
    /* Checks number of arguments (zero arguments expected) */
    if (argc > 3) {
        fprintf(stderr, "Unexpected arguments!\n");
        exit(EXIT_FAILURE);
    }

    /* Redirects the error output stream to the specified file in the arguments (if present) */
    FILE* error_stream = stderr;

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "2>") == 0) {
            if (argv[i + 1] != NULL) {
                if ((error_stream = fopen(argv[i + 1], "a")) == NULL) {
                    fprintf(stderr,"Error while opening the error output stream\n");
                    exit(EXIT_FAILURE);
                }
                /* Set the error stream to unbeffered */
                if (setvbuf(error_stream, NULL, _IONBF, 0) != 0) {
                    fprintf(stderr, "Error while setting the error stream as unbuffered: %s", strerror(errno));
                    exit(EXIT_FAILURE);
                }
            } else {
                fprintf(stderr, "Wrong arguments! Did you mean \"2> error_output_file\"?\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    char* current_path = NULL;

    /* Gets current path and saves it in "current_path" string */
    if ((current_path = getcwd(NULL, 0)) == NULL) {
        fprintf(error_stream, "Error: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* Prints "current_path" string */
    printf("%s\n", current_path);
    free(current_path);

    /* Closes the error stream */
    fclose(error_stream);

    return EXIT_SUCCESS;
}
