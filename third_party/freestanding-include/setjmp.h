/* Minimal freestanding setjmp.h for arm-none-eabi builds. */
#ifndef _FREESTANDING_SETJMP_H
#define _FREESTANDING_SETJMP_H
typedef char jmp_buf[64];
int  setjmp(jmp_buf env);
void longjmp(jmp_buf env, int val);
#endif
