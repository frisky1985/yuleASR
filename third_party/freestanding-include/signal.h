/* Minimal freestanding signal.h for arm-none-eabi builds. */
#ifndef _FREESTANDING_SIGNAL_H
#define _FREESTANDING_SIGNAL_H
typedef int sig_atomic_t;
typedef void (*sighandler_t)(int);
#define SIG_DFL ((sighandler_t)0)
#define SIG_IGN ((sighandler_t)1)
#define SIG_ERR ((sighandler_t)-1)
#define SIGINT  2
#define SIGABRT 6
#define SIGTERM 15
sighandler_t signal(int signum, sighandler_t handler);
int raise(int signum);
#endif
