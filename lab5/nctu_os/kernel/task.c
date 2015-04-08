#include <inc/mmu.h>
#include <inc/types.h>
#include <inc/x86.h>
#include <kernel/task.h>

// Global descriptor table.
//
// Set up global descriptor table (GDT) with separate segments for
// kernel mode and user mode.  Segments serve many purposes on the x86.
// We don't use any of their memory-mapping capabilities, but we need
// them to switch privilege levels. 
//
// The kernel and user segments are identical except for the DPL.
// To load the SS register, the CPL must equal the DPL.  Thus,
// we must duplicate the segments for the user and the kernel.
//
// In particular, the last argument to the SEG macro used in the
// definition of gdt specifies the Descriptor Privilege Level (DPL)
// of that descriptor: 0 for kernel and 3 for user.
//
struct Segdesc gdt[6] =
{
	// 0x0 - unused (always faults -- for trapping NULL far pointers)
	SEG_NULL,

	// 0x8 - kernel code segment
	[GD_KT >> 3] = SEG(STA_X | STA_R, 0x0, 0xffffffff, 0),

	// 0x10 - kernel data segment
	[GD_KD >> 3] = SEG(STA_W, 0x0, 0xffffffff, 0),

	// 0x18 - user code segment
	[GD_UT >> 3] = SEG(STA_X | STA_R, 0x0, 0xffffffff, 3),

	// 0x20 - user data segment
	[GD_UD >> 3] = SEG(STA_W , 0x0, 0xffffffff, 3),

	// First TSS descriptors (starting from GD_TSS0) are initialized
	// in task_init()
	[GD_TSS0 >> 3] = SEG_NULL
	
};

struct Pseudodesc gdt_pd = {
	sizeof(gdt) - 1, (unsigned long) gdt
};



static struct tss_struct tss;
volatile Task tasks[NR_TASKS];

volatile char kern_stack[KERN_STACK_SIZE];

volatile Task *cur_task = NULL; //Current running task


//Find an avaiable task slot and setup 
int task_create()
{
	/* Find a free task structure */
	int i = 0;
	Task *ts = NULL;
	for (i = 0; i < NR_TASKS; i++)
	{
		if (tasks[i].state == TASK_FREE || tasks[i].state == TASK_STOP)
		{
			ts = &(tasks[i]);
			break;
		}
	}
	if (i >= NR_TASKS)
		return -1;

	/* Initial Trapframe */
	memset( &(ts->tf), 0, sizeof(ts->tf));

	ts->tf.tf_cs = GD_UT | 0x03;
	ts->tf.tf_ds = GD_UD | 0x03;
	ts->tf.tf_es = GD_UD | 0x03;
	ts->tf.tf_ss = GD_UD | 0x03;
	ts->tf.tf_esp = ts->usr_stack + USR_STACK_SIZE;
	
	/* Lab4 TODO: Setup task structure (task_id and parent_id) */
	ts->task_id = i;
	ts->state = TASK_RUNNABLE;
	ts->parent_id = (cur_task == NULL)? 0: cur_task->task_id;
	ts->remind_ticks = TIME_QUANT;

	return i;
}

void sys_kill(int pid)
{
	/*Lab4 TODO: Died task recycle, just set task state as TASK_STOP and sched_yield() */
	tasks[pid].state = TASK_STOP;
	sched_yield();

}

int sys_fork()
{
	int pid = -1;
	int offset = 0;

	/* Initial task space */
	pid = task_create();

	if (pid < 0 )
		return -1;
	
	if ((uint32_t)cur_task != NULL)
	{
		/* Lab4 TODO: Copy parent's tf to new task's tf */
		tasks[pid].tf = cur_task->tf;

		/* Lab4 TODO: Copy parent's usr_stack to new task's usr_stack and reset the new task's esp pointer. */
		memcpy(tasks[pid].usr_stack, cur_task->usr_stack, USR_STACK_SIZE);
		offset = (char *)cur_task->tf.tf_esp - cur_task->usr_stack;
		tasks[pid].tf.tf_esp = tasks[pid].usr_stack + offset;

		/* Lab4 TODO: Set system call's return value, parent return 0 and child return pid. */
		tasks[pid].tf.tf_regs.reg_eax = pid;
		return 0;
	}

	return 0;
	
	
}
void task_init()
{
	int i;

	/* Initial task sturcture */
	for (i = 0; i < NR_TASKS; i++)
	{
		memset(&(tasks[i]), 0, sizeof(Task));
		tasks[i].state = TASK_FREE;

	}
	// Setup a TSS so that we get the right stack
	// when we trap to the kernel.
	memset(&(tss), 0, sizeof(tss));
	tss.ts_esp0 = kern_stack + KERN_STACK_SIZE;
	tss.ts_ss0 = GD_KD;

	// fs and gs stay in user data segment
	tss.ts_fs = GD_UD | 0x03;
	tss.ts_gs = GD_UD | 0x03;

	/* Setup TSS in GDT */
	gdt[GD_TSS0 >> 3] = SEG16(STS_T32A, (uint32_t)(&tss), sizeof(struct tss_struct), 0);
	gdt[GD_TSS0 >> 3].sd_s = 0;

	/* Setup first task */
	i = task_create();
	cur_task = &(tasks[i]);
	
	/* Load GDT&LDT */
	lgdt(&gdt_pd);


	lldt(0);

	// Load the TSS selector 
	ltr(GD_TSS0);

	cur_task->state = TASK_RUNNING;
	
}



