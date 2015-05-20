/*
 * mmkiller.c
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>		/* for get_mm_rss() */

#define DRIVER_AUTHOR "Zespre Schmidt <starbops@gmail.com>"
#define DRIVER_DESC "Memory killer"

#define MAX_SIZE 256

int mmkiller_pid;

asmlinkage long (*sys_tkill)(int pid, int sig);

static int mmkiller(void *data)
{
	int pos = 0, min_pos, min_rss, tmp_rss, i, j;
	struct task_struct *p;
	struct task_struct *task_ptr[MAX_SIZE];
	/*unsigned long *sys_call_table = (unsigned long *)0xc145a138;*/
	unsigned long *sys_call_table = (unsigned long *)0xc145a140;
	sys_tkill = (void *)sys_call_table[__NR_tkill];

	daemonize("mmkiller");

	/*
	 * record all processes excluding kernel threads
	 */
	for_each_process(p) {
		if(p->mm && p->real_parent->pid != 2) {
			task_ptr[pos++] = p;
		}
	}

	/*
	 * selection sort by resident memory
	 */
	for(i = 0; i < pos; i++) {
		min_rss = get_mm_rss(task_ptr[i]->mm);
		min_pos = i;
		for(j = i+1; j < pos; j++) {
			tmp_rss = get_mm_rss(task_ptr[j]->mm);
			if(tmp_rss < min_rss) {
				min_rss = tmp_rss;
				min_pos = j;
			}
		}
		p = task_ptr[min_pos];
		task_ptr[min_pos] = task_ptr[i];
		task_ptr[i] = p;
	}

	/*
	 * dump the list with ascending order
	 */
	for(i = 0; i < pos; i++) {
		p = task_ptr[i];
		printk(KERN_INFO
				"[%-5d]| %-20s| %-8lu\n",
				p->pid, p->comm, get_mm_rss(p->mm));
	}

	/*
	 * kill the king
	 */
	p = task_ptr[pos-1];
	printk(KERN_INFO
			"[%d] %s was killed.\n",
			p->pid, p->comm);
	sys_tkill(p->pid, SIGKILL);

	return 0;
}

static int __init init_mmkiller(void)
{
	printk(KERN_INFO "[mmkiller] Up\n");
	mmkiller_pid = kernel_thread(mmkiller , NULL , CLONE_KERNEL);
	if(mmkiller_pid > 0) {
		printk("Created mmkiller PID = %d\n", mmkiller_pid);
	} else {
		printk("Failed to create mmkiller\n");
	}
	return 0;
}

static void __exit cleanup_mmkiller(void)
{
	printk(KERN_INFO "[mmkiller] Bye\n");
}

module_init(init_mmkiller);
module_exit(cleanup_mmkiller);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);

