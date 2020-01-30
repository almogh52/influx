#include <kernel/panic.h>

void kpanic()
{
	// TODO: Add print

	// Disable interrupts and halt
	__asm__ __volatile__("cli; hlt;"); 
}