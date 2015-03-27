// Simple implementation of cprintf console output for the kernel,
// based on printfmt() and the kernel console's cputchar().
#include <inc/types.h>
#include <inc/stdio.h>

static void
putch(int ch, int *cnt)
{
	k_putch(ch); // in kernel/screen.c
	*cnt++;
}

int
vprintk(const char *fmt, va_list ap)
{
	int cnt = 0;

	vprintfmt((void*)putch, &cnt, fmt, ap);
	return cnt;
}

int
printk(const char *fmt, ...)
{
	va_list ap;
	int cnt;

	va_start(ap, fmt);
	cnt = vprintk(fmt, ap);
	va_end(ap);

	return cnt;
}

