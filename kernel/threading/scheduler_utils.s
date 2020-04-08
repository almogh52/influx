%include "macros.s"

%define THREAD_CONTEXT_OFFSET 16
%define PROCESS_CR3_OFFSET 18

section .text
global switch_task, get_cr3

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

;   uint64_t get_cr3()
get_cr3:
    mov rax, cr3
    ret