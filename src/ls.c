#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>

#define MAX_BUFF 256
#define FILES 0
#define DIRS 1
#define ALL 2

struct file {
    char* name;     /*!< File name */
    struct file* pun;   /*!< Pointer to file i-node */
    struct stat sb; /*!< File's i-node*/
};

struct option {
    int l_info;     /*!< Flag for printing the files' i-nodes */
    int a_hidden;   /*!< Flag for printing all the files (hidden included) */
    int desired;    /*!< Flag for desired output: 0 files, 1 directories, 2 all */
    int help;       /*!< Flag for printing help */
};

char* ddir = NULL; /* desired directory */

//void prepare_command(int, char**);
void add_to_list(struct file**, const struct stat*, const char* name);
void free_list(struct file**);
void print_error (const char*);
void print_list(struct file*, const char*, struct option*);
void print_help();
struct option options(int, char**);

FILE* error_stream = NULL;    /* Error output stream */

/**
 *  \brief          This function emulates the ls command from the Unix shell.
 *  \note           The options must be preceded by a dash '-'.
 *  \param[in]      argv[1]: Desired path on which the ls will be executed
 *  \param[in]      argv[2]: Options (Try "ls -h" for more informations).
 *  \return         Returns 0 on success, 1 on error. 
*/
int
main (int argc, char* argv[]) {
    
    error_stream = stderr;

    /* Cheks the number of arguments passed */
    if (argc > 9) {   /* Maximum number of arguments, considering the path and all the 5 options selected */
        fprintf(error_stream, "Unexpected arguments!\n");
        exit(EXIT_FAILURE);
    }

    /* Redirects the error output stream to the specified file in the arguments (if present) */
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "2>") == 0) {
            if (argv[i + 1] != NULL) {
                if ((error_stream = fopen(argv[i + 1], "a")) == NULL) {
                    fprintf(error_stream,"Error while opening the error output stream\n");
                    exit(EXIT_FAILURE);
                }
                /* Set the error stream to unbeffered */
                if (setvbuf(error_stream, NULL, _IONBF, 0) != 0) {
                    fprintf(error_stream, "Error while setting the error stream as unbuffered: %s", strerror(errno));
                    exit(EXIT_FAILURE);
                }
            } else {
                fprintf(error_stream, "Wrong arguments! Did you mean \"2> error_output_file\"?\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    /* Gets options */
    struct option opt = options(argc, argv);

    /* Prints the help section and terminates the process */
    if (opt.help) {
        print_help();
        return EXIT_SUCCESS;
    }

    /* Updates number of argumetns (excluding redirection arguments) */
    if (error_stream != stderr) {
        argc = argc - 2;
    }

    /* Sets "path_arg_num" to the index of the argument containing the path on which the ls will be executed */
    int path_arg_num = 1;   /* Index of the path argument. Starts from 1 (excludes the program name) */

    /* Scans the arguments, until it either reaches the end or it finds a "non-option" argument */
    while ((path_arg_num < argc) && (strchr(argv[path_arg_num], '-') != NULL)) {
        path_arg_num++;
    }

    /* Open the specified path (if present); otherwise it opens the current directory*/
    if ((path_arg_num < argc) && (argv[path_arg_num] != NULL)) {
        if (chdir(argv[path_arg_num]) < 0) {
            fprintf(error_stream, "ls: cannot access '%s': No such file or directory\n", argv[path_arg_num]);
            return EXIT_FAILURE;
        }
    }

    DIR* dp;
    int fd;

    /* Open destination directory */
    dp = opendir(getcwd(NULL, 0));

    if (dp == NULL) {
        print_error("opening directory");
        return EXIT_FAILURE;
    }

    struct file* files = NULL;
    struct file* dirs = NULL;

    struct dirent* dirp;

    /* Scan each entry of the directory */
    while ((dirp = readdir(dp)) != NULL) {

        struct stat sb;
        
        if (lstat(dirp->d_name, &sb) < 0){
            print_error("opening i-node");
            return EXIT_FAILURE;
        }

        /* Sort files and directories in the right list */
        if (S_ISDIR(sb.st_mode) > 0) {
            add_to_list (&dirs, &sb, dirp->d_name);
        } else {
            add_to_list (&files, &sb, dirp->d_name);
        }
    }
    closedir(dp);

    /*print the lists*/
    if (opt.desired != FILES)
        print_list(dirs, "--> Directories:", &opt);
    if (opt.desired != DIRS)
        print_list(files, "--> Files:", &opt);

    /*free heap*/
    free_list(&files);
    free_list(&dirs);

    return EXIT_SUCCESS;
}

/**
 * \brief           This function parses the argument string and select the desired options
 * \return          Returns an option structure containing the flags relative to the desired options.
*/
struct option
options(int argc, char** argv) {
    
    struct option opt;
    opt.a_hidden = 0;
    opt.l_info = 0;
    opt.desired = ALL;
    opt.help = 0;

    /* Parses the arguments array to find the set options. It automatically discards the */
    /* arguments not written with the format "-" + option */
    char ch;
    while ((ch = getopt (argc, argv, "ladfh")) != -1) {

        switch (ch) {
            case 'l':
                opt.l_info = 1;     /*include info*/
                break;
            case 'a':
                opt.a_hidden = 1;   /*show hidden files*/
                break;
            case 'd':
                opt.desired = DIRS; /*show directories only*/
                break;
            case 'f':
                opt.desired = FILES; /*show files only*/
                break;
            case 'h':
                opt.help = 1; /*help*/
                break;
            default:
                fprintf(error_stream, "Try 'ls -h' for more information\n");
                exit(EXIT_FAILURE);
        }
    }

    return opt;
}

/**
 *  \brief          This function appends on top the passed entry of the directory to the specified list.
 *  \param[in]      last: Pointer to the address of the last element of the passed list.
 *  \param[in]      sb: Pointer to the file's i-node
 *  \param[in]      name: String of the file's name
*/
void
add_to_list (struct file** last, const struct stat* sb, const char* name) {

    struct file* new_elem = malloc (sizeof (struct file));

    if (new_elem == NULL) {
        print_error ("allocating");
        exit(EXIT_FAILURE);
    }

    new_elem->name = calloc (strlen (name) + 1, sizeof(char));
    if (new_elem->name == NULL) {
        print_error ("allocating");
        exit(EXIT_FAILURE);
    }
    strcpy (new_elem->name, name);

    /*adding the new element at the beggining of the list*/
    new_elem->sb = *sb;
    new_elem->pun = *last;
    *last = new_elem;

    return;
}

/**
 *  \brief          This function prints the final output of the ls command, based on the selected options.
 *  \param[in]      list: List of files inside the desired directory (pointer to the last element of the list).
 *  \param[in]      title: First line of the output.
 *  \param[in]      opt: Selected option flag structure.
*/
void
print_list (struct file* list, const char* title, struct option* opt) {

    printf("%s\n", title);

    for (struct file* pivot = list; pivot != NULL; pivot = pivot->pun) {

        if (!(opt->a_hidden) && *(pivot->name) == '.') /*continue if it is an hidden file*/
            continue;
        if (opt->l_info) { /*show options*/

            fprintf(stdout, "- ");

            fprintf(stdout, "%c", !S_ISREG(pivot->sb.st_mode) ? 's' : '-');

            /*permissions*/
            fprintf(stdout, "%c", (pivot->sb.st_mode & S_IRUSR) ? 'r' : '-');
                        fprintf(stdout, "%c", (pivot->sb.st_mode & S_IWUSR) ? 'w' : '-');
                        fprintf(stdout, "%c", (pivot->sb.st_mode & S_IXUSR) ? 'x' : '-');
                        fprintf(stdout, "%c", (pivot->sb.st_mode & S_IRGRP) ? 'r' : '-');
                        fprintf(stdout, "%c", (pivot->sb.st_mode & S_IWGRP) ? 'w' : '-');
                        fprintf(stdout, "%c", (pivot->sb.st_mode & S_IXGRP) ? 'x' : '-');
                        fprintf(stdout, "%c", (pivot->sb.st_mode & S_IROTH) ? 'r' : '-');
                        fprintf(stdout, "%c", (pivot->sb.st_mode & S_IWOTH) ? 'w' : '-');
                        fprintf(stdout, "%c", (pivot->sb.st_mode & S_IXOTH) ? 'x' : '-');

            /*hard links*/
            fprintf(stdout, "\t%d", pivot->sb.st_nlink);

            /*owners*/
            struct passwd *pw = getpwuid(pivot->sb.st_uid);
            struct group *gr = getgrgid(pivot->sb.st_gid);
			fprintf(stdout, "\t%s\t%s", pw->pw_name, gr->gr_name);

            /*size*/
			fprintf(stdout, "\t%d", pivot->sb.st_size);

            /*name*/
			fprintf(stdout, "\t%s\n", pivot->name);

        } else /*show only the file name*/
            printf("\t- %s\n", pivot->name);
    }
    return;
}

/**
 *  \brief          This function deallocates the reserved space in the heap memory used for the lists.
 *  \param[in]      files: Pointer to the address of the last element of the list, which has to be freed.
*/
void
free_list (struct file** files) {

    if (*files == NULL)
        return;

    for (struct file* pivot = *files; *files != NULL; *files = pivot) {
        pivot = (*files)->pun;
        free((*files)->name);
        free(*files);
    }

    return;
}

/**
 *  \brief          This function prints the help section for the ls command.
*/
void
print_help() {
	const char help[] =
		"\tList information about the FILEs (the current directory by default).\n\
		\t-a\tShow hidden files.\n\
		\t-l\tShow info.\n\
		\t-h\tPrint help page.\n\
		\t-f\tShow files only.\n\
		\t-d\tShow directories only.\n\
		\n";

	fprintf(stdout, "%s", help);
}

/**
 * \brief           This function prints an error message on error_stream, with format: "Error while *your string*
 *                  [code *errno*, *strerror(errno)*]"
 * \param[in]       str: Your costant string, which will be pasted inside the error message.
*/
void
print_error (const char str[]) {
    fprintf(error_stream, "Error while %s [code %d, %s] \n", str, errno, strerror(errno));
    return;
}