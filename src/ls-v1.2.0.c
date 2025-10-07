/*
* Programming Assignment 02: ls-v1.2.0
* Adds column display (down then across)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>

extern int errno;

void do_ls(const char *dir);
void print_columns(char **names, int count);

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        do_ls(".");
    }
    else
    {
        for (int i = 1; i < argc; i++)
        {
            printf("Directory listing of %s:\n", argv[i]);
            do_ls(argv[i]);
            puts("");
        }
    }
    return 0;
}

void do_ls(const char *dir)
{
    DIR *dp;
    struct dirent *entry;

    dp = opendir(dir);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory: %s\n", dir);
        return;
    }

    // Store filenames in array
    char **names = NULL;
    int count = 0;
    size_t max_len = 0;

    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue; // skip hidden files

        names = realloc(names, sizeof(char *) * (count + 1));
        names[count] = strdup(entry->d_name);
        if (strlen(entry->d_name) > max_len)
            max_len = strlen(entry->d_name);
        count++;
    }
    closedir(dp);

    if (count == 0)
        return;

    // Print names in columns
    print_columns(names, count);

    // Free memory
    for (int i = 0; i < count; i++)
        free(names[i]);
    free(names);
}

void print_columns(char **names, int count)
{
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int term_width = w.ws_col ? w.ws_col : 80;

    int max_len = 0;
    for (int i = 0; i < count; i++)
        if ((int)strlen(names[i]) > max_len)
            max_len = strlen(names[i]);

    int spacing = 2;
    int col_width = max_len + spacing;
    int cols = term_width / col_width;
    if (cols < 1) cols = 1;
    int rows = (count + cols - 1) / cols;

    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            int index = c * rows + r;
            if (index < count)
                printf("%-*s", col_width, names[index]);
        }
        printf("\n");
    }
}
