
GCC_LIB := $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

KERN_LDFLAGS = -T kernel/kern.ld

KERN_SRCFILES := kernel/entry.S \
		kernel/main.c \
		kernel/picirq.c \
		kernel/kbd.c \
		kernel/screen.c \
		kernel/printf.c \
		lib/printfmt.c \
		lib/string.c

KERN_OBJS = kernel/entry.o \
	kernel/main.o \
	kernel/picirq.o \
	kernel/kbd.o \
	kernel/screen.o \
	kernel/trap.o \
	kernel/trap_entry.o \
	kernel/printf.o \
	kernel/shell.o \
	kernel/timer.o \
	kernel/task.o \
	kernel/syscall.o \
	kernel/assert.o \
	kernel/sched.o \
	lib/printf.o \
	lib/printfmt.o \
	lib/readline.o \
	lib/string.o \
	lib/console.o \
	user.o

kernel/%.o: kernel/%.c
	$(CC) $(CFLAGS) -O0 -c -o $@ $<

kernel/%.o: kernel/%.S
	$(CC) $(CFLAGS) -c -o $@ $<

kernel/system: $(KERN_OBJS)
	@echo + ld kernel/system
	$(LD) $(KERN_LDFLAGS) $(KERN_OBJS) $(GCC_LIB) -o $@
	$(OBJDUMP) -S $@ > $@.asm
	$(NM) -n $@ > $@.sym
