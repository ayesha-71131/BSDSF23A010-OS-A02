/*
 * ls-v1.6.0
 * Feature 7 (Tasks 2 & 3): Recursive Listing (-R)
 * Adds -R flag and recursive traversal through subdirectories.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <errno.h>

#define PATH_BUF 1024

/* ---------- ANSI Color Codes ---------- */
#define COLOR_RESET   "\033[0m"
#define COLOR_BLUE    "\033[0;34m"
#define COLOR_GREEN   "\033[0;32m"
#define COLOR_RED     "\033[0;31m"
#define COLOR_MAGENTA "\033[0;35m"
#define COLOR_REVERSE "\033[7m"

/* ---------- Function Declarations ---------- */
int read_names(const char *dir, char ***out_names, int *out_count, int *out_maxlen);
void free_names(char **names, int count);
void print_columns_down(const char *dir, char **names, int count, int maxlen);
void print_columns_across(const char *dir, char **names, int count, int maxlen);
void print_long_listing(const char *dir, char **names, int count);
int compare_names(const void *a, const void *b);
void print_colored_name(const char *dir, const char *name);
void do_ls(const char *dir, int long_flag, int x_flag, int recursive_flag);

/* ---------- Main ---------- */
int main(int argc, char *argv[])
{
    int opt, long_flag = 0, x_flag = 0, recursive_flag = 0;

    while ((opt = getopt(argc, argv, "lxR")) != -1)
    {
        if (opt == 'l') long_flag = 1;
        else if (opt == 'x') x_flag = 1;
        else if (opt == 'R') recursive_flag = 1;
        else
        {
            fprintf(stderr, "Usage: %s [-l] [-x] [-R] [directory...]\n", argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (long_flag) x_flag = 0;

    if (optind == argc)
        do_ls(".", long_flag, x_flag, recursive_flag);
    else
    {
        for (int i = optind; i < argc; i++)
        {
            do_ls(argv[i], long_flag, x_flag, recursive_flag);
            if (i < argc - 1) putchar('\n');
        }
    }

    return EXIT_SUCCESS;
}

/* ---------- Read + Sort Filenames ---------- */
int read_names(const char *dir, char ***out_names, int *out_count, int *out_maxlen)
{
    DIR *dp = opendir(dir);
    if (!dp)
    {
        fprintf(stderr, "Cannot open directory: %s\n", dir);
        return -1;
    }

    struct dirent *entry;
    char **names = NULL;
    int count = 0, maxlen = 0;

    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.') continue;  // skip hidden
        char *s = strdup(entry->d_name);
        if (!s) { perror("strdup"); closedir(dp); return -1; }

        char **tmp = realloc(names, sizeof(char*) * (count + 1));
        if (!tmp) { perror("realloc"); closedir(dp); return -1; }
        names = tmp;

        names[count++] = s;
        int len = strlen(s);
        if (len > maxlen) maxlen = len;
    }
    closedir(dp);

    if (count > 0)
        qsort(names, count, sizeof(char *), compare_names);

    *out_names = names;
    *out_count = count;
    *out_maxlen = maxlen;
    return 0;
}

int compare_names(const void *a, const void *b)
{
    const char *na = *(const char **)a;
    const char *nb = *(const char **)b;
    return strcasecmp(na, nb);
}

void free_names(char **names, int count)
{
    if (!names) return;
    for (int i = 0; i < count; i++) free(names[i]);
    free(names);
}

/* ---------- Color Printing ---------- */
void print_colored_name(const char *dir, const char *name)
{
    char path[PATH_BUF];
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    struct stat st;
    if (lstat(path, &st) == -1)
    {
        printf("%s", name);
        return;
    }

    const char *color = COLOR_RESET;

    if (S_ISDIR(st.st_mode)) color = COLOR_BLUE;
    else if (S_ISLNK(st.st_mode)) color = COLOR_MAGENTA;
    else if (S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode) || S_ISSOCK(st.st_mode)) color = COLOR_REVERSE;
    else if (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) color = COLOR_GREEN;
    else if (strstr(name, ".tar") || strstr(name, ".gz") || strstr(name, ".zip")) color = COLOR_RED;

    printf("%s%s%s", color, name, COLOR_RESET);
}

/* ---------- Printing Helpers ---------- */
void print_columns_down(const char *dir, char **names, int count, int maxlen)
{
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int termw = w.ws_col ? w.ws_col : 80;
    int spacing = 2;
    int colw = maxlen + spacing;
    int cols = termw / colw;
    if (cols < 1) cols = 1;
    int rows = (count + cols - 1) / cols;

    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            int idx = c * rows + r;
            if (idx < count)
            {
                print_colored_name(dir, names[idx]);
                int pad = colw - (int)strlen(names[idx]);
                for (int s = 0; s < pad; s++) putchar(' ');
            }
        }
        putchar('\n');
    }
}

void print_columns_across(const char *dir, char **names, int count, int maxlen)
{
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int termw = w.ws_col ? w.ws_col : 80;
    int spacing = 2;
    int colw = maxlen + spacing;
    int cols = termw / colw;
    if (cols < 1) cols = 1;
    int rows = (count + cols - 1) / cols;

    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            int idx = r * cols + c;
            if (idx < count)
            {
                print_colored_name(dir, names[idx]);
                int pad = colw - (int)strlen(names[idx]);
                for (int s = 0; s < pad; s++) putchar(' ');
            }
        }
        putchar('\n');
    }
}

void print_long_listing(const char *dir, char **names, int count)
{
    char path[PATH_BUF];
    for (int i = 0; i < count; i++)
    {
        snprintf(path, sizeof(path), "%s/%s", dir, names[i]);
        struct stat st;
        if (lstat(path, &st) == -1) continue;

        char type = S_ISDIR(st.st_mode) ? 'd' : '-';
        if (S_ISLNK(st.st_mode)) type = 'l';

        char perm[10];
        perm[0]=(st.st_mode&S_IRUSR)?'r':'-';
        perm[1]=(st.st_mode&S_IWUSR)?'w':'-';
        perm[2]=(st.st_mode&S_IXUSR)?'x':'-';
        perm[3]=(st.st_mode&S_IRGRP)?'r':'-';
        perm[4]=(st.st_mode&S_IWGRP)?'w':'-';
        perm[5]=(st.st_mode&S_IXGRP)?'x':'-';
        perm[6]=(st.st_mode&S_IROTH)?'r':'-';
        perm[7]=(st.st_mode&S_IWOTH)?'w':'-';
        perm[8]=(st.st_mode&S_IXOTH)?'x':'-';
        perm[9]='\0';

        struct passwd *pw = getpwuid(st.st_uid);
        struct group *gr = getgrgid(st.st_gid);

        char timebuf[32];
        struct tm *tm = localtime(&st.st_mtime);
        strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", tm);

        printf("%c%s %2ld %-8s %-8s %8ld %s ",
            type, perm, (long)st.st_nlink,
            pw?pw->pw_name:"?", gr?gr->gr_name:"?",
            (long)st.st_size, timebuf);

        print_colored_name(dir, names[i]);
        putchar('\n');
    }
}

/* ---------- Core Recursive Logic ---------- */
void do_ls(const char *dir, int long_flag, int x_flag, int recursive_flag)
{
    char **names = NULL; int count = 0; int maxlen = 0;
    if (read_names(dir, &names, &count, &maxlen) != 0) return;

    printf("%s:\n", dir);
    if (count == 0) { putchar('\n'); free_names(names, count); return; }

    if (long_flag)
        print_long_listing(dir, names, count);
    else if (x_flag)
        print_columns_across(dir, names, count, maxlen);
    else
        print_columns_down(dir, names, count, maxlen);

    /* Recursive step for subdirectories */
    if (recursive_flag)
    {
        for (int i = 0; i < count; i++)
        {
            char path[PATH_BUF];
            snprintf(path, sizeof(path), "%s/%s", dir, names[i]);
            struct stat st;
            if (lstat(path, &st) == -1) continue;

            if (S_ISDIR(st.st_mode) &&
                strcmp(names[i], ".") != 0 &&
                strcmp(names[i], "..") != 0)
            {
                putchar('\n');
                do_ls(path, long_flag, x_flag, recursive_flag);
            }
        }
    }

    free_names(names, count);
}
