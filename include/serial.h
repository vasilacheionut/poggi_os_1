#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>
#include <stdarg.h>

void serial_init(void);
void serial_putchar(char c);
void serial_puts(const char *s);
void serial_puthex(uint32_t num);
void printf(const char *fmt, ...);

#endif