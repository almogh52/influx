MULTIBOOT2_HEADER_MAGIC equ 0xE85250D6
GRUB_MULTIBOOT_ARCHITECTURE_I386 equ 0
MULTIBOOT_HEADER_LENGTH equ 0x100000000 - (MULTIBOOT2_HEADER_MAGIC + GRUB_MULTIBOOT_ARCHITECTURE_I386 + (multiboot_header_end - multiboot_header_start))

HIGHER_HALF_OFFSET equ 0xFFFFFF7F80000000

bits 32

align 64
section .multiboot_header
multiboot_header_start:
    dd MULTIBOOT2_HEADER_MAGIC
    dd GRUB_MULTIBOOT_ARCHITECTURE_I386
    dd multiboot_header_end - multiboot_header_start
    dd MULTIBOOT_HEADER_LENGTH

;   A closing tag
    dw 0
    dw 0
    dd 8
multiboot_header_end:

extern long_mode

section .text
global _start
_start:
;   Disable interrupts
	cli

;   Load the GDT
    lgdt [gdt_ptr - HIGHER_HALF_OFFSET]

;   Jump to the new code segment
    jmp 0x8:new_gdt - HIGHER_HALF_OFFSET

new_gdt:
;   Reload segments with the new data segment
    mov cx, 0x10
    mov ss, cx
    mov ds, cx
    mov es, cx
    mov fs, cx
    mov gs, cx

;   Set the pointer to the top of the stack
    mov esp, stack_top - HIGHER_HALF_OFFSET
    mov ebp, esp

;   Save multiboot parameters and prepare to send them to the kernel
    push ebx
    push eax

;   TODO: Make sure that the processer is a 64-bit compatible
;   Even though it's not required since the GRUB bootloader is 64-bit

;   Switch to long mode
    jmp long_mode

section .data
align 32
gdt_start:
null_descriptor:
    dq 0

code:
    dw 0xFFFF    ; 0-15 Bits of the segment limit
    dw 0x0000    ; 0-15 Bits of the base address
    db 0x0000    ; 16-23 Bits of the base address
    db 10011010b ; Access byte for the code segment
    db 11001111b ; Flags and 16-19 Bits of the base address
    db 0x0000    ; 24-31 Bits of the base address

data:
    dw 0xFFFF    ; 0-15 Bits of the segment limit
    dw 0x0000    ; 0-15 Bits of the base address
    db 0x0000    ; 16-23 Bits of the base address
    db 10010010b ; Access byte for the data segment
    db 11001111b ; Flags and 16-19 Bits of the base address
    db 0x0000    ; 24-31 Bits of the base address
gdt_end:

align 32
gdt_ptr:
    dw (gdt_end - gdt_start - 1)
    dd gdt_start - HIGHER_HALF_OFFSET

; Creating the stack by the System V ABI Spec
section .bss
align 16
global stack_bottom, stack_top
stack_bottom:
    resb 16384
stack_top:
