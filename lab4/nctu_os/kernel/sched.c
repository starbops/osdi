#include <kernel/task.h>

static index = 0;
void sched_yield(void)
{

	extern Task tasks[];
	extern Task *cur_task;
	int i;
	int next_i = 0;

	/* Lab4 TODO: Implement a simple round-robin scheduling there 
	*  Hint: Choose a runnable task from tasks[] and use env_pop_tf() do the context-switch
	*/
	

}
