/*
 * hideproc.c
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/list.h>			/* for list_for_each() */
#include <linux/sched.h>		/* for task_struct */
#include <asm/param.h>			/* for HZ */

#define DRIVER_AUTHOR "Zespre Schmidt <starbops@gmail.com>"
#define DRIVER_DESC "Hide loop process"

static int backup_pid = 0;
static struct task_struct *backup_task = NULL;

int move_pid(void)
{
	struct task_struct *p = NULL;
	for_each_process(p) {
		if(!strcmp(p->comm, "loop")) {
			printk("[%d] %s found\n", p->pid, p->comm);
			backup_task = p;
			backup_pid = p->pid;
			p->pid = 5566;
			break;
		}
	}
	set_current_state(TASK_INTERRUPTIBLE);
	schedule_timeout(10*HZ);
	return 0;
}

int restore_pid(struct task_struct *p, int pid)
{
	for_each_process(p) {
		if(!strcmp(p->comm, "loop")) {
			printk("[%d] %s found\n", p->pid, p->comm);
			p->pid = backup_pid;
			break;
		}
	}
	set_current_state(TASK_INTERRUPTIBLE);
	schedule_timeout(10*HZ);
	return 0;
}

static int __init init_hideproc(void)
{
	printk(KERN_DEBUG "[hideproc] Up\n");
	move_pid();
	return 0;
}

static void __exit cleanup_hideproc(void)
{
	restore_pid(backup_task, backup_pid);
	printk(KERN_DEBUG "[hideproc] Bye\n");
}

module_init(init_hideproc);
module_exit(cleanup_hideproc);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);

