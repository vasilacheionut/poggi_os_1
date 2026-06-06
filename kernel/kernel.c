#include "../drivers/vga.h"
#include "../drivers/keyboard.h"
#include "../drivers/heap.h"
#include "../drivers/ata.h"
#include "../include/serial.h"
#include "../include/fat32.h"
#include "../include/string.h"
#include "task.h"

typedef struct
{
    char name[16];
    unsigned int start_lba;
    unsigned int size;
    char padding[8];
} __attribute__((packed)) PoggiFile;

#define CMD_BUFFER_SIZE 64
char cmd_buffer[CMD_BUFFER_SIZE];
int cmd_index = 0;

char *test_ptr1 = 0;
char *test_ptr2 = 0;

// Declarație forward pentru execute_command (folosită în exec_autorun)
void execute_command(const char *cmd);

// ==================== TASK-URI COOPERATIVE ====================
void task_utilitar_1(void)
{
    while (1)
    {
        kprint("[Task 1] Execut calcule matematice de fundal...\n");
        task_yield();
    }
}

void task_utilitar_2(void)
{
    while (1)
    {
        kprint("[Task 2] Verific integritatea structurii PoggiFS...\n");
        task_yield();
    }
}

// ==================== CPUID ====================
void get_cpu_vendor(void)
{
    unsigned int ebx = 0, edx = 0, ecx = 0;
    unsigned int eax = 0;
    __asm__ __volatile__("cpuid" : "=b"(ebx), "=d"(edx), "=c"(ecx) : "a"(eax));
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
    return (str1[i] == '\0' && str2[i] == '\0');
}

// ==================== POGGIFS ====================
void poggifs_ls(void)
{
    unsigned short sector_buffer[256];
    ata_read_sector(55, sector_buffer);
    PoggiFile *files = (PoggiFile *)sector_buffer;
    kprint("--- LISTA FISIERE (PoggiFS) ---\n");
    int gasit = 0;
    for (int i = 0; i < 16; i++)
    {
        if (files[i].name[0] != '\0' && files[i].start_lba != 0)
        {
            kprint("  * ");
            kprint(files[i].name);
            kprint("  [Dimensiune: OK]\n");
            gasit = 1;
        }
    }
    if (!gasit)
        kprint("(Niciun fisier gasit pe disc. Indexul este gol.)\n");
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
    unsigned short content_buffer[256];
    ata_read_sector(files[gasit].start_lba, content_buffer);
    char *text = (char *)content_buffer;
    if (files[gasit].size < 512)
        text[files[gasit].size] = '\0';
    else
        text[511] = '\0';
    kprint("=== CONTINUT FISIER ===\n");
    kprint(text);
    kprint("\n=======================\n");
}

void poggifs_touch(const char *nume_nou)
{
    unsigned short sector_buffer[256];
    ata_read_sector(55, sector_buffer);
    PoggiFile *files = (PoggiFile *)sector_buffer;
    int index_liber = -1;
    unsigned int urmatorul_lba_disponibil = 56;
    for (int i = 0; i < 16; i++)
    {
        if (kstrcmp(files[i].name, nume_nou))
        {
            kprint("Eroare: Fisierul exista deja!\n");
            return;
        }
        if (files[i].name[0] == '\0' && index_liber == -1)
            index_liber = i;
        if (files[i].start_lba >= urmatorul_lba_disponibil)
            urmatorul_lba_disponibil = files[i].start_lba + 1;
    }
    if (index_liber == -1)
    {
        kprint("Eroare: PoggiFS a atins limita maxima de 16 fisiere.\n");
        return;
    }
    int j = 0;
    while (nume_nou[j] != '\0' && j < 15)
    {
        files[index_liber].name[j] = nume_nou[j];
        j++;
    }
    files[index_liber].name[j] = '\0';
    files[index_liber].start_lba = urmatorul_lba_disponibil;
    files[index_liber].size = 0;
    ata_write_sector(55, sector_buffer);
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
        kprint("Eroare: Fisierul nu exista. Foloseste 'touch'.\n");
        return;
    }
    unsigned short content_buffer[256];
    char *text_dest = (char *)content_buffer;
    for (int i = 0; i < 256; i++)
        content_buffer[i] = 0;
    int len = 0;
    while (text_de_scris[len] != '\0' && len < 511)
    {
        text_dest[len] = text_de_scris[len];
        len++;
    }
    text_dest[len] = '\0';
    files[gasit].size = len;
    ata_write_sector(files[gasit].start_lba, content_buffer);
    ata_write_sector(55, sector_buffer);
    kprint("Datele au fost scrise cu succes in ");
    kprint(nume_tinta);
    kprint(".\n");
}

void poggifs_rm(const char *nume_tinta)
{
    unsigned short sector_buffer[256];
    ata_read_sector(55, sector_buffer);
    PoggiFile *files = (PoggiFile *)sector_buffer;
    int gasit = -1;
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
    unsigned short sector_gol[256];
    for (int i = 0; i < 256; i++)
        sector_gol[i] = 0;
    ata_write_sector(files[gasit].start_lba, sector_gol);
    files[gasit].name[0] = '\0';
    files[gasit].start_lba = 0;
    files[gasit].size = 0;
    for (int j = 0; j < 8; j++)
        files[gasit].padding[j] = 0;
    ata_write_sector(55, sector_buffer);
    kprint("Fisierul '");
    kprint(nume_tinta);
    kprint("' a fost sters cu succes de pe disc!\n");
}

// ==================== COMENZI NOI ====================
void cmd_echo(const char *args)
{
    printf("%s\n", args);
}

void cmd_ps(void)
{
    task_list();
}

// ==================== EXECUTIE AUTORUN ====================
void exec_autorun(void)
{
    if (fat32_open("AUTORUN.TXT") == 0)
    {
        printf("Executing autorun.txt...\n");
        char line[256];
        int pos = 0;
        uint8_t ch;
        while (fat32_read(&ch, 1) == 1)
        {
            if (ch == '\n' || ch == '\r')
            {
                if (pos > 0)
                {
                    line[pos] = '\0';
                    execute_command(line);
                    pos = 0;
                }
                if (ch == '\r')
                    continue;
            }
            else
            {
                if (pos < 255)
                    line[pos++] = ch;
            }
        }
        if (pos > 0)
        {
            line[pos] = '\0';
            execute_command(line);
        }
        fat32_close();
    }
    else
    {
        printf("No autorun.txt found on FAT32 partition.\n");
    }
}

// ==================== PROCESARE COMENZI ====================
void execute_command(const char *cmd)
{
    if (kstrcmp(cmd, "help"))
    {
        kprint("Comenzi disponibile in Poggi OS:\n");
        kprint("  help    - Afiseaza acest meniu\n");
        kprint("  clear   - Curata ecranul complet\n");
        kprint("  poggi   - Afiseaza logo-ul oficial\n");
        kprint("  mem     - Afiseaza starea Heap-ului (RAM)\n");
        kprint("  alloc   - Test: Aloca doua blocuri de memorie\n");
        kprint("  free    - Test: Elibereaza blocurile alocate\n");
        kprint("  cpu     - Citeste informatii hardware\n");
        kprint("  ls      - Listeaza fisierele PoggiFS\n");
        kprint("  cat     - Citeste un fisier (Ex: cat nota.txt)\n");
        kprint("  touch   - Creeaza fisier gol (Ex: touch test.txt)\n");
        kprint("  write   - Scrie text in fisier (Ex: write test.txt text)\n");
        kprint("  rm      - Sterge un fisier (Ex: rm test.txt)\n");
        kprint("  run     - Porneste task-uri cooperative in fundal\n");
        kprint("  yield   - Cedeaza controlul catre task-uri\n");
        kprint("  reboot  - Restarteaza sistemul\n");
        kprint("  echo    - Afiseaza text (Ex: echo Salut!)\n");
        kprint("  ps      - Listare task-uri active\n");
        kprint("  fat32_ls - Listare fisiere pe partitia FAT32\n");
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
            kprint("Eroare: Sintaxa corecta: 'write <nume> <text>'\n");
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
        kprint("Task-uri create! Scrie 'yield' pentru a comuta.\n");
    }
    else if (kstrcmp(cmd, "yield"))
    {
        task_yield();
    }
    else if (kstrcmp(cmd, "poggi"))
    {
        kprint("\n   _____   ____   _____  _____ _____    ____   _____\n");
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
        kprint("Se aloca 128 bytes pentru test_ptr1...\n");
        test_ptr1 = (char *)kmalloc(128);
        kprint("Se aloca 256 bytes pentru test_ptr2...\n");
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
        kprint("Se trimite semnalul de reset...\n");
        for (volatile int i = 0; i < 5000000; i++)
            ;
        outb(0x64, 0xFE);
    }
    else if (cmd[0] == 'e' && cmd[1] == 'c' && cmd[2] == 'h' && cmd[3] == 'o' && cmd[4] == ' ')
    {
        cmd_echo(&cmd[5]);
    }
    else if (kstrcmp(cmd, "ps"))
    {
        cmd_ps();
    }
    else if (kstrcmp(cmd, "fat32_ls"))
    {
        fat32_list_root();
    }
    else if (kstrcmp(cmd, ""))
    {
        // nimic
    }
    else
    {
        kprint("Eroare: Comanda '");
        kprint(cmd);
        kprint("' nu este recunoscuta. Scrie 'help'.\n");
    }
}

// ==================== KERNEL MAIN ====================
void kernel_main(void)
{
    kheap_init(0x100000, 65536);
    task_init();
    serial_init();
    clear_screen();

    kprint("====================================================\n");
    kprint("   Poggi OS v7.0 -- HARDWARE INTERACTION KERNEL     \n");
    kprint("====================================================\n");
    kprint("Suport CPUID si Porturi I/O activat cu succes.\n");
    kprint("Debug serial activat (COM1, 38400 baud).\n");

    fat32_init();
    exec_autorun();

    kprint("Scrie 'help' ca sa vezi comenzile disponibile!\n\n");
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