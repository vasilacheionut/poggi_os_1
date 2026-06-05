// kernel/kernel.c
#include "../drivers/vga.h"
#include "../drivers/keyboard.h"
#include "../drivers/heap.h"
#include "../drivers/ata.h" // Adăugăm noul driver de disc
#include "task.h"           // Adăugat pentru suportul de Multitasking Cooperativ

// Structura unui fișier în tabela PoggiFS (Dimensiune: 32 bytes)
typedef struct
{
    char name[16];          // Numele fișierului (ex: "salut.txt")
    unsigned int start_lba; // Sectorul de pe disc unde începe conținutul
    unsigned int size;      // Dimensiunea fișierului în bytes
    char padding[8];        // Aliniere rigidă la 32 de bytes
} __attribute__((packed)) PoggiFile;

#define CMD_BUFFER_SIZE 64
char cmd_buffer[CMD_BUFFER_SIZE];
int cmd_index = 0;

// Pointeri globali pentru testul de heap
char *test_ptr1 = 0;
char *test_ptr2 = 0;

// ====================================================================
// FUNCȚII DE TEST PENTRU TASK-URILE COOPERATIVE IN FUNDAL
// ====================================================================
void task_utilitar_1(void)
{
    while (1)
    {
        kprint("[Task 1] Execut calcule matematice de fundal...\n");
        task_yield(); // Cedează voluntar controlul procesorului următorului task
    }
}

void task_utilitar_2(void)
{
    while (1)
    {
        kprint("[Task 2] Verific integritatea structurii PoggiFS...\n");
        task_yield(); // Cedează voluntar controlul procesorului următorului task
    }
}
// ====================================================================

// Funcție sigură de CPUID pentru format freestanding 64-bit
void get_cpu_vendor(void)
{
    unsigned int ebx = 0, edx = 0, ecx = 0;
    unsigned int eax = 0;

    // Executăm instrucțiunea cpuid nativă
    __asm__ __volatile__(
        "cpuid"
        : "=b"(ebx), "=d"(edx), "=c"(ecx)
        : "a"(eax));

    // Reconstituim stringul din registre
    char vendor[13];
    vendor[0] = ebx & 0xFF;
    vendor[1] = (ebx >> 8) & 0xFF;
    vendor[2] = (ebx >> 16) & 0xFF;
    vendor[3] = (ebx >> 24) & 0xFF;

    vendor[4] = edx & 0xFF;
    vendor[5] = (edx >> 8) & 0xFF;
    vendor[6] = (edx >> 16) & 0xFF;
    vendor[7] = (edx >> 24) & 0xFF;

    vendor[8] = ecx & 0xFF;
    vendor[9] = (ecx >> 8) & 0xFF;
    vendor[10] = (ecx >> 16) & 0xFF;
    vendor[11] = (ecx >> 24) & 0xFF;
    vendor[12] = '\0';

    kprint("Procesor detectat: ");
    kprint(vendor);
    kprint("\nArhitectura: x86_64 (64-bit Long Mode)\n");
}

int kstrcmp(const char *str1, const char *str2)
{
    int i = 0;
    while (str1[i] != '\0' && str2[i] != '\0')
    {
        if (str1[i] != str2[i])
            return 0;
        i++;
    }
    if (str1[i] == '\0' && str2[i] == '\0')
        return 1;
    return 0;
}

void poggifs_ls(void)
{
    unsigned short sector_buffer[256];

    // Citim Sectorul 55 (unde stocăm indexul root de fișiere)
    ata_read_sector(55, sector_buffer);

    PoggiFile *files = (PoggiFile *)sector_buffer;

    kprint("--- LISTA FISIERE (PoggiFS) ---\n");
    int gasit = 0;

    // Citim maxim 16 intrări posibile în acel sector (16 * 32 bytes = 512 bytes)
    for (int i = 0; i < 16; i++)
    {
        if (files[i].name[0] != '\0' && files[i].start_lba != 0)
        {
            kprint("  * ");
            kprint(files[i].name);
            kprint("  [Dimensiune: ");

            // Afișăm o confirmare simplă de integritate
            kprint("OK]\n");
            gasit = 1;
        }
    }

    if (!gasit)
    {
        kprint("(Niciun fisier gasit pe disc. Indexul este gol.)\n");
    }
    kprint("-------------------------------\n");
}

void poggifs_cat(const char *nume_cautat)
{
    unsigned short sector_buffer[256];
    ata_read_sector(55, sector_buffer);

    PoggiFile *files = (PoggiFile *)sector_buffer;
    int gasit = -1;

    for (int i = 0; i < 16; i++)
    {
        if (kstrcmp(files[i].name, nume_cautat))
        {
            gasit = i;
            break;
        }
    }

    if (gasit == -1)
    {
        kprint("Eroare: Fisierul '");
        kprint(nume_cautat);
        kprint("' nu a fost gasit.\n");
        return;
    }

    // Citim sectorul fizic de conținut mapat fișierului găsit
    unsigned short content_buffer[256];
    ata_read_sector(files[gasit].start_lba, content_buffer);

    char *text = (char *)content_buffer;

    // Protecție buffer împotriva overflow-ului la afișare string
    if (files[gasit].size < 512)
    {
        text[files[gasit].size] = '\0';
    }
    else
    {
        text[511] = '\0';
    }

    kprint("=== CONTINUT FISIER ===\n");
    kprint(text);
    kprint("\n=======================\n");
}

void poggifs_touch(const char *nume_nou)
{
    unsigned short sector_buffer[256];
    ata_read_sector(55, sector_buffer); // Încărcăm indexul din LBA 55

    PoggiFile *files = (PoggiFile *)sector_buffer;
    int index_liber = -1;
    unsigned int urmatorul_lba_disponibil = 56; // Primul sector liber implicit pentru date

    for (int i = 0; i < 16; i++)
    {
        // Prevenim crearea de fișiere duplicate
        if (kstrcmp(files[i].name, nume_nou))
        {
            kprint("Eroare: Fisierul exista deja!\n");
            return;
        }
        // Identificăm primul slot gol în tabelă
        if (files[i].name[0] == '\0' && index_liber == -1)
        {
            index_liber = i;
        }
        // Calculăm dinamic următorul LBA nelucrat prin adunarea structurilor active
        if (files[i].start_lba >= urmatorul_lba_disponibil)
        {
            urmatorul_lba_disponibil = files[i].start_lba + 1;
        }
    }

    if (index_liber == -1)
    {
        kprint("Eroare: PoggiFS a atins limita maxima de 16 fisiere pe sectorul root.\n");
        return;
    }

    // Copiem numele în siguranță (max 15 caractere + \0)
    int j = 0;
    while (nume_nou[j] != '\0' && j < 15)
    {
        files[index_liber].name[j] = nume_nou[j];
        j++;
    }
    files[index_liber].name[j] = '\0';
    files[index_liber].start_lba = urmatorul_lba_disponibil;
    files[index_liber].size = 0; // Inițial gol

    // Rescriem indexul actualizat în sectorul 55
    ata_write_sector(55, sector_buffer);

    // Ștergem reziduurile de pe sectorul nou alocat (îl umplem cu 0 brute)
    unsigned short sector_gol[256];
    for (int i = 0; i < 256; i++)
        sector_gol[i] = 0;
    ata_write_sector(urmatorul_lba_disponibil, sector_gol);

    kprint("Fisierul '");
    kprint(nume_nou);
    kprint("' a fost creat cu succes!\n");
}

void poggifs_write(const char *nume_tinta, const char *text_de_scris)
{
    unsigned short sector_buffer[256];
    ata_read_sector(55, sector_buffer);

    PoggiFile *files = (PoggiFile *)sector_buffer;
    int gasit = -1;

    for (int i = 0; i < 16; i++)
    {
        if (kstrcmp(files[i].name, nume_tinta))
        {
            gasit = i;
            break;
        }
    }

    if (gasit == -1)
    {
        kprint("Eroare: Fisierul nu exista. Foloseste mai intai 'touch'.\n");
        return;
    }

    // Alocăm un buffer local curat pe stivă pentru sector (512 bytes)
    unsigned short content_buffer[256];
    char *text_dest = (char *)content_buffer;

    for (int i = 0; i < 256; i++)
        content_buffer[i] = 0;

    // Copiem stringul primit (limită fizică de 511 caractere pe sector)
    int len = 0;
    while (text_de_scris[len] != '\0' && len < 511)
    {
        text_dest[len] = text_de_scris[len];
        len++;
    }
    text_dest[len] = '\0';

    // Salvăm lungimea reală în structura fișierului
    files[gasit].size = len;

    // Împingem textul în sectorul lui fizic LBA
    ata_write_sector(files[gasit].start_lba, content_buffer);

    // Salvăm și noua dimensiune actualizată în tabela de index (Sectorul 55)
    ata_write_sector(55, sector_buffer);

    kprint("Datele au fost scrise cu succes in ");
    kprint(nume_tinta);
    kprint(".\n");
}

void poggifs_rm(const char *nume_tinta)
{
    unsigned short sector_buffer[256];
    // Citim tabela de index din sectorul 55
    ata_read_sector(55, sector_buffer);

    PoggiFile *files = (PoggiFile *)sector_buffer;
    int gasit = -1;

    // Căutăm fișierul în cele 16 sloturi disponibile
    for (int i = 0; i < 16; i++)
    {
        if (files[i].name[0] != '\0' && kstrcmp(files[i].name, nume_tinta))
        {
            gasit = i;
            break;
        }
    }

    if (gasit == -1)
    {
        kprint("Eroare: Fisierul '");
        kprint(nume_tinta);
        kprint("' nu a fost gasit.\n");
        return;
    }

    // Pasul de curățare hardware: suprascriem sectorul de date cu 0 pentru a nu lăsa reziduuri
    unsigned short sector_gol[256];
    for (int i = 0; i < 256; i++)
    {
        sector_gol[i] = 0;
    }
    ata_write_sector(files[gasit].start_lba, sector_gol);

    // Resetăm complet structura fișierului în tabela de index
    files[gasit].name[0] = '\0';
    files[gasit].start_lba = 0;
    files[gasit].size = 0;
    for (int j = 0; j < 8; j++)
    {
        files[gasit].padding[j] = 0;
    }

    // Salvăm tabela de index actualizată înapoi pe disc (Sectorul 55)
    ata_write_sector(55, sector_buffer);

    kprint("Fisierul '");
    kprint(nume_tinta);
    kprint("' a fost sters cu succes de pe disc!\n");
}

void execute_command(const char *cmd)
{
    if (kstrcmp(cmd, "help"))
    {
        kprint("Comenzi disponibile in Poggi OS:\n");
        kprint("  help   - Afiseaza acest meniu\n");
        kprint("  clear  - Curata ecranul complet\n");
        kprint("  poggi  - Afiseaza logo-ul oficial\n");
        kprint("  mem    - Afiseaza starea actuala a Heap-ului (RAM)\n");
        kprint("  alloc  - Test: Aloca dinamic doua blocuri de memorie\n");
        kprint("  free   - Test: Elibereaza blocurile alocate\n");
        kprint("  cpu    - Citeste informatiile hardware din procesor\n");
        kprint("  ls     - Afiseaza fisierele de pe PoggiFS\n");
        kprint("  cat    - Citeste un fisier (Ex: cat nota.txt)\n");
        kprint("  touch  - Creeaza un fisier nou gol (Ex: touch test.txt)\n");
        kprint("  write  - Scrie text intr-un fisier (Ex: write test.txt text_nou)\n");
        kprint("  rm     - Sterge un fisier de pe disc (Ex: rm test.txt)\n");
        kprint("  run    - Porneste task-urile cooperative in fundal\n");     // Adăugat pentru Multitasking
        kprint("  yield  - Cedeaza controlul catre task-urile din fundal\n"); // Adăugat pentru Multitasking
        kprint("  reboot - Trimite semnal hardware de restart\n");
    }
    else if (kstrcmp(cmd, "ls"))
    {
        poggifs_ls();
    }
    else if (cmd[0] == 'c' && cmd[1] == 'a' && cmd[2] == 't' && cmd[3] == ' ')
    {
        poggifs_cat(&cmd[4]);
    }
    else if (cmd[0] == 't' && cmd[1] == 'o' && cmd[2] == 'u' && cmd[3] == 'c' && cmd[4] == 'h' && cmd[5] == ' ')
    {
        poggifs_touch(&cmd[6]);
    }
    else if (cmd[0] == 'w' && cmd[1] == 'r' && cmd[2] == 'i' && cmd[3] == 't' && cmd[4] == 'e' && cmd[5] == ' ')
    {
        // Parsare manuală pentru: "write <nume> <text_fără_spații>"
        int i = 6;
        char nume_fisier[16];
        int n_idx = 0;

        while (cmd[i] != ' ' && cmd[i] != '\0' && n_idx < 15)
        {
            nume_fisier[n_idx++] = cmd[i++];
        }
        nume_fisier[n_idx] = '\0';

        if (cmd[i] == ' ')
        {
            i++;
            poggifs_write(nume_fisier, &cmd[i]);
        }
        else
        {
            kprint("Eroare: Sintaxa corecta este 'write <nume> <text>'\n");
        }
    }
    else if (kstrcmp(cmd, "clear"))
    {
        clear_screen();
    }
    else if (cmd[0] == 'r' && cmd[1] == 'm' && cmd[2] == ' ')
    {
        poggifs_rm(&cmd[3]);
    }
    else if (kstrcmp(cmd, "run"))
    {
        kprint("Se pornesc task-urile cooperative in fundal...\n");
        task_create(task_utilitar_1);
        task_create(task_utilitar_2);
        kprint("Task-uri create! Scrie 'yield' ca sa le vezi ruland alternativ.\n");
    }
    else if (kstrcmp(cmd, "yield"))
    {
        // Consola cedează voluntar controlul stivei hardware către scheduler
        task_yield();
    }
    // ==========================================
    else if (kstrcmp(cmd, "poggi"))
    {
        kprint("\n");
        kprint("   _____   ____   _____  _____ _____    ____   _____\n");
        kprint("  |  __ \\ / __ \\ / ____|/ ____|_   _|  / __ \\ / ____|\n");
        kprint("  | |__) | |  | | |  __| |  __  | |   | |  | | (___  \n");
        kprint("  |  ___/| |  | | | |_ | | |_ | | |   | |  | |\\___ \\ \n");
        kprint("  | |    | |__| | |__| | |__| |_| |_  | |__| |____) |\n");
        kprint("  |_|     \\____/ \\_____|\\_____|_____|  \\____/|_____/ \n");
        kprint("                 Sistem de Operare Nativ\n\n");
    }
    else if (kstrcmp(cmd, "mem"))
    {
        kheap_dump();
    }
    else if (kstrcmp(cmd, "alloc"))
    {
        if (test_ptr1 != 0 || test_ptr2 != 0)
        {
            kprint("Eroare: Blocurile sunt deja alocate! Scrie 'free'.\n");
            return;
        }
        kprint("Se aloca dinamic 128 bytes pentru test_ptr1...\n");
        test_ptr1 = (char *)kmalloc(128);
        kprint("Se aloca dinamic 256 bytes pentru test_ptr2...\n");
        test_ptr2 = (char *)kmalloc(256);
        kprint("Alocare finalizata! Scrie 'mem'.\n");
    }
    else if (kstrcmp(cmd, "free"))
    {
        if (test_ptr1 == 0 && test_ptr2 == 0)
        {
            kprint("Eroare: Nu exista blocuri de eliberat.\n");
            return;
        }
        kprint("Se elibereaza memoria...\n");
        kfree(test_ptr1);
        kfree(test_ptr2);
        test_ptr1 = 0;
        test_ptr2 = 0;
        kprint("Memorie eliberata! Scrie 'mem'.\n");
    }
    else if (kstrcmp(cmd, "cpu"))
    {
        get_cpu_vendor();
    }
    else if (kstrcmp(cmd, "reboot"))
    {
        kprint("Se trimite semnalul de Pulse Reset...\n");
        kprint("La revedere!\n");

        for (volatile int i = 0; i < 5000000; i++)
            ;

        // Apelăm controllerul de tastatură 8042 pentru restart fizic instantaneu
        outb(0x64, 0xFE);
    }
    else if (kstrcmp(cmd, ""))
    {
        // Enter simplu
    }
    else
    {
        kprint("Eroare: Comanda '");
        kprint(cmd);
        kprint("' nu este recunoscuta. Scrie 'help'.\n");
    }
}

void kernel_main(void)
{
    kheap_init(0x100000, 65536);

    // Inițializăm subsistemul de Task Control Blocks.
    // Această funcție înregistrează automat kernelul curent drept Task-ul cu ID 0
    task_init();

    clear_screen();

    kprint("====================================================\n");
    kprint("   Poggi OS v7.0 -- HARDWARE INTERACTION KERNEL     \n");
    kprint("====================================================\n");
    kprint("Suport CPUID si Porturi I/O activat cu succes.\n");
    kprint("Scrie 'help' ca sa vezi noile comenzi de control!\n\n");
    kprint("> ");

    cmd_index = 0;
    cmd_buffer[0] = '\0';

    while (1)
    {
        char tasta = kgetch();

        if (tasta == '\n')
        {
            kputchar('\n');
            cmd_buffer[cmd_index] = '\0';
            execute_command(cmd_buffer);
            cmd_index = 0;
            cmd_buffer[0] = '\0';
            kprint("> ");
        }
        else if (tasta == '\b')
        {
            if (cmd_index > 0)
            {
                cmd_index--;
                cmd_buffer[cmd_index] = '\0';
                kputchar('\b');
            }
        }
        else
        {
            if (cmd_index < CMD_BUFFER_SIZE - 1)
            {
                cmd_buffer[cmd_index] = tasta;
                cmd_index++;
                kputchar(tasta);
            }
        }
    }
}