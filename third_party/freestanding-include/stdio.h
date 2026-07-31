/* Minimal freestanding stdio.h for arm-none-eabi builds. */
#ifndef _FREESTANDING_STDIO_H
#define _FREESTANDING_STDIO_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
#define EOF (-1)
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#define BUFSIZ 512
typedef struct __FILE FILE;
extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;
FILE *fopen(const char *path, const char *mode);
int   fclose(FILE *stream);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
int   fflush(FILE *stream);
int   fseek(FILE *stream, long offset, int whence);
long  ftell(FILE *stream);
void  rewind(FILE *stream);
int   feof(FILE *stream);
int   ferror(FILE *stream);
void  clearerr(FILE *stream);
int   remove(const char *path);
int   rename(const char *oldpath, const char *newpath);
int   setbuf(FILE *stream, char *buf);
int   setvbuf(FILE *stream, char *buf, int mode, size_t size);
int   fgetc(FILE *stream);
int   getc(FILE *stream);
int   fputc(int c, FILE *stream);
int   putc(int c, FILE *stream);
int   ungetc(int c, FILE *stream);
char *fgets(char *s, int size, FILE *stream);
int   fputs(const char *s, FILE *stream);
int   printf(const char *format, ...);
int   fprintf(FILE *stream, const char *format, ...);
int   sprintf(char *str, const char *format, ...);
int   snprintf(char *str, size_t size, const char *format, ...);
int   vprintf(const char *format, __builtin_va_list ap);
int   vfprintf(FILE *stream, const char *format, __builtin_va_list ap);
int   vsprintf(char *str, const char *format, __builtin_va_list ap);
int   vsnprintf(char *str, size_t size, const char *format, __builtin_va_list ap);
int   puts(const char *s);
int   perror(const char *s);
#ifdef __cplusplus
}
#endif
#endif
