#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <math.h>

#define default_name "guest"
#define PROMPT "@myshell# "
#define MAX_BUFF 256
#define MAX_ARG_NUM 10

char* username = NULL;
char* prompt = NULL;
char stdin_buff[MAX_BUFF];
char* absolute_path = NULL;

//TODO: Check which parameters can be made constant
void sigint_handler(int);
void prepare_command(int*, char**);
void setup(int*, char**);
void loop();
void prepare_history(const int, const char*, const char*, char**);
void exit_command(void);
int run_command(char*, int*, char**);
int cd(int*, char**);
void print_error(const char*);
void get_cmd_path();
void clear_args(char**);

//TODO: when no command is read, just prompt the defualt sentence again

int main(int argc, char* argv[]) {

    //find the directory where the executable files are located            
    //char* absolute_path;          //absolute path to the executable
    char relative_path[MAX_BUFF]; //relative path to the executable
    char pivot_path[MAX_BUFF];
    strcpy(pivot_path, argv[0]);
    /*char c;
    for (int i = (strlen(argv[0]) - 1); i >= 0; i--) {
        c = pivot_path[i];
        if (c == '/') {
            for (int j = 0; j < i; j++) {
                strncat(relative_path, &pivot_path[j], 1);
            }
            break;
        }
    }*/
    chdir(relative_path);
    absolute_path = getcwd(NULL, 0);
    setenv("PATH", absolute_path, 1);

    //This function has to be done before exit
    if (atexit(exit_command)!= 0) {
        fprintf(stderr, "\ncan't register exit_command\n");
    }

    //Check if there is a signal
    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        fprintf(stderr, "Error! Couldn't register SIGINT handler: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    setup(&argc, argv);
    loop();
    
    return EXIT_SUCCESS;
}


/** 
 *  \brief          Questa funzione non fa un cazzo
 *  \note           Nota inutile
 *  \param[in]      argc: numero di argomenti
 *  \param[in]      argv: altra roba
 *  \return         Non ritorna un cazzo
 * 
*/
void setup(int* argc, char* argv[]) {
    //setup code
    //Check if there's an exceeding number of arguments
    if (*argc > 2) {
        fprintf(stderr, "Error, Unexpected arguments!\n");
        exit(EXIT_FAILURE);
    }
    
    //Chooses username
    if (*argc == 1) {
        username = (char*)malloc(strlen(default_name)+1);
        strcpy(username, default_name);
    } else {
        username = (char*)malloc(strlen(argv[1])+1);
        strcpy(username, argv[1]);
    }

    prompt = (char*)malloc(strlen(username)+1);
    strcpy(prompt, username);
    prompt = strcat(prompt, PROMPT);
}

void loop() {
    //here it starts doing things
    char* command = NULL;
    int argno = 0;           //Number of arguments for the command
    char* args[MAX_ARG_NUM]; //Array of strings, containing the arguments to pass
    //TODO check if the number of arguments is exceeding (or make it dynamic)
    int exit_status = 0;
    int command_counter = 0;    //Successfully executed command counter
    char* last_command = NULL;

    //start loop
    while (1) {

        printf("%s", prompt);

        if (fgets(stdin_buff, MAX_BUFF, stdin) == NULL) {
            fprintf(stderr, "Error: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        prepare_command(&argno, args);

        //get che command
        command = (char*)malloc(strlen(args[0]) + 1);
        strcpy(command, args[0]);


        //commands
        if (strcmp(command, "") == 0) {
            continue;
        }
        else if (strcmp (command, "cd") == 0) {
            exit_status = cd(&argno, args);
        }
        else if (strcmp(command, "ls") == 0 || strcmp(command, "pwd") == 0) {
            exit_status = run_command(command, &argno, args);
        }
        else if (strcmp(command, "history") == 0) {
            prepare_history(-1, absolute_path, last_command, args);
            exit_status = run_command(command, &argno, args); //TODO: Serve davvero argno?
        }
        else if (strcmp (command, "exit") == 0) { //TODO: Frees allocated memory before exiting (da mettere dentro la exit)
            //Add exit command to .history file
            char last_command[] = "exit";
            prepare_history(command_counter, absolute_path, last_command, args);

            strcpy(command, "history");

            //Run "history" and check its exit status
            if (run_command(command, &argno, args) == EXIT_FAILURE) {
                fprintf(stderr, "Error while executing history\n");
            }
            exit(EXIT_SUCCESS);
        }
        else {
            exit_status = EXIT_FAILURE;
            printf("bash: %s: command not found\n", command);
        }

        //Check the exit status of the last command. Add it to history if it succeeded.
        if (exit_status == EXIT_SUCCESS) {
            last_command = (char*)realloc(last_command, strlen(command) + 1);
            strcpy(last_command, command);
            prepare_history(command_counter, absolute_path, last_command, args);
            strcpy (command, "history"); 

            //Run "history" and check its exit status
            if (run_command(command, &argno, args) == EXIT_FAILURE) {
                fprintf(stderr, "Error while executing history\n");
            } else {
                command_counter++;
            }

        }
        clear_args(args);
        argno = 0;
    }
    //return EXIT_SUCCESS;
}

void sigint_handler(int signo) {
    fprintf(stdout, "\nTo close myshell, type \"exit\"\n");
    printf("%s", prompt);
    fflush(stdout);
    return;
}

void exit_command(void) {
    fprintf(stdout, "Goodbye, %s!\n", username);
}

void prepare_command(int* argno, char* args[]) {
    int is_word = 0;
    char buff[MAX_BUFF] = "";

    for (int i = 0; i < strlen(stdin_buff); i++) {
        char c = stdin_buff[i];

        //Check if it's an alphanumerical character (i.e. plausible text)
        
        if (c != '\0' && c != ' ' && c != '\n') {
            is_word = 1;
            strncat(buff, &c, 1);   //Concatenates chars until assembling the first isolated word in the user input
        }

        if (c == ' ' || c == '\n') { 
            if (is_word == 0) {
                continue;
            } else {
                //Adds the word found to the arguments list
                args[*argno] = (char*)malloc(strlen(buff) + 1);
                strcpy(args[*argno], buff);
//              printf("%s\n", args[*argno]);
                (*argno)++;

                //Clears the buffer made for containing each word
                buff[0] = '\0';
                is_word = 0;
            }
        } 
    }
    return; 
}

void prepare_history(const int command_counter, const char* storage_path, const char* last_command, char* args[]) {
    /*
    args[1] = command_counter
    args[2] = storage_path
    args[3] = last_command

    If command_counter == -1, it only sets the first and the second arguments
    */

    //Allocates enough character, in order to containe the number command_counter converted into a string 

    if (command_counter >= 0) {
        //For a positive integer: SIZE = ceil(log10(command_counter)) + 1

        //int size = (int)(ceil(log10(command_counter))) + 1;
        args[1] = (char*)malloc((size_t)5);

        //Prepare the third argument
        args[3] = (char*)malloc(strlen(last_command) + 1);
        strcpy(args[3], last_command);

        args[4] = NULL; //TODO: bisogna trovare un modo di pulire char* args[], perché il vettore di argomenti deve terminare con un NULL, altrimenti legge più argomenti del previsto
    } else if (command_counter == -1) {
        args[1] = (char*)malloc(strlen("-1") + 1);

        args[3] = NULL;     //End of arguments array
    }

    //Prepare the first argument (convert the integer to a string by printing it in the string)
    sprintf(args[1], "%d", command_counter);

    //Prepare the second argument for both cases
    args[2] = (char*)malloc(strlen(storage_path) + 1);
    strcpy(args[2], storage_path);

    return;
}

int run_command(char* command, int* argno, char* args[]) {
        //start forking
        int status;
        pid_t pid;

        if ((pid = fork()) < 0) {
            print_error("forking");
            return EXIT_FAILURE;
        }
        else if (pid == 0) {
            printf("sono qui\n");
            //child process
            if (execvp(command, args) == EOF){
                print_error("executing child process");
                return EXIT_FAILURE;
            }
        }
        else {
            //parent process
            if (wait(&status) < 0) {
                print_error("waiting");
                return EXIT_FAILURE;
            }
		    if (WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS)
			    return EXIT_SUCCESS;
		    else
			    return EXIT_FAILURE;            
        }
        return EXIT_SUCCESS;
}

int cd(int* argno, char* args[]) {
	char* newdir;

	if (*argno > 2) {
		fprintf(stderr, "Unexpected arguments\n");
		return EXIT_FAILURE;	
	}
	
	if (*argno == 2) {
		newdir = (char*)malloc(strlen(args[1]) + 1);
		strcpy(newdir, args[1]);
		if (chdir(newdir) < 0) {
			print_error("opening");
			return EXIT_FAILURE;
		}
	}
    
    return EXIT_SUCCESS;
}

void clear_args(char* args[]) {
    
    for (int i = 0; i < MAX_ARG_NUM; i++) {
        args[i] = (char*)0;
    }
    return;
}

void print_error(const char str[]) {
    fprintf(stderr, "Error while %s [code %d, %s] \n", str, errno, strerror(errno));
    return;
}