// drivers/heap.c
#include "heap.h"
#include "vga.h" // Pentru a putea printa erori de memorie dacă e cazul

struct BlockHeader
{
    unsigned long size;
    int is_free;
    struct BlockHeader *next;
};

// Indicatorul către primul bloc din memoria noastră (capul listei înlănțuite)
static struct BlockHeader *heap_start = 0;

void kheap_init(unsigned long start_address, unsigned long size)
{
    // Plasăm primul header chiar la începutul zonei rezervate
    heap_start = (struct BlockHeader *)start_address;

    // La început, avem un singur bloc uriaș și liber
    heap_start->size = size - sizeof(struct BlockHeader);
    heap_start->is_free = 1;
    heap_start->next = 0;
}

void *kmalloc(unsigned long size)
{
    struct BlockHeader *current = heap_start;

    // Aliniem dimensiunea cerută la 8 bytes pentru performanța procesorului 64-bit
    size = (size + 7) & ~7;

    // Parcurgem lista și căutăm primul bloc liber destul de mare (Algoritmul First-Fit)
    while (current != 0)
    {
        if (current->is_free && current->size >= size)
        {

            // Dacă blocul găsit e mult mai mare decât avem nevoie, îl spargem în două
            // Avem nevoie de spațiu pentru datele noastre + un nou header în spate
            if (current->size > size + sizeof(struct BlockHeader) + 8)
            {
                struct BlockHeader *next_block = (struct BlockHeader *)((unsigned long)current + sizeof(struct BlockHeader) + size);

                next_block->size = current->size - size - sizeof(struct BlockHeader);
                next_block->is_free = 1;
                next_block->next = current->next;

                current->size = size;
                current->next = next_block;
            }

            // Marcăm blocul ca ocupat
            current->is_free = 0;

            // Returnăm adresa de memorie de IMEDIAT DUPĂ header (unde încep datele utile)
            return (void *)((unsigned long)current + sizeof(struct BlockHeader));
        }
        current = current->next;
    }

    // Dacă am parcurs tot și nu mai avem spațiu, am rămas fără RAM (Out of Memory)
    kprint("\n[KERNEL PANIC] Out of memory in kmalloc!\n");
    return 0;
}

void kfree(void *ptr)
{
    if (!ptr)
        return;

    // Mergem înapoi în memorie cu câțiva bytes ca să găsim header-ul acestui bloc
    struct BlockHeader *header = (struct BlockHeader *)((unsigned long)ptr - sizeof(struct BlockHeader));
    header->is_free = 1; // Îl marcăm ca liber!

    // Optimizare: Unim blocurile libere consecutive ca să prevenim fragmentarea memoriei
    struct BlockHeader *current = heap_start;

    while (current != 0)
    {
        if (current->is_free && current->next && current->next->is_free)
        {
            current->size += sizeof(struct BlockHeader) + current->next->size;
            current->next = current->next->next;
            continue; // Verificăm din nou pentru blocul proaspăt extins
        }
        current = current->next;
    }
}

// O funcție de debugging foarte utilă pentru a vedea cum "arată" memoria RAM din interior
void kheap_dump(void)
{
    struct BlockHeader *current = heap_start;
    kprint("--- STARE HEAP MEMORIE ---\n");
    int i = 0;
    while (current != 0)
    {
        kprint("Bloc ");
        kputchar('0' + i);
        kprint(": Adresa=");
        // Printăm simplificat starea
        if (current->is_free)
        {
            kprint(" LIBER  | ");
        }
        else
        {
            kprint(" OCUPAT | ");
        }
        kprint("Dimensiune utila bytes: ");

        // Convertim dimensiunea rudimentar în text pentru afișare rapidă
        unsigned long s = current->size;
        char buf[16];
        int idx = 0;
        if (s == 0)
            kputchar('0');
        while (s > 0 && idx < 15)
        {
            buf[idx++] = '0' + (s % 10);
            s /= 10;
        }
        for (int j = idx - 1; j >= 0; j--)
            kputchar(buf[j]);

        kprint("\n");
        current = current->next;
        i++;
    }
    kprint("--------------------------\n");
}