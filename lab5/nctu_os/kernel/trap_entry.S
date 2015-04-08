#include <inc/mmu.h>
#include <inc/trap.h>

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps


.text

/* ISRs */
TRAPHANDLER_NOEC(Default_ISR, T_DEFAULT)

/* Lab3 TODO: Setup timer and keyboard's ISR entry point */
TRAPHANDLER_NOEC(Timer_ISR, IRQ_OFFSET)
TRAPHANDLER_NOEC(Keyboard_ISR, IRQ_OFFSET+1)

/* Lab4 TODO: Setup system call's ISR entry point */
TRAPHANDLER_NOEC(Syscall_ISR, T_SYSCALL)


.globl default_trap_handler;
_alltraps:
	/* Lab3 TODO: Push the registers into stack( fill the Trapframe structure )
	 * You can reference the http://www.osdever.net/bkerndev/Docs/isrs.htm
	 * After stack parpared, just "call default_trap_handler".
	 */
	# Build trap frame
	pushl %ds
	pushl %es
	pushal

	# Set up data segments
	movw $GD_KD, %ax
	movw %ax, %ds
	movw %ax, %es

	# disable FL_IF in %eflags, i.e. disable interrupts
	pushfl
	popl %eax
	movl $FL_IF, %ebx
	notl %ebx
	andl %ebx, %eax
	pushl %eax
	popfl

	# Call default_trap_handler(tf), where tf=%esp
	pushl %esp
	call default_trap_handler
	popl %esp

	# Cleanup pushes and ret
	popal
	popl %es
	popl %ds
	add $8, %esp # Cleans up the pushed error code and pushed ISR number
	iret # pops 5 things at once: CS, EIP, EFLAGS, SS, and ESP!

