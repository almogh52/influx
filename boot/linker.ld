/* Begin putting sections at 2MiB, because the bootstrap is sitting in 1MiB */
HIGHER_HALF   = 0xffffff7f80000000;
BOOTSTRAP_LMA = 1M;
BOOTSTRAP_VMA = HIGHER_HALF + BOOTSTRAP_LMA;

/* The bootloader will look at this image and start execution at the symbol
   designated as the entry point. */
ENTRY(_start)
 
/* Tell where the various sections of the object files will be put in the final
   bootstrap image. */
SECTIONS
{
	/* Begin putting sections at 1MiB, because it's a convention to load operation system at 1MiB. */
	. = BOOTSTRAP_VMA;
 
	/* First put the multiboot header, as it is required to be put very early
	   early in the image or the bootloader won't recognize the file format. */
	.boot : AT(ADDR(.boot) - HIGHER_HALF)
	{
		*(.multiboot_header)
	}

	/* Code. */
	.text ALIGN(0x1000) : AT(ADDR(.text) - HIGHER_HALF)
	{
        *(.start)
        *(.text)
        *(.text.*)
	}
 
	/* Read-only data. */
	.rodata ALIGN(0x1000) : AT(ADDR(.rodata) - HIGHER_HALF)
	{
		*(.rodata)
	}
 
	/* Read-write data (initialized) */
	.data ALIGN(0x1000) : AT(ADDR(.data) - HIGHER_HALF)
	{
		*(.data)
	}
 
	/* Read-write data (uninitialized) and stack */
	.bss ALIGN(0x1000) : AT(ADDR(.bss) - HIGHER_HALF)
	{
		*(COMMON)
		*(.bss)
	}
 
	/* The compiler may produce other sections, by default it will put them in
	   a segment with the same name. Simply add stuff here as needed. */
}