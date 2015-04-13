#ifndef TASK_H
#define TASK_H

#include <inc/trap.h>
#define NR_TASKS	5
#define TIME_QUANT	1

typedef enum
{
	TASK_FREE = 0,
	TASK_RUNNABLE,
	TASK_RUNNING,
	TASK_SLEEP,
	TASK_STOP,
} TaskState;

// Each task's user space
#define USR_STACK_SIZE	(40960)
#define KERN_STACK_SIZE	(8*4096)

typedef struct
{
	int task_id;
	int parent_id;
	struct Trapframe tf; //Saved registers
	int32_t remind_ticks; //time quantum
	TaskState state;	//Task state

	char usr_stack[USR_STACK_SIZE];
	
} Task;

int sys_fork();
void sys_kill(int pid);
void env_pop_tf(struct Trapframe *tf);
#endif
