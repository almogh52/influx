bits 32

PML4T_ADDR equ 0x1000
PDPT_ADDR equ 0x2000
PDT_ADDR equ 0x3000

global long_mode

section .text
long_mode:
;   Turn off paging since GRUB enables it, (it's the last bit of control register 0)
    mov eax, cr0
    and eax, 01111111111111111111111111111111b
    mov cr0, eax

;   Copy the paging structures
    mov ecx, 0x3000 / 4
    mov esi, pml4t
    mov edi, PML4T_ADDR
    rep movsd

;   Set the physical address of the PML4 table in cr3
    mov eax, PML4T_ADDR
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

    ret

section .data
pml4t:
;   Set the first entry to point at our first pdp that will identity map the first 4MB
;   The last 2 bits are for present flag and read/write flag
    dq PDPT_ADDR << 12 + 11b

;   Set null for the rest of the entries
    times 510 dq 0

;   Set the last entry to point at our first pdp that will create as a higher half for the kernel
    dq PDPT_ADDR << 12 + 11b

pdpt:
;   Identity map for the first 1GB
    dq PDT_ADDR << 12 + 11b

;    Set null for the rest of the entries
    times 511 dq 0

pdt:
;   Identity map for the first 4MB
    dq 0 << 21 + 1000011b
    dq 0x200000 << 21 + 1000011b

;   Set null for the rest of the entries
    times 510 dq 0