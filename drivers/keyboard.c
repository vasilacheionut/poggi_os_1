// drivers/keyboard.c
#include "keyboard.h"
#include "vga.h" // Avem nevoie de inb() definit în vga.h

static int shift_apasat = 0;
static int caps_lock_activ = 0;

static const char scancode_to_ascii_normal[] = {
    0,
    27,
    '1',
    '2',
    '3',
    '4',
    '5',
    '6',
    '7',
    '8',
    '9',
    '0',
    '-',
    '=',
    '\b',
    '\t',
    'q',
    'w',
    'e',
    'r',
    't',
    'y',
    'u',
    'i',
    'o',
    'p',
    '[',
    ']',
    '\n',
    0,
    'a',
    's',
    'd',
    'f',
    'g',
    'h',
    'j',
    'k',
    'l',
    ';',
    '\'',
    '`',
    0,
    '\\',
    'z',
    'x',
    'c',
    'v',
    'b',
    'n',
    'm',
    ',',
    '.',
    '/',
    0,
    '*',
    0,
    ' ',
    0,
    0,
};

static const char scancode_to_ascii_shift[] = {
    0,
    27,
    '!',
    '@',
    '#',
    '$',
    '%',
    '^',
    '&',
    '*',
    '(',
    ')',
    '_',
    '+',
    '\b',
    '\t',
    'Q',
    'W',
    'E',
    'R',
    'T',
    'Y',
    'U',
    'I',
    'O',
    'P',
    '{',
    '}',
    '\n',
    0,
    'A',
    'S',
    'D',
    'F',
    'G',
    'H',
    'J',
    'K',
    'L',
    ':',
    '"',
    '~',
    0,
    '|',
    'Z',
    'X',
    'C',
    'V',
    'B',
    'N',
    'M',
    '<',
    '>',
    '?',
    0,
    '*',
    0,
    ' ',
    0,
    0,
};

char kgetch(void)
{
    unsigned char scancode = 0;

    while (1)
    {
        if ((inb(0x64) & 1) == 1)
        {
            scancode = inb(0x60);

            if (scancode < 0x80)
            {
                if (scancode == 0x2A || scancode == 0x36)
                {
                    shift_apasat = 1;
                    continue;
                }
                if (scancode == 0x3A)
                {
                    caps_lock_activ = !caps_lock_activ;
                    continue;
                }

                char ascii = 0;
                int vrea_majuscula = shift_apasat ^ caps_lock_activ;

                char caracter_baza = scancode_to_ascii_normal[scancode];
                if (caracter_baza >= 'a' && caracter_baza <= 'z')
                {
                    if (vrea_majuscula)
                    {
                        ascii = scancode_to_ascii_shift[scancode];
                    }
                    else
                    {
                        ascii = caracter_baza;
                    }
                }
                else
                {
                    if (shift_apasat)
                    {
                        ascii = scancode_to_ascii_shift[scancode];
                    }
                    else
                    {
                        ascii = caracter_baza;
                    }
                }

                if (ascii != 0)
                {
                    return ascii;
                }
            }
            else
            {
                if (scancode == 0xAA || scancode == 0xB6)
                {
                    shift_apasat = 0;
                }
            }
        }
    }
}