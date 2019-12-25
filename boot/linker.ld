/* The bootloader will look at this image and start execution at the symbol
   designated as the entry point. */
ENTRY(_start)
 
/* Tell where the various sections of the object files will be put in the final
   bootstrap image. */
SECTIONS
{
	/* Begin putting sections at 512 KiB, because the kernel is loaded into 1MiB. */
	. = 512K;
 
	/* First put the multiboot header, as it is required to be put very early
	   early in the image or the bootloader won't recognize the file format. */
	.boot :
	{
		*(.multiboot_header)
	}

	/* Code. */
	.text : ALIGN(0x1000)
	{
        *(.start)
        *(.text)
        *(.text.*)
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