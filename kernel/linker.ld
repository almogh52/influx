/* Begin putting sections at 1 MiB, a conventional place for kernels to be
	   loaded at by the bootloader. */
KERNEL_LMA = 0x0000000000100000;
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