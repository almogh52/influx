%include "macros.s"

%define THREAD_CONTEXT_OFFSET 16
%define PROCESS_CR3_OFFSET 18

section .text
global switch_task, jump_to_ring_3

;   void switch_task(thread *current_task, thread *new_task, process *new_task_process)

;   rdi = current task
;   rsi = new task
;   rdx = new task process
switch_task:
;   Save current task's context
    save_context

;   Save the context in the TCB
    mov [rdi + THREAD_CONTEXT_OFFSET], rsp

;   Load new task' CR3 if differnet from the current one
    mov rdi, rdx ; Old task no longer needed
    mov rbx, [rdi + PROCESS_CR3_OFFSET]
    mov rax, cr3
    cmp rax, rbx
    je .cr3_loaded

    mov rax, rbx
    mov cr3, rax

.cr3_loaded:
;   Load new task's stack pointer
    mov rsp, [rsi + THREAD_CONTEXT_OFFSET]

;   Restore new task's registers
    restore_context

    ret

;   void jump_to_ring_3(uint64_t ring_3_function_address, void *user_stack, void *param)

;   rdi = the address of the ring 3 function
;   rsi = the pointer to the user stack
;   rdx = the parameter to pass to the ring 3 function
jump_to_ring_3:
;   Set ring 3 data segment
    mov ax, 0x20 + 11b ; Ring 3 data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

;   Prepare interrupt stack frame
    push 0x20 + 11b ; Ring 3 data segment as stack segment
    push rsi ; The userland stack pointer
    pushf
    push 0x18 + 11b ; Ring 3 code segment
    push rdi

;   Set parameter for function
    mov rdi, rdx

;   Clear registers
    xor rax, rax
    xor rbx, rbx
    xor rcx, rcx
    xor rdx, rdx
    xor rsi, rsi
    xor rbp, rbp
    xor r8, r8
    xor r9, r9
    xor r10, r10
    xor r11, r11
    xor r12, r12
    xor r13, r13
    xor r14, r14
    xor r15, r15

;   Jump to ring 3 function
    iretq