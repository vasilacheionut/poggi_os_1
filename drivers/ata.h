// drivers/ata.h
#ifndef ATA_H
#define ATA_H

#include "vga.h"

void ata_read_sector(unsigned int lba, unsigned short *dest);
void ata_write_sector(unsigned int lba, unsigned short *src); // Noua funcție de scriere

#endif