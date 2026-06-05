[org 0x7c00]
BITS 16

KERNEL_OFFSET equ 0x9000

start:
    ; Resetăm complet registrele de segment
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00

    ; Salvăm numărul discului de boot oferit de BIOS în DL
    mov [BOOT_DRIVE], dl

    ; --- CITIREA DISCULUI CU RESET ȘI RETRY ---
disk_load:
    mov bx, KERNEL_OFFSET   ; ES:BX = 0x0000:0x9000
    mov ah, 0x02            ; Funcția BIOS citire sectoare
    mov al, 50               ; <<< SCHIMBĂ AICI: Citim exact cele 50 sectoare de cod C!
    mov ch, 0               ; Cilindru 0
    mov dh, 0               ; Cap 0
    mov cl, 2               ; Sectorul 2 (imediat după bootloader)
    mov dl, [BOOT_DRIVE]    ; Discul de boot salvat
    int 0x13                ; Apel BIOS

    jnc disk_success        ; Dacă nu avem erori (Carry Flag = 0), mergem mai departe

    ; Dacă a eșuat, resetăm controller-ul de disc și încercăm din nou
    xor ah, ah
    mov dl, [BOOT_DRIVE]
    int 0x13
    jmp disk_load

disk_success:
    ; Pasul 2: Trecem în Protected Mode (32-bit)
    cli
    lgdt [gdt_32bit_descriptor]
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax
    jmp CODE_SEG_32:init_32bit

BOOT_DRIVE: db 0            ; Variabilă pentru stocarea id-ului de disc

; --- GDT 32-bit ---
gdt_32bit_start: 
    dd 0x0, 0x0
gdt_32bit_code:  
    dw 0xffff, 0x0 
    db 0x0, 10011010b, 11001111b, 0x0
gdt_32bit_data:  
    dw 0xffff, 0x0 
    db 0x0, 10010010b, 11001111b, 0x0
gdt_32bit_end:

gdt_32bit_descriptor: 
    dw gdt_32bit_end - gdt_32bit_start - 1 
    dd gdt_32bit_start

CODE_SEG_32 equ gdt_32bit_code - gdt_32bit_start
DATA_SEG_32 equ gdt_32bit_data - gdt_32bit_start

; --- MODUL PROTEJAT 32-BIT ---
BITS 32
init_32bit:
    mov ax, DATA_SEG_32 
    mov ds, ax 
    mov ss, ax 
    mov es, ax
    
    ; Configurare Paginare (Identity Mapping primii 2MB)
    mov edi, 0x1000 
    mov cr3, edi 
    xor eax, eax 
    mov ecx, 3072 
    rep stosd

    mov dword [0x1000], 0x2003 
    mov dword [0x2000], 0x3003 
    mov dword [0x3000], 0x0083

    ; Activare PAE
    mov eax, cr4 
    or eax, 1 << 5 
    mov cr4, eax 

    ; Activare Long Mode
    mov ecx, 0xc0000080 
    rdmsr 
    or eax, 1 << 8 
    wrmsr 

    ; Activare Paging
    mov eax, cr0 
    or eax, 1 << 31 
    mov cr0, eax 

    lgdt [gdt_64bit_descriptor]
    jmp CODE_SEG_64:init_64bit

; --- GDT 64-bit ---
gdt_64bit_start: 
    dd 0x0, 0x0
gdt_64bit_code:  
    dw 0x0, 0x0 
    db 0x0, 10011010b, 00100000b, 0x0
gdt_64bit_data:  
    dw 0x0, 0x0 
    db 0x0, 10010010b, 0x0, 0x0
gdt_64bit_end:

gdt_64bit_descriptor: 
    dw gdt_64bit_end - gdt_64bit_start - 1 
    dd gdt_64bit_start

CODE_SEG_64 equ gdt_64bit_code - gdt_64bit_start

; --- MODUL LONG MODE 64-BIT ---
BITS 64
init_64bit:
    ; Resetăm segmentele pentru Long Mode
    xor ax, ax 
    mov ds, ax 
    mov ss, ax 
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Inițializăm o stivă temporară stabilă în RAM la 0x90000
    mov rsp, 0x90000
    and rsp, 0xfffffffffffffff0  ; Mască pe biți care forțează alinierea pe 16 octeți cerută de C

    ; Sărim direct la adresa din RAM unde se află binarul nostru din C!
    mov rax, KERNEL_OFFSET
    jmp rax

; Semnătura MBR fix la 512 bytes
times 510 - ($ - $$) db 0
dw 0xaa55