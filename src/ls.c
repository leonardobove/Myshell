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
    char* name;
    struct file* pun;
    struct stat sb;
};

struct option {
    int l_info;
    int a_hidden;
    int desired; //0 files, 1 directories, 2 all
};
char* ddir = NULL; //desired directory

//void prepare_command(int, char**);
void add_to_list(struct file**, const struct stat*, const char* name);
void free_list(struct file**);
void print_error (const char str[]);
struct option options(int, char**);
void print_list(struct file*, const char*, struct option*);

int main (int argc, char* argv[]) {
    DIR* dp;
    int fd;

    struct option opt = options(argc, argv);

    char* bin_path = NULL;
    if (argv[1] != NULL) {
        chdir(argv[1]);
        bin_path = getcwd(NULL, 0);
        dp = opendir(bin_path);
    }
    else {
        dp = opendir(".");
    }

    if (dp == NULL) {
        print_error("opening directory");
        return EXIT_FAILURE;
    }

    struct file* files = NULL;
    struct file* dirs = NULL;

    struct dirent* dirp;

    while ((dirp = readdir(dp)) != NULL) {

        struct stat sb;
        
        if (lstat(dirp->d_name, &sb) < 0){
            print_error("opening i-node");
            return EXIT_FAILURE;
        }

        
        if (S_ISDIR(sb.st_mode) > 0) {
            add_to_list (&dirs, &sb, dirp->d_name);
        } else {
            add_to_list (&files, &sb, dirp->d_name);
        }
    }
    closedir(dp);

    if (opt.desired != FILES)
        print_list(dirs, "--> Directories:", &opt);
    if (opt.desired != DIRS)
        print_list(files, "--> Files:", &opt);

    //free heap
    free_list(&files);
    free_list(&dirs);

    return EXIT_SUCCESS;
}

struct option options(int argc, char **argv) {
    
    struct option opt;
    opt.a_hidden = 0;
    opt.l_info = 0;
    opt.desired = ALL;

    char ch;
    while ((ch = getopt (argc, argv, "ladf")) != -1 ) {

        switch (ch) {
            case 'l':
                opt.l_info = 1;
                break;
            case 'a':
                opt.a_hidden = 1;
                break;
            case 'd':
                opt.desired = DIRS;
                break;
            case 'f':
                opt.desired = FILES;
                break;
            default:
                break;
        }
    }

    return opt;
}

void add_to_list (struct file** last, const struct stat* sb, const char* name) {

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

    new_elem->sb = *sb;
    new_elem->pun = *last;
    *last = new_elem;

    return;
}

void print_list (struct file* list, const char* title, struct option* opt) {

    printf("%s\n", title);

    for (struct file* pivot = list; pivot != NULL; pivot = pivot->pun) {

        if (!(opt->a_hidden) && *(pivot->name) == '.')
            continue;
        if (opt->l_info) {

            fprintf(stdout, "- ");

            fprintf(stdout, "%c", !S_ISREG(pivot->sb.st_mode) ? 's' : '-');

            fprintf(stdout, "%c", (pivot->sb.st_mode & S_IRUSR) ? 'r' : '-');
                        fprintf(stdout, "%c", (pivot->sb.st_mode & S_IWUSR) ? 'w' : '-');
                        fprintf(stdout, "%c", (pivot->sb.st_mode & S_IXUSR) ? 'x' : '-');
                        fprintf(stdout, "%c", (pivot->sb.st_mode & S_IRGRP) ? 'r' : '-');
                        fprintf(stdout, "%c", (pivot->sb.st_mode & S_IWGRP) ? 'w' : '-');
                        fprintf(stdout, "%c", (pivot->sb.st_mode & S_IXGRP) ? 'x' : '-');
                        fprintf(stdout, "%c", (pivot->sb.st_mode & S_IROTH) ? 'r' : '-');
                        fprintf(stdout, "%c", (pivot->sb.st_mode & S_IWOTH) ? 'w' : '-');
                        fprintf(stdout, "%c", (pivot->sb.st_mode & S_IXOTH) ? 'x' : '-');

            fprintf(stdout, "\t%d", pivot->sb.st_nlink);

            struct passwd *pw = getpwuid(pivot->sb.st_uid);
            struct group *gr = getgrgid(pivot->sb.st_gid);
			fprintf(stdout, "\t%s\t%s", pw->pw_name, gr->gr_name);

			fprintf(stdout, "\t%d", pivot->sb.st_size);

			fprintf(stdout, "\t%s\n", pivot->name);

        } else
            printf("\t- %s\n", pivot->name);
    }
    return;
}

void free_list (struct file** files) {

    if (*files == NULL)
        return;

    for (struct file* pivot = *files; *files != NULL; *files = pivot) {
        pivot = (*files)->pun;
        free((*files)->name);
        free(*files);
    }

    return;
}

void print_error (const char str[]) {
    fprintf(stderr, "Error while %s [code %d, %s] \n", str, errno, strerror(errno));
    return;
}