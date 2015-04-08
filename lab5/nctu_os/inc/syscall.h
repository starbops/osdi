#ifndef USR_SYSCALL_H
#define USR_SYSCALL_H
#include <inc/types.h>

/* system call numbers */
enum {
	SYS_puts = 0,
	SYS_getc,
	SYS_getpid,
	SYS_fork,
	SYS_kill,
	SYS_sleep,

	NSYSCALLS
};

int32_t fork(void);

int32_t getpid(void);

void kill_self();

void sleep(uint32_t ticks);

void sys_cputs(const char *s, size_t len);
int sys_cgetc(void);
#endif
