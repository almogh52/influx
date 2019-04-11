bits 32

extern boot_main, sse_enable

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
    mov eax, pml4t
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

;   Load the new 64-bit GDT
    lgdt [gdt64.ptr]
    jmp 0x8:mode64

bits 64
mode64:
;   Reload segments with the new data segment
    mov cx, 0x10
    mov ss, cx
    mov ds, cx
    mov es, cx
    mov fs, cx
    mov gs, cx

;   Enable SSE
    call sse_enable

    movaps xmm0, xmm1

;   Call the bootstrap main function
    call boot_main

section .data
align 0x1000
pml4t:
;   Set the first entry to point at our first pdp that will identity map the first 4MB
;   The last 2 bits are for present flag and read/write flag
    dq pdpt + 11b

;   Set null for the rest of the entries
    times 510 dq 0

;   Set the last entry to point at our first pdp that will create as a higher half for the kernel
    dq pdpt + 11b

pdpt:
;   Identity map for the first 1GB
    dq pdt + 11b

;    Set null for the rest of the entries
    times 511 dq 0

pdt:
;   Identity map for the first 4MB
    dq 0 + 10000011b
    dq 0x200000 + 10000011b

;   Set null for the rest of the entries
    times 510 dq 0

gdt64:
align 16
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
    dq gdt64