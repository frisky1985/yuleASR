/* Minimal freestanding dirent.h for arm-none-eabi builds. */
#ifndef _FREESTANDING_DIRENT_H
#define _FREESTANDING_DIRENT_H
struct dirent {
    long d_ino;
    char d_name[256];
};
typedef struct DIR DIR;
DIR *opendir(const char *name);
struct dirent *readdir(DIR *dirp);
int closedir(DIR *dirp);
#endif
