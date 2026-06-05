// drivers/ata.c
#include "ata.h"

// Citirea unui cuvânt pe 16 biți (word) dintr-un port I/O hardware
static inline unsigned short inw(unsigned short port)
{
    unsigned short result;
    __asm__ __volatile__("inw %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

// Scrierea unui cuvânt pe 16 biți (word) într-un port I/O hardware
static inline void outw(unsigned short port, unsigned short data)
{
    __asm__ __volatile__("outw %0, %1" : : "a"(data), "Nd"(port));
}

// Funcția de citire a unui sector (512 bytes) folosind mod LBA28
void ata_read_sector(unsigned int lba, unsigned short *dest)
{
    // 1. Trimitem semnal controllerului că vrem să citim exact 1 sector
    outb(0x1F2, 1);

    // 2. Trimitem biții adresei logice de sector (LBA) pe porturile dedicate
    outb(0x1F3, (unsigned char)lba);
    outb(0x1F4, (unsigned char)(lba >> 8));
    outb(0x1F5, (unsigned char)(lba >> 16));

    // Configurație Master + Mod LBA + ultimii 4 biți din adresa LBA
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));

    // 3. Trimitem comanda hardware de READ SECTORS (0x20)
    outb(0x1F7, 0x20);

    // 4. Polling: Așteptăm ca discul să nu mai fie ocupat (BSY) și să fie gata de transfer (DRQ)
    while ((inb(0x1F7) & 0x80) != 0)
        ;
    while ((inb(0x1F7) & 0x08) == 0)
        ;

    // 5. Citim exact 256 de cuvinte de 16 biți (adică cei 512 bytes ai sectorului) în RAM
    for (int i = 0; i < 256; i++)
    {
        dest[i] = inw(0x1F0);
    }
}

// Funcția de scriere a unui sector (512 bytes) de la o adresă din RAM pe disc
void ata_write_sector(unsigned int lba, unsigned short *src)
{
    // 1. Trimitem semnal controllerului că vrem să scriem exact 1 sector
    outb(0x1F2, 1);

    // 2. Trimitem biții adresei logice de sector (LBA) pe porturile dedicate
    outb(0x1F3, (unsigned char)lba);
    outb(0x1F4, (unsigned char)(lba >> 8));
    outb(0x1F5, (unsigned char)(lba >> 16));

    // Configurație Master + Mod LBA + ultimii 4 biți din adresa LBA
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));

    // 3. Trimitem comanda hardware de WRITE SECTORS (0x30)
    outb(0x1F7, 0x30);

    // 4. Polling: Așteptăm ca bufferul controllerului să devină gata să preia datele brute
    while ((inb(0x1F7) & 0x80) != 0)
        ;
    while ((inb(0x1F7) & 0x08) == 0)
        ;

    // 5. Împingem cele 256 de cuvinte de 16 biți din memoria RAM către controllerul de disc
    for (int i = 0; i < 256; i++)
    {
        outw(0x1F0, src[i]);
    }

    // 6. Forțăm controllerul hardware să își golească memoria cache direct pe disc (Persistență)
    outb(0x1F7, 0xE7); // Comanda hardware CACHE FLUSH
    while ((inb(0x1F7) & 0x80) != 0)
        ; // Așteptăm ca scrierea fizică să fie completă
}