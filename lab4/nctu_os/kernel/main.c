#include <inc/stdio.h>
#include <inc/kbd.h>
#include <inc/shell.h>
#include <inc/timer.h>
#include <inc/x86.h>
#include <kernel/trap.h>
#include <kernel/picirq.h>
#include <kernel/task.h>

extern void init_video(void);
extern Task *cur_task;

void kernel_main(void)
{
	extern void task_job();

	init_video();

	trap_init();
	pic_init();
	kbd_init();
	timer_init();
	syscall_init();

	task_init();

	/* Enable interrupt */
	__asm __volatile("sti");
	/* Move to user mode */
	asm volatile("movl %0,%%eax\n\t" \
	"pushl %1\n\t" \
	"pushl %%eax\n\t" \
	"pushfl\n\t" \
	"pushl %2\n\t" \
	"pushl $1f\n\t" \
	"iret\n" \
	"1:\tmovl %1,%%eax\n\t" \
	"movw %%ax,%%ds\n\t" \
	"movw %%ax,%%es\n\t" \
	"movw %%ax,%%fs\n\t" \
	"movw %%ax,%%gs" \
	:: "m" (cur_task->tf.tf_esp), "i" (GD_UD | 0x03), "i" (GD_UT | 0x03)
	:"ax");

	/* Below code is running on user mode */
	if (fork())
	{

		/* Child */
		if (fork()) task_job(); 
		else{
		if (fork()) task_job(); else
		if (fork()) task_job(); else task_job();}
		
	}
	else
	{
		shell();
	}
	/* task recycle */
	kill_self();
	for(;;){};

}
