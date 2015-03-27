#include <inc/stdio.h>
#include <inc/error.h>

#define BUFLEN 1024
static char buf[BUFLEN];

char *readline(const char *prompt)
{
	int i, c, echoing;

	if (prompt != NULL)
		cprintf("%s", prompt);

	i = 0;
	while (1) {
		c = getc();
		if (c < 0) {
			cprintf("read error: %e\n", c);
			return NULL;
		} else if ((c == '\b' || c == '\x7f') && i > 0) {
			putch('\b');
			i--;
		} else if (c >= ' ' && i < BUFLEN-1) {
			putch(c);
			buf[i++] = c;
		} else if (c == '\n' || c == '\r') {
			putch('\n');
			buf[i] = 0;
			return buf;
		}
	}
}

