/* Minimal freestanding stdlib.h for arm-none-eabi builds. */
#ifndef _FREESTANDING_STDLIB_H
#define _FREESTANDING_STDLIB_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
void *malloc(size_t size);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
void  free(void *ptr);
void  abort(void);
void  exit(int status);
int   atoi(const char *nptr);
long  atol(const char *nptr);
int   abs(int j);
long  labs(long j);
long  strtol(const char *nptr, char **endptr, int base);
int   rand(void);
void  srand(unsigned int seed);
unsigned long strtoul(const char *nptr, char **endptr, int base);
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#ifdef __cplusplus
}
#endif
#endif
