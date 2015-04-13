#ifndef LOCK_H
#define LOCK_H
static inline void lock()
{
	__asm __volatile("cli");
}
static inline void unlock()
{
	__asm __volatile("sti");
}
#endif
