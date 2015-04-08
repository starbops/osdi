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
	i = (cur_task->task_id + 1) % NR_TASKS;
	for (; next_i < NR_TASKS; next_i++)
	{
		if (tasks[i].state == TASK_RUNNABLE)
		{
			if (cur_task->state == TASK_RUNNING)
			{
				cur_task->state = TASK_RUNNABLE;
				cur_task->remind_ticks = TIME_QUANT;
				cur_task = &tasks[i];
				tasks[i].state = TASK_RUNNING;
				break;
			} else if (cur_task->state == TASK_SLEEP)
			{
				cur_task = &tasks[i];
				tasks[i].state = TASK_RUNNING;
				break;
			} else if (cur_task->state == TASK_STOP) {
				cur_task = &tasks[i];
				tasks[i].state = TASK_RUNNING;
				break;
			}
		} else if ( tasks[i].state == TASK_RUNNING)
		{
			cur_task = &tasks[i];
			tasks[i].remind_ticks = TIME_QUANT;
		}
		i = (i + 1) % NR_TASKS;
	}

	env_pop_tf(&cur_task->tf);
}
