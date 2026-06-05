; kernel/entry.asm
BITS 64
global _start
extern kernel_main

_start:
    ; Apelăm funcția principală din C
    call kernel_main

    ; În caz că funcția C returnează vreodată, blocăm procesorul
.halt:
    hlt
    jmp .halt