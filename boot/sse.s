bits 64

global sse_enable
section .text
sse_enable:
;   Check if the CPU supports SSE, if it doesn't "panic"
    mov eax, 0x1
    cpuid
    test edx, 1 << 25
    jz sse_error

;   Enable SSE
    mov rax, cr0
    and ax, 0xFFFB ; Clear Co-Processor emulation flag (CR0.EM)
    or  ax, 1 << 1 ; Set Co-Processor monitor flag (CR0.MP)
    mov cr0, rax
    mov rax, cr4
    or rax, 1 << 10 | 1 << 9 ; Enable CR4.OSFXSR and CR4.OSXMMEXCPT at the same time to enable SSE
    mov cr4, rax

sse_finish:
    ret

sse_error:
;   Infinite loop
    cli
    hlt
    jmp sse_error