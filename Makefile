# Poggi OS

**Autor:** Vasilache Ionut  
**Arhitectură:** x86-64 (64 de biți)  
**Mediu:** Freestanding (bootabil pe hardware real sau în QEMU)

Poggi OS este un sistem de operare minimalist, dar extensibil, dezvoltat ca proiect personal de studiu. Include un bootloader propriu, un kernel cu multitasking cooperativ, un sistem de fișieres propriu (PoggiFS) și un shell interactiv cu mai multe comenzi.

---

## Cuprins

- [Caracteristici](#caracteristici)
- [Cerințe de compilare](#cerințe-de-compilare)
- [Compilare și rulare](#compilare-și-rulare)
- [Structura proiectului](#structura-proiectului)
- [Comenzi disponibile în shell](#comenzi-disponibile-în-shell)
- [Sistemul de fișiere PoggiFS](#sistemul-de-fișiere-poggifs)
- [Multitasking](#multitasking)
- [Depanare (debug)](#depanare-debug)
- [Plan de dezvoltare](#plan-de-dezvoltare)
- [Licență](#licență)

---

## Caracteristici

- **Bootloader** scris în NASM – încarcă kernel-ul de pe disc.
- **Kernel 64‑bit** compilat cu `gcc` în mod *freestanding*.
- **Drivere** pentru:
  - VGA (text mode)
  - Tastatură PS/2
  - Heap (alocare dinamică de memorie)
  - ATA (citire/scriere disc hard)
- **Sistem de fișiere propriu:** PoggiFS – structură simplă, bazată pe indecși în sectorul 55.
- **Shell interactiv** cu istoric și comenzi utile.
- **Multitasking cooperativ** – comutare manuală prin `yield`.
- **Suport pentru creare, citire, scriere și ștergere** de fișiere pe disc.
- **Construcție profesionistă** cu `make` – compilare incrementală, imagine disc generată automat.

---

## Cerințe de compilare

Pentru a construi și rula Poggi OS ai nevoie de:

- **Linux** (recomandat Ubuntu/Debian) sau WSL
- **gcc** (suport pentru `-m64 -ffreestanding`)
- **ld** (linker pentru x86_64)
- **nasm** (asamblator)
- **make** (GNU Make)
- **python3** (pentru instrumentul `poggi_mkfs.py`)
- **qemu-system-x86_64** (recomandat pentru testare)

Instalare pe Ubuntu/Debian:
```bash
sudo apt update
sudo apt install build-essential nasm qemu-system-x86 python3


Compilare și rulare
Clonare repository (dacă nu ai deja sursele):

bash
git clone https://github.com/vasilacheionut/poggi_os.git
cd poggi_os
Compilare completă (generează build/bin/os_image.bin):

bash
make clean
make
Rulare în QEMU:

bash
make run
Rulare cu consolă serială (pentru debugging):

bash
make run-debug
Curățare (șterge tot ce a fost generat):

bash
make clean
Structura proiectului
text
poggi_os/
├── boot/
│   ├── boot.asm          # Bootloader (sector 0)
│   └── boot.bin          # Generat la compilare
├── kernel/
│   ├── entry.asm         # Punct de intrare în kernel (assembly)
│   ├── task_switch.asm   # Comutare context (assembly)
│   ├── kernel.c          # Inițializare și bucla principală
│   ├── task.c            # Planificatorul de procese
│   ├── task.h
│   └── linker.ld         # Script de legare
├── drivers/
│   ├── vga.c / vga.h
│   ├── keyboard.c / keyboard.h
│   ├── heap.c / heap.h
│   └── ata.c / ata.h
├── include/              # Headere publice
│   ├── poggi_fs.h
│   ├── poggi_task.h
│   ├── poggi_mem.h
│   └── ...
├── fs_root/              # Fișierele care vor fi incluse în imaginea disc
│   ├── help.txt
│   ├── nota.txt
│   └── test.txt
├── tools/
│   └── poggi_mkfs.py     # Scrie sistemul de fișiere pe imagine
├── build/                # Generat la compilare (obiecte, kernel.bin, os_image.bin)
├── Makefile              # Construcție profesională
└── README.md
Comenzi disponibile în shell
După pornirea sistemului, în promptul > poți tasta:

Comanda	Descriere
help	Afișează lista comenzilor disponibile
clear	Curăță ecranul
poggi	Afișează logo‑ul oficial
mem	Arată starea heap‑ului (memorie alocată / liberă)
alloc	Test: alocă două blocuri de memorie
free	Eliberează blocurile alocate anterior
cpu	Afișează informații CPU (CPUID, suport porturi I/O)
ls	Listează fișierele din sistemul de fișiere PoggiFS
cat	Afișează conținutul unui fișier. Exemplu: cat test.txt
touch	Creează un fișier gol. Exemplu: touch fisier_nou.txt
write	Scrie text într‑un fișier. Exemplu: write test.txt "Hello, OS Dev!"
rm	Șterge un fișier. Exemplu: rm test.txt
run	Pornește task‑urile cooperative în fundal
yield	Cedează voluntar controlul către scheduler (test multitasking)
reboot	Repornește sistemul (triple fault)
Observație: Comenzile run și yield sunt demonstrații pentru multitasking.

Sistemul de fișiere PoggiFS
PoggiFS este un sistem extrem de simplu, potrivit pentru un kernel mic:

Sectorul 55 – conține indexul rădăcină (max. 16 intrări, câte 32 octeți).

Fiecare intrare: nume fișier (16 octeți) + LBA start (4 octeți) + dimensiune (4 octeți) + padding (8 octeți).

Sectoarele >= 56 – conțin datele fișierelor.

Instrumentul tools/poggi_mkfs.py scanează directorul fs_root/ și generează automat indexul și datele pe imaginea disc.

Pentru a adăuga un fișier nou în imagine:

bash
echo "Continut exemplu" > fs_root/fisier.txt
make clean && make
După repornirea în QEMU, vei vedea noul fișier cu ls și cat.

Multitasking
Poggi OS implementează multitasking cooperativ:

Două task‑uri de fundal execută calcule matematice periodice.

Comutarea se face explicit prin apelul yield() (sau automat în run).

Planificatorul salvează/restaurează starea registrelor (implementat în task_switch.asm).

Pentru a testa:

Tastează run – task‑urile încep să ruleze în fundal.

Tastează yield – comută între task‑uri.

Vei vedea mesaje precum [Task 1] Execută calcule....

Depanare (debug)
Cu consolă serială
bash
make run-debug
Orice mesaj trimis de kernel prin portul serial (COM1) va apărea în terminal.

Cu GDB
Pornește QEMU în modul de așteptare GDB:

bash
make run-gdb
În alt terminal:

bash
gdb build/bin/kernel.bin
(gdb) target remote localhost:1234
(gdb) break kernel_main
(gdb) continue
Verificare rapidă a imaginii disc
bash
hexdump -C build/bin/os_image.bin | head -5
Ultimele două caractere din primul sector trebuie să fie 55 aa.

Plan de dezvoltare
Adăugare suport pentru debugging prin serial în tot kernel‑ul.

Implementare printf() cu formatare completă.

Suport pentru directoare în PoggiFS.

Driver pentru timer (întreruperi periodice) – multitasking preemptiv.

Suport pentru alocare paginată (paging).

Portare pe ARM (Raspberry Pi).

Licență
Acest proiect este distribuit sub licența MIT. Puteți folosi, modifica și distribui codul liber, cu condiția păstrării notificării de copyright.

text
Copyright (c) 2025 Vasilache Ionut

Permission is hereby granted, free of charge, to any person obtaining a copy...
Contact
Pentru întrebări sau sugestii, puteți deschide un Issue pe GitHub sau să îl contactați pe autor direct prin GitHub.

Succes în explorarea sistemelor de operare!

text

Acest README.md este gata de copiat. După ce îl salvezi în directorul proiectului, rulează:

```bash
git add README.md
git commit -m "Adaugă README complet pentru Poggi OS"
git push
Dacă dorești să ajustezi vreo secțiune (de exemplu, să incluzi mai multe detalii tehnice sau să schimbi limba în engleză), spune-mi.
