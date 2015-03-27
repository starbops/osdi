#include <kernel/task.h>
#include <kernel/syscall.h>
#include <inc/trap.h>

void do_puts(char *str, uint32_t len)
{
	uint32_t i;
	for (i = 0; i < len; i++)
	{
		k_putch(str[i]);
	}
}

int32_t do_getc()
{
	return k_getc();
}

int32_t do_syscall(uint32_t syscallno, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
{
	int32_t retVal = -1;
	extern Task *cur_task;

	switch (syscallno)
	{
	case SYS_getc:
		retVal = do_getc();
		break;
	case SYS_puts:
		do_puts((char*)a1, a2);
		retVal = 0;

		break;
	case SYS_fork:
		/* Lab4 TODO: create process */
		retVal = sys_fork(); //In task.c
		break;
	case SYS_getpid:
		/* Lab4: get current task's pid */
		retVal = cur_task->task_id;
		break;

	case SYS_sleep:
		/* Lab4 TODO: set task to sleep state and yield this task. */

		break;
	case SYS_kill:
		/* Lab4 TODO: kill task. */
		sys_kill(a1);
		retVal = 0;
		break;
	}
	return retVal;
}

static void syscall_handler(struct Trapframe *tf)
{
	int32_t ret = -1;
	/* Lab4 TODO: call do_syscall and pass the parmeters from tf */

	/* Set system return value */
	tf->tf_regs.reg_eax = ret;

}


static inline int32_t
syscall(int num, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
{
	int32_t ret;

	// Generic system call: pass system call number in AX,
	// up to five parameters in DX, CX, BX, DI, SI.
	// Interrupt kernel with T_SYSCALL.
	//
	// The "volatile" tells the assembler not to optimize
	// this instruction away just because we don't use the
	// return value.
	//
	// The last clause tells the assembler that this can
	// potentially change the condition codes and arbitrary
	// memory locations.

	asm volatile("int %1\n"
		: "=a" (ret)
		: "i" (T_SYSCALL),
		  "a" (num),
		  "d" (a1),
		  "c" (a2),
		  "b" (a3),
		  "D" (a4),
		  "S" (a5)
		: "cc", "memory");

	//if(check && ret > 0)
	//	panic("syscall %d returned %d (> 0)", num, ret);

	return ret;
}

void
sys_cputs(const char *s, size_t len)
{
	syscall(SYS_puts,(uint32_t)s, len, 0, 0, 0);
}

int
sys_cgetc(void)
{
	return syscall(SYS_getc, 0, 0, 0, 0, 0);
}

void sleep(uint32_t ticks)
{
	syscall(SYS_sleep, ticks, 0, 0, 0, 0);
}

int32_t fork(void)
{
	return syscall(SYS_fork, 0, 0, 0, 0, 0);
}

int32_t getpid(void)
{
	return syscall(SYS_getpid, 0, 0, 0, 0, 0);
}

void kill_self()
{
	int pid;
	pid = getpid();
	syscall(SYS_kill, pid, 0, 0, 0, 0);
}
void syscall_init()
{
	/* Initial syscall trap after trap_init()*/
	/* Lab4 TODO: Register system call's trap handler (syscall_handler)*/

}

