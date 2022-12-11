#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#define FILE_NAME ".history"

FILE* history = NULL;
const int MAX_LINE = 1024;

/**
 * \brief           This program manages the history of the command successfully executed by the user.
 *                  The history of myshell is kept inside a hidden filled ".history", stored in the original folder
 *                  where the process myshell is launched.
 *                  Depending on the value of "command_counter", this program can write in append on the file ".history" 
 *                  (command_counter >= 0) or it can read from it and print its content on stdout (command_counter = -1).
 *                  It takes as inputs the total number of the executed commands, the name of the last command and the
 *                  path for the storage of the file ".history".     
 * \param[in]       argv[1]: equivalent to the command counter. If set to -1, the file ".history" will be open
 *                  on read.
 * \param[in]       argv[2]: contains the absolute storage path, where the file ".history" will be saved
 * \param[in]       argv[3]: if argv[1] >= 0, it holds the name of the last successfully executed command.
 * \return          Returns 0 on success, a non-zero integer on error.            
*/
int main(int argc, char* argv[]) {

    int command_counter = atoi(argv[1]);

    if (argc > 4) {
        fprintf(stderr, "Unexpected arguments\n");
        exit(EXIT_FAILURE);
    }

    if ((argc < 4 && (command_counter != -1)) || (argc < 3 && (command_counter == -1))) {
        fprintf(stderr, "Too few arguments\n");
        exit(EXIT_FAILURE);
    }

    if (command_counter < -1) {
        fprintf(stderr, "Wrong arguments\n");
        exit(EXIT_FAILURE);
    }
    
    //Get the storage path for ".history"
    char* path = (char*)malloc(strlen(argv[2]) + strlen(FILE_NAME) + 2);
    strcpy(path, argv[2]);
    strcat(path, "/");
    strcat(path, FILE_NAME);

    if (command_counter >= 0) {

        //Open the file stream in write mode
        if (command_counter == 0) {     //New session of myshell: truncate the file
            if ((history = fopen(path, "w")) == NULL) {
            fprintf(stderr, "Error while opening file \".history\": %s\n", strerror(errno));
            exit(EXIT_FAILURE);
            }
        } else {                        //Not a new session: append to the file
            if ((history = fopen(path, "a")) == NULL) {
            fprintf(stderr, "Error while opening file \".history\": %s\n", strerror(errno));
            exit(EXIT_FAILURE);
            }
        }

        //Print the line to ".history"
        if (fprintf(history, "%s %s\n", argv[1], argv[3]) <= 0) {
            fprintf(stderr, "Error while writing\n");
            exit(EXIT_FAILURE);
        }

    } else if (command_counter == -1) {

        //Open the file stream in read mode
        if ((history = fopen(path, "r")) == NULL) {
            fprintf(stderr, "Error while opening file \".history\": %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        //Read each line of ".history"
        char buff[MAX_LINE];
        while (fgets(buff, MAX_LINE, history) != NULL) {
            
            //Print it on stdout
            if (fputs(buff, stdout) == EOF) {
                fprintf(stderr, "Error while printing to stdout\n");
                exit(EXIT_FAILURE);
            }
        } 
    }

    //Close the stream
    if (fclose(history) == EOF) {
        fprintf(stderr, "Error while closing the stream\n");
        exit(EXIT_FAILURE);
    }

    free(path);
    return EXIT_SUCCESS;    
}