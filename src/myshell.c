#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <limits.h>
#include <sys/wait.h>
#include <math.h>

#define default_name "guest"
#define PROMPT "@myshell# "
#define filename "/mainv2"
#define MAX_BUFF 256
#define MAX_ARG  20
#define H_READ   -1

void sigint_handler(int);
void setup(int, char**);
void loop();
void update_path(char*);
void exit_command(void);
void print_error(const char*);
void clear_args(char** args);

char*  read_command(char*);

int parse_command(char*, char**);
int prepare_history(const int, const char*, const char*, char**);
int run_command(char**);
int cd(char**);

char* username = NULL;
char* prompt = NULL;
char* bin_path = NULL;
char* err_file_path = NULL;


/**
 * \brief           Main function: after setting up the new PATH, it calls the setup() and loop() functions.
 * \param[in]       argc: Number of arguments passed by the user through the main shell (2 maximum)
 * \param[in]       argv: Array of arguments. argv[0] contains the path to this file as passed by the user;
 *                  argv[1] (optional) contains the username set by the user ("guest" as default).
 * \return          Returns 0 in case of success, a non-zero integer oterwise.      
*/
int
main(int argc, char* argv[]) {
    /* Set stderr to default stderr */
    stderr = stderr;

    update_path(argv[0]);

    /* Setup function */
    setup(argc, argv);

    /* Loop function */
    loop();
}

/**
 * \brief           This function updates the local variable PATH of the process to the absolute path
 *                  of the bin folder, where all the executable files can be found.
 * \param[in]       argv0: Corrsponds to the argv[0] string sent to the main; therefore, it contains the
 *                  program path sent by the user as argument (It can be relative or absolute compared
 *                  to the actual location of the main shell when myshell is launched).
*/
void
update_path(char* argv0) {
    char* last_occurence = NULL;
    if ((last_occurence = strrchr(argv0, '/')) == NULL) {
        print_error("updating the PATH variable");
    }

    int rel_path_length = (int)(last_occurence - argv0);
    char* relative_path = (char*)malloc(rel_path_length);
    strncat(relative_path, argv0, rel_path_length);

    /* Gets absolute bin path, given its relative path */
    bin_path = realpath(relative_path, bin_path);
    setenv("PATH", bin_path, 1);

    free(relative_path);
    return;
}

/**
 * \brief           This function checks if the right number of arguments was passed by the user, tries to
 *                  register the exit and SIGINT handlers and sets up the prompt string with the right username.
 *                  Moreover, it handles the eventual redirection of the error output stream.
 * \param[in]       argc: number of arguments passed to main()
 * \param[in]       argv: array of arguments passed to main()
*/
void
setup(int argc, char* argv[]) {
    FILE* stderr = stderr;

    /* Check if there's an exceeding number of arguments */
    if (argc > 4) {
        fprintf(stderr, "Unexpected arguments!\n");
        exit(EXIT_FAILURE);
    }
    
    /* Tries to add an exit handler */
    if (atexit(exit_command) != 0) {
        fprintf(stderr, "Cannot register exit handler.\n");
        exit(EXIT_FAILURE);
    }

    /* Tries to add a SIGINT interrupt handler */
    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        fprintf(stderr, "Error! Couldn't register SIGINT handler: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* Chooses username */
    if (argc == 1 && (stderr == stderr)) {
        username = (char*)malloc(strlen(default_name)+1);
        strcpy(username, default_name);
    } else {
        username = (char*)malloc(strlen(argv[1])+1);
        strcpy(username, argv[1]);
    }

    /* Initializes prompt string */
    prompt = (char*)malloc(strlen(username)+1);
    strcpy(prompt, username);
    prompt = strcat(prompt, PROMPT);

    return;
}

/**
 * \brief           This function repeats, until termination of the process, the tasks of the shell: reads the
 *                  command, executes it and shows again the prompr, ready for the next command, eventually
 *                  displaying an error message.
*/
void
loop() {
    char*  last_command = NULL;     /* Name of the last successfully executed command */
    char*  stdin_buff = NULL;       /* Stdin buffer */
    char*  args[MAX_ARG] = {NULL};  /* Array of strings, containing the arguments to pass */
    int    exit_status = 0;
    int    command_counter = 0;     /* Successfully executed command counter */
    

    /* Starts loop */
    while (1) {

        /* Print the prompt */
        printf("%s", prompt);

        /* Read the command */
        stdin_buff = read_command(stdin_buff);

        if (stdin_buff == NULL) {   /* Nothing was read: error */
            print_error("reading");
            exit(EXIT_FAILURE);
        } else if (strcmp(stdin_buff, "\n") == 0) { /* Read \n: prompt again */
            continue;
        }

        /* Parse the command */
        if (parse_command(stdin_buff, args) == EOF) {
            print_error("parsing the command");
            continue;
        }

        /* Selects command */
        if (strcmp (args[0], "cd") == 0) {
            exit_status = cd(args);
        }
        else if (strcmp(args[0], "ls") == 0 || strcmp(args[0], "pwd") == 0) {
            exit_status = run_command(args);
        }
        else if (strcmp(args[0], "history") == 0) {
            if (prepare_history(H_READ, bin_path, last_command, args) == EOF) {
                print_error("preparing history");
                continue;
            }

            exit_status = run_command(args);

        } else if (strcmp (args[0], "exit") == 0) {
            /* Add exit command to .history file */
            if (prepare_history(command_counter, bin_path, "exit", args) == EOF) {
                print_error("preparing history");
                /* No continue instruction is used, let the process exit anyways */
            }

            /* Run "history" and check its exit status */
            if (run_command(args) == EXIT_FAILURE) {
                print_error("executing history");
                /* No continue instruction is used, let the process exit anyways */
            }

            /* Frees all the dynamically allocated memory */
            /* String username is not freed, the exit handler function needs it */
            clear_args(args);
            free(last_command);
            free(stdin_buff);
            free(prompt);
            free(bin_path);

            /* Exit through exit handler */
            exit(EXIT_SUCCESS);
        }
        else {
            exit_status = EXIT_FAILURE;
            printf("bash: %s: command not found\n", args[0]);
            continue;
        }

        /* Check the exit status of the last command. Add it to history if it succeeded. */
        if (exit_status == EXIT_SUCCESS) {
            last_command = (char*)realloc(last_command, strlen(args[0]) + 1);
            strcpy(last_command, args[0]);

            if (prepare_history(command_counter, bin_path, last_command, args) == EOF) {
                print_error("preparing history");
                continue;
            }

            /* Run "history" and check its exit status */
            if (run_command(args) == EXIT_FAILURE) {
                print_error("executing history");
            } else {
                command_counter++;
            }

        }

        clear_args(args);
    }
}

/**
 *  \brief          This function reads one line from stdin and stores it in a string.
 *  \note           The pointer to the string sent by the user will be modified.
 *  \param[in]      stdin_buff: pointer to the location in memory, where the read string will be written.
 *  \return         Returns the pointer to the read string, NULL on error.
*/
char*
read_command(char* stdin_buff) {
    char tempbuf[MAX_BUFF]; /* Temporary buffer */
    size_t inputlen = 0;
    size_t templen = 0;

    /* Store at most MAX_BUFF chars in tempbuf and append them in stdin_buff. */
    /* If stdin stream has still content, repeat until new line.              */
    do {
        if (fgets(tempbuf, MAX_BUFF, stdin) == NULL) {
            print_error("reading from stdin");
            return NULL;
        }

        templen = strlen(tempbuf);
        stdin_buff = realloc(stdin_buff, inputlen + templen + 1);

        if (stdin_buff == NULL) {
            print_error("reading from stdin");
            return NULL;
        }

        strcpy(stdin_buff + inputlen, tempbuf);
        inputlen += templen;
    } while (templen == MAX_BUFF-1 && tempbuf[MAX_BUFF-2] != '\n');

    return stdin_buff;
}

/**
 *  \brief          This function parses the string read on stdin and collects the name of the command and 
 *                  the arguments that the user wants to execute. In addition, it prepares the arguments'
 *                  array, which will be sent to the child process.
 *  \param[in]      string: pointer to the command string read on stdin
 *  \param[in]      args: pointer to the array of strings, which will be sent to the child process.
 *  \return         Returns 0 on success, EOF on error.
*/
int
parse_command(char* string, char** args_vect) {
    int is_word = 0;
    int arg_num = 0;
    char buff[MAX_BUFF] = "";

    for (int i = 0; i < strlen(string); i++) {
        char c = string[i];

        /* Check if it's an alphanumerical character (i.e. plausible text) */
        
        if (c != '\0' && c != ' ' && c != '\n') {
            is_word = 1;
            strncat(buff, &c, 1);   /*Concatenates chars until assembling the first isolated word in the user input */
        }

        if (c == ' ' || c == '\n') { 
            if (is_word == 0) {
                continue;
            } else {
                /* Adds the word found to the arguments' array */
                args_vect[arg_num] = (char*)malloc(strlen(buff) + 1);

                if (args_vect[arg_num] == NULL) {
                    return EOF;
                }

                strcpy(args_vect[arg_num], buff);
                arg_num++;
                
                /* Clears the buffer made for containing each word */
                buff[0] = '\0';
                is_word = 0;
            }
        } 
    }

    /* Adds the arguments' array terminator, i.e. NULL pointer */
    args_vect[arg_num++] = NULL;

    return 0; 
}

/**
 * \brief           This function prepares the array of arguments for the history command.
 * \note            args[1] := command_counter
 *                  args[2] := storage_path
 *                  args[3] := last_command
 *                  If command_counter is equal to -1, then the file ".history" will be opened on read,
 *                  therefore there is no need of the string "last_command" (args[3] is set to NULL).
 * \note            It clears the arguments' array.
 * \param[in]       command_counter: counter of the successfully executed commands.
 * \param[in]       storage_path: absolute path to the storage folder, where the file ".history" is located.
 * \param[in]       last_command: a string containing the name of the last successfully executed command.
 * \param[in]       args: pointer to the array of arguments.
 * \return          Returns 0 on success, EOF on error.
*/
int
prepare_history(const int command_counter, const char* storage_path, const char* last_command, char* args[]) {
    /* Clear the arguments' array */
    clear_args(args);
    
    /* Allocates enough characters, in order to contain the integer command_counter converted into a string */
    int size = 1;
    int _command_counter = command_counter;

    if (command_counter >= 0) {     /* Open ".history" file on write in append */        
        /* Counts how many digits there are inside integer command_counter*/
        while ((_command_counter / 10) != 0) {
            size++;
            _command_counter = _command_counter / 10;
        } 

        /* Prepare the first argument */
        args[1] = (char*)malloc((size_t)(size + 1));
        if (args[1] == NULL) {
            return EOF;
        }

        /* Prepare the third argument */
        args[3] = (char*)malloc(strlen(last_command) + 1);
        if (args[3] == NULL) {
            return EOF;
        }
        strcpy(args[3], last_command);

        /* Sets the terminator of the arguments' string */
        args[4] = NULL;

    } else if (command_counter == H_READ) {     /* Open ".history" file on read */
        /* Prepare the first argument */
        size = (int)strlen("-1") + 1;
        args[1] = (char*)malloc(strlen("-1") + 1);
        if (args[1] == NULL) {
            return EOF;
        }

        /* Sets the terminator of the arguments' string */
        args[3] = NULL;
    }

    /* Sets the argument containing the name of the command history */
    args[0] = (char*)malloc(strlen("history") + 1);
    if (args[0] == NULL) {
            return EOF;
        }
    strcpy (args[0], "history");

    /* Converts the integer command_counter to a string, by printing it on the very same string */
    /* snprintf() is used in order to avoid overflow errors */
    snprintf(args[1], (size_t)(size + 1), "%d", command_counter);

    /* Prepare the second argument for both cases */
    args[2] = (char*)malloc(strlen(storage_path) + 1);
    if (args[2] == NULL) {
            return EOF;
        }
    strcpy(args[2], storage_path);

    return 0;
}

/**
 * \brief           This function forks the current process and executes the command requested by the user 
 *                  in the child process. Sets the parent process on wait for the child process.
 * \param[in]       args: arguments' array for the called command.
 * \return          Returns 0 if the child process executes and terminates correctly, a non-zero integer otherwise
*/
int run_command(char* args[]) {
        /* start forking */
        int status;
        pid_t pid;
        
        if ((pid = fork()) < 0) {
            print_error("forking");
            return EXIT_FAILURE;
        }
        else if (pid == 0) {
            /* child process */
            if (execvp(args[0], args) == EOF){
                fprintf(stderr, "process: %s\n", args[0]);
                print_error("executing child process");
                return EXIT_FAILURE;
            }
        }
        else {
            /* parent process */
            if (wait(&status) < 0) {
                print_error("waiting");
                return EXIT_FAILURE;
            }
		    if (WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS)
			    return EXIT_SUCCESS;
		    else
			    return EXIT_FAILURE;            
        }
        //return EXIT_SUCCESS;
}

/**
 *  \brief          This function changes the current directory to the one passed in args.
 *  \param[in]      args[1]: If present, contains the new path. 
 *  \return         Returns 0 on success, 1 on error.
*/
int cd(char* args[]) {
    int arg_num = 0;
    while (args[arg_num] != NULL) {
        arg_num++;
    }

	char* newdir;

	if (arg_num > 2) {
		fprintf(stderr, "Unexpected arguments\n");
		return EXIT_FAILURE;	
	}
	
	if (arg_num == 2) {
		newdir = (char*)malloc(strlen(args[1]) + 1);
		strcpy(newdir, args[1]);
		if (chdir(newdir) < 0) {
			print_error("opening");
			return EXIT_FAILURE;
		}
	}

    return EXIT_SUCCESS;
}

/**
 * \brief           This function clears the dynamic array of strings, made to hold the arguments.
 * \param[in]       args: pointer to the array of strings that has to be cleared.
*/
void
clear_args(char** args) {
    /* Frees each string in the array */    
    for (int i = 0; i < MAX_ARG; i++) {
        free(args[i]);
        args[i] = NULL; 
    }
    return;
}

/**
 * \brief           This function prints an error message on stderr, with format: "Error while *your string*
 *                  [code *errno*, *strerror(errno)*]"
 * \param[in]       str: Your costant string, which will be pasted inside the error message.
*/
void
print_error(const char str[]) {
    fprintf(stderr, "Error while %s [code %d, %s] \n", str, errno, strerror(errno));
    return;
}

/**
 *  \brief          This functions handles the CTRL+C interrupt (SIGINT).
 *  \param[in]      signo: ID numbe for SIGINT.
*/
void
sigint_handler(int signo) {
    /* Notifies the user about the right procedure in order to close Myshell */
    printf("\nTo close myshell, type \"exit\"\n");
    printf("%s", prompt);
    fflush(stdout);
    return;
}

/**
 *  \brief          This functions handles the exit command (greets the user).
*/
void
exit_command(void) {
    printf("Goodbye, %s!\n", username);

    /* Deallocates the space reserved in the heap for the "username" string */
    free(username);
}