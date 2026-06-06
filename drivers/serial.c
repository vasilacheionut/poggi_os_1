#include "../include/serial.h"
#include "../include/string.h"
#include <stdint.h>

#define COM1 0x3F8

static inline void outb(uint16_t port, uint8_t val)
{
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void serial_init(void)
{
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x80);
    outb(COM1 + 0, 0x03);
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);
    outb(COM1 + 2, 0xC7);
    outb(COM1 + 4, 0x0B);
}

void serial_putchar(char c)
{
    while ((inb(COM1 + 5) & 0x20) == 0)
        ;
    outb(COM1, c);
}

void serial_puts(const char *s)
{
    while (*s)
        serial_putchar(*s++);
}

void serial_puthex(uint32_t num)
{
    const char *hex = "0123456789ABCDEF";
    serial_puts("0x");
    for (int i = 28; i >= 0; i -= 4)
    {
        serial_putchar(hex[(num >> i) & 0xF]);
    }
}

static void print_num(int num)
{
    char buf[12];
    int i = 0;
    if (num == 0)
    {
        serial_putchar('0');
        return;
    }
    if (num < 0)
    {
        serial_putchar('-');
        num = -num;
    }
    while (num)
    {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    }
    while (i--)
        serial_putchar(buf[i]);
}

void printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    for (const char *p = fmt; *p; p++)
    {
        if (*p == '%' && *(p + 1))
        {
            p++;
            switch (*p)
            {
            case 'd':
                print_num(va_arg(args, int));
                break;
            case 's':
                serial_puts(va_arg(args, const char *));
                break;
            case 'c':
                serial_putchar(va_arg(args, int));
                break;
            case 'x':
                serial_puthex(va_arg(args, uint32_t));
                break;
            default:
                serial_putchar(*p);
            }
        }
        else
        {
            serial_putchar(*p);
        }
    }
    va_end(args);
}