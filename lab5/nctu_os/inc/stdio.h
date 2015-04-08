#ifndef JOS_INC_STDIO_H
#define JOS_INC_STDIO_H

#include <inc/stdarg.h>

#ifndef NULL
#define NULL	((void *) 0)
#endif /* !NULL */

//lib/kbd.c
int k_getc(void);

//lib/screen.c
void	k_puts(unsigned char *text);
void	k_putch(unsigned char c);

//kernel/printf.c
void	k_puts(unsigned char *text);
int	printk(const char *fmt, ...);

// lib/printfmt.c
void	printfmt(void (*putch)(int, void*), void *putdat, const char *fmt, ...);
void	vprintfmt(void (*putch)(int, void*), void *putdat, const char *fmt, va_list);
int	snprintf(char *str, int size, const char *fmt, ...);
int	vsnprintf(char *str, int size, const char *fmt, va_list);

// lib/printf.c
int	cprintf(const char *fmt, ...);
int	vcprintf(const char *fmt, va_list);

// lib/readline.c
char *readline(const char *prompt);

// lib/console.c, use it in user space 
void cputchar(int ch);
int getchar(void);

#endif /* !JOS_INC_STDIO_H */
