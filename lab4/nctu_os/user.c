#include <inc/stdio.h>
#include <inc/syscall.h>

int k = 0;
void task_job()
{
	int pid = 0;
	int i = 0;

	pid = getpid();
	for (i = 0; i < 10; i++)
	//while(1)
	{
		cprintf("Im %d, local=%d global=%d\n", pid, i, k++);
		sleep(100);
	}
}
