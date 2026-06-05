// drivers/vga.c
#include "vga.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_ADDRESS 0xB8000

int cursor_x = 0;
int cursor_y = 0;

void outb(unsigned short port, unsigned char val)
{
    __asm__ __volatile__("outb %0, %1" : : "a"(val), "Nd"(port));
}

unsigned char inb(unsigned short port)
{
    unsigned char ret;
    __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static void update_cursor(int x, int y)
{
    unsigned short pos = y * VGA_WIDTH + x;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (unsigned char)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (unsigned char)((pos >> 8) & 0xFF));
}

static void scroll(void)
{
    char *video_memory = (char *)VGA_ADDRESS;
    if (cursor_y >= VGA_HEIGHT)
    {
        for (int i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH * 2; i++)
        {
            video_memory[i] = video_memory[i + VGA_WIDTH * 2];
        }
        for (int i = (VGA_HEIGHT - 1) * VGA_WIDTH * 2; i < VGA_HEIGHT * VGA_WIDTH * 2; i += 2)
        {
            video_memory[i] = ' ';
            video_memory[i + 1] = 0x07;
        }
        cursor_y = VGA_HEIGHT - 1;
    }
}

void kputchar(char c)
{
    char *video_memory = (char *)VGA_ADDRESS;

    if (c == '\n')
    {
        cursor_x = 0;
        cursor_y++;
    }
    else if (c == '\b')
    {
        if (cursor_x > 0)
        {
            cursor_x--;
            int offset = (cursor_y * VGA_WIDTH + cursor_x) * 2;
            video_memory[offset] = ' ';
            video_memory[offset + 1] = 0x07;
        }
    }
    else
    {
        int offset = (cursor_y * VGA_WIDTH + cursor_x) * 2;
        video_memory[offset] = c;
        video_memory[offset + 1] = 0x0F;
        cursor_x++;
    }

    if (cursor_x >= VGA_WIDTH)
    {
        cursor_x = 0;
        cursor_y++;
    }

    scroll();
    update_cursor(cursor_x, cursor_y);
}

void kprint(const char *str)
{
    for (int i = 0; str[i] != '\0'; i++)
    {
        kputchar(str[i]);
    }
}

void clear_screen(void)
{
    char *video_memory = (char *)VGA_ADDRESS;
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
    {
        video_memory[i * 2] = ' ';
        video_memory[i * 2 + 1] = 0x07;
    }
    cursor_x = 0;
    cursor_y = 0;
    update_cursor(cursor_x, cursor_y);
}