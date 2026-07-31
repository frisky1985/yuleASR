/* Minimal freestanding assert.h for arm-none-eabi builds. */
#ifndef _FREESTANDING_ASSERT_H
#define _FREESTANDING_ASSERT_H
#ifdef NDEBUG
#define assert(expr) ((void)0)
#else
void __assert_fail(const char *expr, const char *file, int line);
#define assert(expr) \
    ((expr) ? (void)0 : __assert_fail(#expr, __FILE__, __LINE__))
#endif
#endif
