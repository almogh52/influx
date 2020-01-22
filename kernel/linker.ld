/* Begin putting sections at 2MiB, because the bootstrap is sitting in 1MiB */
KERNEL_LMA = 2M;
KERNEL_VMA = 0xffffffff80000000 + KERNEL_LMA;

/* The bootstrap will look at this image and start execution at the symbol
   designated as the entry point. */
ENTRY(_start)
 
/* Tell where the various sections of the object files will be put in the final
   kernel image. */
SECTIONS
{
	. = KERNEL_VMA;

	/* Code. */
	.text : ALIGN(0x1000)
	{
        *(.start)
        *(.text)
        *(.text.*)
		*(.init*)
		*(.fini*)
	}
 
	/* Read-only data. */
	.rodata : ALIGN(0x1000)
	{
		*(.rodata)
	}
 
	/* Read-write data (initialized) */
	.data : ALIGN(0x1000)
	{
		*(.data)
	}
 
	/* Read-write data (uninitialized) and stack */
	.bss : ALIGN(0x1000)
	{
		*(COMMON)
		*(.bss)
	}
 
	/* The compiler may produce other sections, by default it will put them in
	   a segment with the same name. Simply add stuff here as needed. */
}