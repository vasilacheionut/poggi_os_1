; kernel/task_switch.asm
[bits 64]
global switch_context

; switch_context(unsigned long *old_rsp, unsigned long new_rsp)
; În convenția System V AMD64:
; RDI = adresa unde salvăm vechiul RSP (pointer către old_rsp)
; RSI = valoarea noului RSP (new_rsp)
switch_context:
    ; 1. Salvăm contextul curent (registrele callee-saved și cele de bază)
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; 2. Salvăm RSP-ul curent în structura vechiului task
    mov [rdi], rsp

    ; 3. Încărcăm RSP-ul noului task
    mov rsp, rsi

    ; 4. Restaurăm contextul noului task (în ordine inversă push-urilor)
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    ; 5. Sărim la adresa memorată în vârful noii stive (unde a rămas noul task)
    ret