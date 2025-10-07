
/*
 * ls-v1.3.0
 * Feature 4: horizontal column display (-x) + option parsing (-l and -x)
 *
 * Simple, readable, and safe implementation that:
 *  - Parses -l and -x (long and horizontal)
 *  - If -l is present, shows long listing (permissions, links, owner, group, size, mtime)
 *  - Else if -x is present, shows horizontal columns (across then down)
 *  - Else shows default columns (down then across)
 *
 * Keep this file as src/ls-v1.3.0.c
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

/* helpers */
int read_names(const char *dir, char ***out_names, int *out_count, int *out_maxlen);
void free_names(char **names, int count);
void print_columns_down(char **names, int count, int maxlen);
void print_columns_across(char **names, int count, int maxlen);
void print_long_listing(const char *dir, char **names, int count);

/* main file-level display functions */
void do_default(const char *dir);    /* down then across */
void do_horizontal(const char *dir); /* across then down */
void do_long(const char *dir);       /* -l */

int main(int argc, char *argv[])
{
    int opt;
    int long_flag = 0;
    int x_flag = 0;

    /* parse options -l and -x */
    while ((opt = getopt(argc, argv, "lx")) != -1) {
        if (opt == 'l') long_flag = 1;
        else if (opt == 'x') x_flag = 1;
        else {
            fprintf(stderr, "Usage: %s [-l] [-x] [directory...]\n", argv[0]);
            return EXIT_FAILURE;
        }
    }

    /* If both -l and -x given, -l takes precedence (like real ls) */
    if (long_flag) x_flag = 0;

    if (optind == argc) {
        /* no directories -> current */
        if (long_flag) do_long(".");
        else if (x_flag) do_horizontal(".");
        else do_default(".");
    } else {
        for (int i = optind; i < argc; i++) {
            printf("Directory listing of %s:\n", argv[i]);
            if (long_flag) do_long(argv[i]);
            else if (x_flag) do_horizontal(argv[i]);
            else do_default(argv[i]);
            if (i < argc - 1) putchar('\n');
        }
    }

    return EXIT_SUCCESS;
}

/* ---------- read names into dynamic array ---------- */
/* returns 0 on success, -1 on error (and prints perror) */
int read_names(const char *dir, char ***out_names, int *out_count, int *out_maxlen)
{
    DIR *dp = opendir(dir);
    if (!dp) {
        fprintf(stderr, "Cannot open directory: %s\n", dir);
        return -1;
    }

    struct dirent *entry;
    char **names = NULL;
    int count = 0;
    int maxlen = 0;

    errno = 0;
    while ((entry = readdir(dp)) != NULL) {
        if (entry->d_name[0] == '.') continue; /* skip hidden files */

        char *s = strdup(entry->d_name);
        if (!s) {
            perror("strdup");
            closedir(dp);
            free_names(names, count);
            return -1;
        }
        char **tmp = realloc(names, sizeof(char*) * (count + 1));
        if (!tmp) {
            perror("realloc");
            free(s);
            closedir(dp);
            free_names(names, count);
            return -1;
        }
        names = tmp;
        names[count++] = s;
        int len = (int)strlen(s);
        if (len > maxlen) maxlen = len;
    }

    if (errno != 0) perror("readdir");

    closedir(dp);

    *out_names = names;
    *out_count = count;
    *out_maxlen = maxlen;
    return 0;
}

void free_names(char **names, int count)
{
    if (!names) return;
    for (int i = 0; i < count; i++) free(names[i]);
    free(names);
}

/* ---------- default: down then across ---------- */
void do_default(const char *dir)
{
    char **names = NULL;
    int count = 0;
    int maxlen = 0;
    if (read_names(dir, &names, &count, &maxlen) != 0) return;
    if (count > 0) print_columns_down(names, count, maxlen);
    free_names(names, count);
}

/* ---------- horizontal: across then down ---------- */
void do_horizontal(const char *dir)
{
    char **names = NULL;
    int count = 0;
    int maxlen = 0;
    if (read_names(dir, &names, &count, &maxlen) != 0) return;
    if (count > 0) print_columns_across(names, count, maxlen);
    free_names(names, count);
}

/* ---------- long listing (-l) ---------- */
void do_long(const char *dir)
{
    char **names = NULL;
    int count = 0;
    int maxlen = 0;
    if (read_names(dir, &names, &count, &maxlen) != 0) return;
    if (count > 0) print_long_listing(dir, names, count);
    free_names(names, count);
}

/* ---------- print helpers ---------- */

void print_columns_down(char **names, int count, int maxlen)
{
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int termw = w.ws_col ? w.ws_col : 80;
    int spacing = 2;
    int colw = maxlen + spacing;
    int cols = termw / colw;
    if (cols < 1) cols = 1;
    int rows = (count + cols - 1) / cols;

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            int idx = c * rows + r; /* down then across */
            if (idx < count) printf("%-*s", colw, names[idx]);
        }
        putchar('\n');
    }
}

void print_columns_across(char **names, int count, int maxlen)
{
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int termw = w.ws_col ? w.ws_col : 80;
    int spacing = 2;
    int colw = maxlen + spacing;
    int cols = termw / colw;
    if (cols < 1) cols = 1;
    int rows = (count + cols - 1) / cols;

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            int idx = r * cols + c; /* across then down */
            if (idx < count) printf("%-*s", colw, names[idx]);
        }
        putchar('\n');
    }
}

/* long listing printing - simple and explainable */
void print_long_listing(const char *dir, char **names, int count)
{
    char path[PATH_BUF];
    for (int i = 0; i < count; i++) {
        /* build path */
        if (snprintf(path, sizeof(path), "%s/%s", dir, names[i]) >= (int)sizeof(path)) {
            fprintf(stderr, "path too long\n");
            continue;
        }

        struct stat st;
        if (stat(path, &st) == -1) {
            perror(path);
            continue;
        }

        /* file type */
        char type = '-';
        if (S_ISDIR(st.st_mode)) type = 'd';
        else if (S_ISLNK(st.st_mode)) type = 'l';
        else if (S_ISCHR(st.st_mode)) type = 'c';
        else if (S_ISBLK(st.st_mode)) type = 'b';
        else if (S_ISFIFO(st.st_mode)) type = 'p';
        else if (S_ISSOCK(st.st_mode)) type = 's';

        /* permissions */
        char perm[10];
        perm[0] = (st.st_mode & S_IRUSR) ? 'r' : '-';
        perm[1] = (st.st_mode & S_IWUSR) ? 'w' : '-';
        perm[2] = (st.st_mode & S_IXUSR) ? 'x' : '-';
        perm[3] = (st.st_mode & S_IRGRP) ? 'r' : '-';
        perm[4] = (st.st_mode & S_IWGRP) ? 'w' : '-';
        perm[5] = (st.st_mode & S_IXGRP) ? 'x' : '-';
        perm[6] = (st.st_mode & S_IROTH) ? 'r' : '-';
        perm[7] = (st.st_mode & S_IWOTH) ? 'w' : '-';
        perm[8] = (st.st_mode & S_IXOTH) ? 'x' : '-';
        perm[9] = '\0';

        /* link count */
        long links = (long) st.st_nlink;

        /* owner/group */
        struct passwd *pw = getpwuid(st.st_uid);
        struct group  *gr = getgrgid(st.st_gid);

        /* size */
        long size = (long) st.st_size;

        /* time */
        char timebuf[32];
        struct tm *tm = localtime(&st.st_mtime);
        strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", tm);

        /* print */
        printf("%c%s %2ld %-8s %-8s %8ld %s %s\n",
               type, perm, links,
               pw ? pw->pw_name : "?",
               gr ? gr->gr_name : "?",
               size, timebuf, names[i]);
    }
}
