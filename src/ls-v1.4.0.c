/*
 * ls-v1.4.0
 * Feature 5: Alphabetical sorting using qsort()
 * Sorts filenames A–Z in all display modes (-l, -x, default)
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

int read_names(const char *dir, char ***out_names, int *out_count, int *out_maxlen);
void free_names(char **names, int count);
void print_columns_down(char **names, int count, int maxlen);
void print_columns_across(char **names, int count, int maxlen);
void print_long_listing(const char *dir, char **names, int count);
int compare_names(const void *a, const void *b);

void do_default(const char *dir);
void do_horizontal(const char *dir);
void do_long(const char *dir);

int main(int argc, char *argv[])
{
    int opt, long_flag = 0, x_flag = 0;

    while ((opt = getopt(argc, argv, "lx")) != -1)
    {
        if (opt == 'l') long_flag = 1;
        else if (opt == 'x') x_flag = 1;
        else
        {
            fprintf(stderr, "Usage: %s [-l] [-x] [directory...]\n", argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (long_flag) x_flag = 0;

    if (optind == argc)
    {
        if (long_flag) do_long(".");
        else if (x_flag) do_horizontal(".");
        else do_default(".");
    }
    else
    {
        for (int i = optind; i < argc; i++)
        {
            printf("Directory listing of %s:\n", argv[i]);
            if (long_flag) do_long(argv[i]);
            else if (x_flag) do_horizontal(argv[i]);
            else do_default(argv[i]);
            if (i < argc - 1) putchar('\n');
        }
    }

    return EXIT_SUCCESS;
}

/* ---------- read + sort names ---------- */
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
    int count = 0;
    int maxlen = 0;

    errno = 0;
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.') continue;

        char *s = strdup(entry->d_name);
        if (!s) { perror("strdup"); closedir(dp); return -1; }

        names = realloc(names, sizeof(char*) * (count + 1));
        if (!names) { perror("realloc"); closedir(dp); return -1; }

        names[count++] = s;
        int len = strlen(s);
        if (len > maxlen) maxlen = len;
    }
    closedir(dp);

    if (count > 0)
        qsort(names, count, sizeof(char *), compare_names); // 🔹 Sort alphabetically

    *out_names = names;
    *out_count = count;
    *out_maxlen = maxlen;
    return 0;
}

/* comparison function for qsort */
int compare_names(const void *a, const void *b)
{
    const char *na = *(const char **)a;
    const char *nb = *(const char **)b;
    return strcasecmp(na, nb); // case-insensitive A–Z
}

/* free helper */
void free_names(char **names, int count)
{
    if (!names) return;
    for (int i = 0; i < count; i++) free(names[i]);
    free(names);
}

/* display modes */
void do_default(const char *dir)
{
    char **names = NULL; int count = 0; int maxlen = 0;
    if (read_names(dir, &names, &count, &maxlen) != 0) return;
    if (count > 0) print_columns_down(names, count, maxlen);
    free_names(names, count);
}

void do_horizontal(const char *dir)
{
    char **names = NULL; int count = 0; int maxlen = 0;
    if (read_names(dir, &names, &count, &maxlen) != 0) return;
    if (count > 0) print_columns_across(names, count, maxlen);
    free_names(names, count);
}

void do_long(const char *dir)
{
    char **names = NULL; int count = 0; int maxlen = 0;
    if (read_names(dir, &names, &count, &maxlen) != 0) return;
    if (count > 0) print_long_listing(dir, names, count);
    free_names(names, count);
}

/* column printing helpers (reuse from Feature 4) */
void print_columns_down(char **names, int count, int maxlen)
{
    struct winsize w; ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int termw = w.ws_col ? w.ws_col : 80;
    int spacing = 2;
    int colw = maxlen + spacing;
    int cols = termw / colw;
    if (cols < 1) cols = 1;
    int rows = (count + cols - 1) / cols;

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            int idx = c * rows + r;
            if (idx < count) printf("%-*s", colw, names[idx]);
        }
        putchar('\n');
    }
}

void print_columns_across(char **names, int count, int maxlen)
{
    struct winsize w; ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int termw = w.ws_col ? w.ws_col : 80;
    int spacing = 2;
    int colw = maxlen + spacing;
    int cols = termw / colw;
    if (cols < 1) cols = 1;
    int rows = (count + cols - 1) / cols;

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            int idx = r * cols + c;
            if (idx < count) printf("%-*s", colw, names[idx]);
        }
        putchar('\n');
    }
}

/* long listing reused from v1.3.0 (unchanged) */
void print_long_listing(const char *dir, char **names, int count)
{
    char path[PATH_BUF];
    for (int i = 0; i < count; i++) {
        snprintf(path, sizeof(path), "%s/%s", dir, names[i]);
        struct stat st;
        if (stat(path, &st) == -1) continue;

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

        printf("%c%s %2ld %-8s %-8s %8ld %s %s\n",
            type, perm, (long)st.st_nlink,
            pw?pw->pw_name:"?", gr?gr->gr_name:"?",
            (long)st.st_size, timebuf, names[i]);
    }
}
