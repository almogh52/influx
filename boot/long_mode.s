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

;   Enable long mode (bit no. 8) and no-execute page-protection (bit no. 11) - Now we're in compatibility mode
    or eax, (1 << 8) | (1 << 11) 
    wrmsr

;   Re-enable paging and write protect with the new 64-bit paging structure
    mov eax, cr0
    or eax, (1 << 31) | (1 << 16) 
    mov cr0, eax

;   Get the multiboot parameters from the stack
    pop edi
    pop esi

;   Load the new 64-bit GDT
    mov eax, gdt64.ptr - HIGHER_HALF_OFFSET
    lgdt [eax]
    jmp (gdt64.seg_ring_0_code - gdt64):mode64 - HIGHER_HALF_OFFSET

bits 64
mode64:
;   Jump to higher half
    mov rax, higher_half
    jmp rax

higher_half:
;   Reload segments with the new data segment
    mov cx, (gdt64.seg_ring_0_data - gdt64)
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

;   Calculate TSS base address for TSS descriptor in GDT
    mov rax, tss64
    mov rdi, gdt64.seg_tss
    add rdi, 2
    mov word [rdi], ax
    shr rax, 16
    add rdi, 2
    mov byte [rdi], al
    mov al, ah
    add rdi, 3
    mov byte [rdi], al
    shr rax, 16
    inc rdi
    mov dword [rdi], eax

;   Reload the 64-bit GDT
    mov rax, gdt64.higher_ptr - HIGHER_HALF_OFFSET
    lgdt [rax]

;   Jump to reload the GDT
    mov rax, higher_half_jump_ptr - HIGHER_HALF_OFFSET
    jmp far [rax]

higher_half_continue:
;   Remove identity mapping in the first 4MiB and invalidate it's page
    mov rax, pml4t - HIGHER_HALF_OFFSET
    mov qword [rax], 0

;   Invalidate all TLB
    mov rax, cr3
    mov cr3, rax

;   Load TSS with RPL of ring 3
    mov ax, (gdt64.seg_tss - gdt64) + 11b
    ltr ax

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
global gdt64, gdt64_end, tss64, tss64_end
gdt64:
align 64
.null_descriptor:
    dq 0

.seg_ring_0_code:
    dw 0x0000    ; Segment limit (bit 0 - 15)
    dw 0x0000    ; Segment base  (bit 0 - 15)
    db 0x00      ; Segment base  (bit 16 - 23)
    db 10011010b ; Access Byte
    db 10101111b ; Flags + Segment limit (bit 16 - 19)
    db 0x00      ; Segment base (bit 24 - 31)

.seg_ring_0_data:
    dw 0x0000    ; Segment limit (bit 0 - 15)
    dw 0x0000    ; Segment base  (bit 0 - 15)
    db 0x00      ; Segment base  (bit 16 - 23)
    db 10010010b ; Access Byte
    db 00000000b ; Flags + Segment limit (bit 16 - 19)
    db 0x00      ; Segment base (bit 24 - 31)

.seg_ring_3_code:
    dw 0x0000    ; Segment limit (bit 0 - 15)
    dw 0x0000    ; Segment base  (bit 0 - 15)
    db 0x00    ; Segment base  (bit 16 - 23)
    db 11111010b ; Access Byte
    db 10101111b ; Flags + Segment limit (bit 16 - 19)
    db 0x00      ; Segment base (bit 24 - 31)

.seg_ring_3_data:
    dw 0x0000    ; Segment limit (bit 0 - 15)
    dw 0x0000    ; Segment base  (bit 0 - 15)
    db 0x00      ; Segment base  (bit 16 - 23)
    db 11110010b ; Access Byte
    db 00000000b ; Flags + Segment limit (bit 16 - 19)
    db 0x00      ; Segment base (bit 24 - 31)

.seg_tss:
    dw tss64_end - tss64 ; Segment limit (bit 0 - 15)
    dw 0x0000            ; Segment base  (bit 0 - 15)
    db 0x00              ; Segment base  (bit 16 - 23)
    db 11101001b         ; Preset, DPL(2 bits), 0, Type(4 bits)
    db 00000000b         ; Flags + Segment limit (bit 16 - 19)
    db 0x00              ; Segment base (bit 24 - 31)
    dd 0x00000000        ; Segment base (bit 32 - 63)
    dd 0x00000000        ; Reserved

.ptr:
    dw $ - gdt64 - 1
    dq gdt64 - HIGHER_HALF_OFFSET
.higher_ptr:
    dw $ - gdt64 - 1
    dq gdt64
gdt64_end:

align 128
tss64:
    times 0xD dq 0
tss64_end:

higher_half_jump_ptr:
    dq higher_half_continue
    dw (gdt64.seg_ring_0_code - gdt64)