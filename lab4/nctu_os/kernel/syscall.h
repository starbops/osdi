#ifndef SYS_CALL_H
#define SYS_CALL_H

#include <inc/types.h>
#include <inc/syscall.h>
int32_t do_syscall(uint32_t syscallno, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5);

void syscall_init();
#endif
