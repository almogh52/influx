bits 32

HIGHER_HALF_OFFSET equ 0xFFFFFF7F80000000

extern boot_main, sse_enable, pml4t

global long_mode
section .text
long_mode:
;   Turn off paging since GRUB enables it, (it's the last bit of control register 0)
    mov eax, cr0
    and eax, 0x7FFFFFFF
    mov cr0, eax

;   Enable PAE using the fifth bit in the control register 4
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

;   Set the physical address of the PML4 table in cr3
    mov eax, pml4t - HIGHER_HALF_OFFSET
    mov cr3, eax

;   Read the EFER MSR
    mov ecx, 0xC0000080
    rdmsr

;   Enable long mode (bit no. 8) - Now we're in compatibility mode
    or eax, 1 << 8
    wrmsr

;   Re-enable paging with the new 64-bit paging structure
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

;   Get the multiboot parameters from the stack
    pop edi
    pop esi

;   Load the new 64-bit GDT
    lgdt [gdt64.ptr - HIGHER_HALF_OFFSET]
    jmp 0x8:mode64 - HIGHER_HALF_OFFSET

bits 64
mode64:
;   Jump to higher half
    mov rax, higher_half
    jmp rax

higher_half:
;   Reload segments with the new data segment
    mov cx, 0x10
    mov ss, cx
    mov ds, cx
    mov es, cx
    mov fs, cx
    mov gs, cx

;   Update rsp to higher half memory space
    mov rax, HIGHER_HALF_OFFSET
    add rsp, rax

;   Save multiboot parameters
    push rsi
    push rdi

;   Reload the 64-bit GDT
    lgdt [gdt64.higher_ptr - HIGHER_HALF_OFFSET]
    jmp far [higher_half_jump_ptr - HIGHER_HALF_OFFSET]

higher_half_continue:
;   Remove identity mapping in the first 4MiB and invalidate it's page
    mov qword [pml4t - HIGHER_HALF_OFFSET], 0

;   Invalidate all TLB
    mov rax, cr3
    mov cr3, rax

;   Enable SSE
    call sse_enable

;   Get multiboot parameters
    pop rdi
    pop rsi

;   Call the bootstrap main function
    call boot_main

;   Disable interrupts
    cli

panic:
    hlt
    jmp $

section .data
align 0x1000
gdt64:
align 64
.null_descriptor:
    dq 0

.seg_code:
    dw 0x0000    ; Segment limit (bit 0 - 15)
    dw 0x0000    ; Segment base  (bit 0 - 15)
    db 0x0000    ; Segment base  (bit 16 - 23)
    db 10011010b ; Fields
    db 10101111b ; Fields + Segment limit (bit 16 - 19)
    db 0x00      ; Segment base (bit 24 - 31)

.seg_data:
    dw 0x0000    ; Segment limit (bit 0 - 15)
    dw 0x0000    ; Segment base  (bit 0 - 15)
    db 0x0000    ; Segment base  (bit 16 - 23)
    db 10010010b ; Fields
    db 00000000b ; Fields + Segment limit (bit 16 - 19)
    db 0x00      ; Segment base (bit 24 - 31)
.ptr:
    dw $ - gdt64 - 1
    dq gdt64 - HIGHER_HALF_OFFSET
.higher_ptr:
    dw $ - gdt64 - 1
    dq gdt64

higher_half_jump_ptr:
    dq higher_half_continue
    dw 0x8