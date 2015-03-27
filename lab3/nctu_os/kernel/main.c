#include <inc/stdio.h>
#include <inc/kbd.h>
#include <inc/shell.h>
#include <inc/timer.h>
#include <inc/x86.h>
#include <kernel/trap.h>
#include <kernel/picirq.h>

extern void init_video(void);
void kernel_main(void)
{
	init_video();

	pic_init();
	kbd_init();
	timer_init();
	trap_init();

	/* Enable interrupt */
	__asm __volatile("sti");

	shell();
}
