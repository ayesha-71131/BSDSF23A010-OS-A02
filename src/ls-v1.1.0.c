/*
* Programming Assignment 02: ls-v1.1.0
* Adds support for -l (long listing)
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

extern int errno;

void do_ls(const char *dir);
void do_ls_long(const char *dir);
void print_file_info(const char *path, const char *name);
void print_permissions(mode_t mode);

int main(int argc, char *argv[])
{
    int opt;
    int long_flag = 0;

    // detect -l option using getopt
    while ((opt = getopt(argc, argv, "l")) != -1)
    {
        if (opt == 'l')
            long_flag = 1;
        else
        {
            fprintf(stderr, "Usage: %s [-l] [directory...]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    // if no directory is given, use current (.)
    if (optind == argc)
    {
        if (long_flag)
            do_ls_long(".");
        else
            do_ls(".");
    }
    else
    {
        for (int i = optind; i < argc; i++)
        {
            printf("Directory listing of %s :\n", argv[i]);
            if (long_flag)
                do_ls_long(argv[i]);
            else
                do_ls(argv[i]);
            puts("");
        }
    }

    return 0;
}

/* -------- Simple display (original version) -------- */
void do_ls(const char *dir)
{
    struct dirent *entry;
    DIR *dp = opendir(dir);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory: %s\n", dir);
        return;
    }

    errno = 0;
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue; // skip hidden
        printf("%s\n", entry->d_name);
    }

    if (errno != 0)
        perror("readdir failed");

    closedir(dp);
}

/* -------- Long listing display -------- */
void do_ls_long(const char *dir)
{
    struct dirent *entry;
    DIR *dp = opendir(dir);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory: %s\n", dir);
        return;
    }

    char path[1024];
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue; // skip hidden

        snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name);
        print_file_info(path, entry->d_name);
    }

    closedir(dp);
}

/* -------- Helper: print info of one file -------- */
void print_file_info(const char *path, const char *name)
{
    struct stat st;
    if (stat(path, &st) == -1)
    {
        perror("stat");
        return;
    }

    // 1. Permissions
    print_permissions(st.st_mode);

    // 2. Link count
    printf(" %2ld", (long)st.st_nlink);

    // 3. Owner and group names
    struct passwd *pw = getpwuid(st.st_uid);
    struct group *gr = getgrgid(st.st_gid);
    printf(" %-8s %-8s", pw ? pw->pw_name : "?", gr ? gr->gr_name : "?");

    // 4. File size
    printf(" %8ld", (long)st.st_size);

    // 5. Last modification time
    char timebuf[32];
    struct tm *tm_info = localtime(&st.st_mtime);
    strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", tm_info);
    printf(" %s", timebuf);

    // 6. File name
    printf(" %s\n", name);
}

/* -------- Helper: convert st_mode to rwx string -------- */
void print_permissions(mode_t mode)
{
    // file type
    if (S_ISDIR(mode))
        printf("d");
    else if (S_ISLNK(mode))
        printf("l");
    else
        printf("-");

    // owner
    printf("%c", (mode & S_IRUSR) ? 'r' : '-');
    printf("%c", (mode & S_IWUSR) ? 'w' : '-');
    printf("%c", (mode & S_IXUSR) ? 'x' : '-');

    // group
    printf("%c", (mode & S_IRGRP) ? 'r' : '-');
    printf("%c", (mode & S_IWGRP) ? 'w' : '-');
    printf("%c", (mode & S_IXGRP) ? 'x' : '-');

    // others
    printf("%c", (mode & S_IROTH) ? 'r' : '-');
    printf("%c", (mode & S_IWOTH) ? 'w' : '-');
    printf("%c", (mode & S_IXOTH) ? 'x' : '-');
}
