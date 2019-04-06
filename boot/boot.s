MULTIBOOT2_HEADER_MAGIC equ 0xE85250D6
GRUB_MULTIBOOT_ARCHITECTURE_I386 equ 0
MULTIBOOT_HEADER_LENGTH equ 0x100000000 - (MULTIBOOT2_HEADER_MAGIC + GRUB_MULTIBOOT_ARCHITECTURE_I386 + (multiboot_header_end - multiboot_header_start))

section .multiboot_header
multiboot_header_start:
    dd MULTIBOOT2_HEADER_MAGIC
    dd GRUB_MULTIBOOT_ARCHITECTURE_I386
    dd multiboot_header_end - multiboot_header_start
    dd MULTIBOOT_HEADER_LENGTH
    dw 0
    dw 0
    dd 8
multiboot_header_end:

; Creating the stack by the System V ABI Spec
section .bss
align 16
stack_bottom:
resb 16384
stack_top:

section .text
global _start

_start:
;   Set the pointer to the top of the stack
    mov esp, stack_top
    mov dword [0xb8000], 0x2f4b2f4f

	cli
idk:	
    hlt
	jmp idk
