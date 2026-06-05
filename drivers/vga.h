// drivers/vga.h
#ifndef VGA_H
#define VGA_H

void outb(unsigned short port, unsigned char val);
unsigned char inb(unsigned short port);

void kputchar(char c);
void kprint(const char* str);
void clear_screen(void);

#endif