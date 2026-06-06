#include "../include/fat32.h"
#include "../include/serial.h"
#include "../include/string.h"
#include "../drivers/ata.h"

static BPB bpb;
static uint32_t partition_lba = 2048; // offset-ul partiției FAT32
static uint32_t first_data_sector;
static uint32_t fat_lba;
static uint32_t sectors_per_cluster;

static uint32_t current_cluster;
static uint32_t current_offset_in_cluster;
static uint32_t current_file_size;
static uint32_t current_file_pos;

// Citire sector de pe același disc (master) folosind driverul ATA existent
static void read_sector_bytes(uint32_t lba, void *buffer)
{
    unsigned short temp[256]; // 512 octeți
    ata_read_sector(lba, temp);
    kmemcpy(buffer, temp, 512);
}

static void read_cluster(uint32_t cluster, void *buffer)
{
    uint32_t lba = first_data_sector + (cluster - 2) * sectors_per_cluster;
    for (uint32_t i = 0; i < sectors_per_cluster; i++)
    {
        read_sector_bytes(lba + i, (uint8_t *)buffer + i * 512);
    }
}

static uint32_t get_next_cluster(uint32_t cluster)
{
    uint32_t fat_offset = cluster * 4;
    uint32_t fat_sector = fat_lba + (fat_offset / 512);
    uint32_t fat_offset_in_sector = fat_offset % 512;
    uint8_t sector[512];
    read_sector_bytes(fat_sector, sector);
    return *(uint32_t *)(sector + fat_offset_in_sector) & 0x0FFFFFFF;
}

void fat32_init(void)
{
    uint8_t sector[512];
    read_sector_bytes(partition_lba, sector);
    kmemcpy(&bpb, sector, sizeof(BPB));

    if (bpb.bytes_per_sector != 512)
    {
        printf("FAT32: Invalid bytes per sector (%d).\n", bpb.bytes_per_sector);
        return;
    }
    // Verifică semnătura de boot la offset 510
    uint16_t *sig = (uint16_t *)(sector + 510);
    if (*sig != 0xAA55)
    {
        printf("FAT32: Invalid boot signature 0x%x\n", *sig);
        return;
    }
    sectors_per_cluster = bpb.sectors_per_cluster;
    uint32_t fat_size = bpb.fat_size_32;
    first_data_sector = partition_lba + bpb.reserved_sectors + (bpb.num_fats * fat_size);
    fat_lba = partition_lba + bpb.reserved_sectors;
    printf("FAT32: OK, partition LBA %d, root cluster %d, sectors per cluster %d\n",
           partition_lba, bpb.root_cluster, sectors_per_cluster);
}

int fat32_open(const char *filename)
{
    char fat_name[11];
    kmemset(fat_name, ' ', 11);
    const char *dot = kstrchr(filename, '.');
    if (dot)
    {
        int namelen = dot - filename;
        if (namelen > 8)
            namelen = 8;
        kmemcpy(fat_name, filename, namelen);
        int extlen = kstrlen(dot + 1);
        if (extlen > 3)
            extlen = 3;
        kmemcpy(fat_name + 8, dot + 1, extlen);
    }
    else
    {
        int len = kstrlen(filename);
        if (len > 8)
            len = 8;
        kmemcpy(fat_name, filename, len);
    }
    for (int i = 0; i < 11; i++)
    {
        if (fat_name[i] >= 'a' && fat_name[i] <= 'z')
            fat_name[i] -= 0x20;
    }

    uint32_t cluster = bpb.root_cluster;
    uint8_t sector[sectors_per_cluster * 512];
    while (cluster < 0x0FFFFFF8)
    {
        read_cluster(cluster, sector);
        uint32_t entries_per_cluster = (sectors_per_cluster * 512) / 32;
        for (uint32_t i = 0; i < entries_per_cluster; i++)
        {
            DirEntry *entry = (DirEntry *)(sector + i * 32);
            if (entry->name[0] == 0x00)
                break;
            if ((uint8_t)entry->name[0] == 0xE5)
                continue;
            if (kmemcmp(entry->name, fat_name, 11) == 0)
            {
                current_cluster = (entry->first_cluster_hi << 16) | entry->first_cluster_lo;
                current_offset_in_cluster = 0;
                current_file_size = entry->file_size;
                current_file_pos = 0;
                return 0;
            }
        }
        cluster = get_next_cluster(cluster);
    }
    printf("FAT32: File %s not found\n", filename);
    return -1;
}

int fat32_read(void *buffer, uint32_t bytes)
{
    uint8_t *buf = (uint8_t *)buffer;
    uint32_t total_read = 0;
    while (total_read < bytes && current_file_pos < current_file_size)
    {
        uint32_t remaining_in_cluster = sectors_per_cluster * 512 - current_offset_in_cluster;
        if (remaining_in_cluster == 0)
        {
            current_cluster = get_next_cluster(current_cluster);
            if (current_cluster >= 0x0FFFFFF8)
                break;
            current_offset_in_cluster = 0;
            remaining_in_cluster = sectors_per_cluster * 512;
        }
        uint32_t to_read = bytes - total_read;
        if (to_read > current_file_size - current_file_pos)
            to_read = current_file_size - current_file_pos;
        if (to_read > remaining_in_cluster)
            to_read = remaining_in_cluster;

        uint8_t cluster_buf[sectors_per_cluster * 512];
        read_cluster(current_cluster, cluster_buf);
        kmemcpy(buf + total_read, cluster_buf + current_offset_in_cluster, to_read);

        current_offset_in_cluster += to_read;
        current_file_pos += to_read;
        total_read += to_read;
    }
    return total_read;
}

void fat32_close(void) {}

void fat32_list_root(void)
{
    uint32_t cluster = bpb.root_cluster;
    printf("Files in root (FAT32):\n");
    while (cluster < 0x0FFFFFF8)
    {
        uint8_t sector[sectors_per_cluster * 512];
        read_cluster(cluster, sector);
        uint32_t entries_per_cluster = (sectors_per_cluster * 512) / 32;
        for (uint32_t i = 0; i < entries_per_cluster; i++)
        {
            DirEntry *entry = (DirEntry *)(sector + i * 32);
            if (entry->name[0] == 0x00)
                break;
            if ((uint8_t)entry->name[0] == 0xE5)
                continue;
            char name[12];
            kmemcpy(name, entry->name, 11);
            name[11] = 0;
            printf("%s\n", name);
        }
        cluster = get_next_cluster(cluster);
    }
}