#include <inc/mmu.h>

.globl _start

.text
_start:
	movw	$0x1234,0x472			# warm boot

	# Setup kernel stack
	movl $0, %ebp
	movl $(bootstacktop), %esp

	call kernel_main
die:
	jmp die

.bss
	# There is kernel initial stack
	.globl		bootstack
bootstack:
	.space		KSTKSIZE
	.globl		bootstacktop   
bootstacktop:

