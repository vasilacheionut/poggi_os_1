#include "../include/string.h"

int kstrlen(const char *s)
{
    int len = 0;
    while (s[len])
        len++;
    return len;
}

char *kstrchr(const char *s, int c)
{
    while (*s)
    {
        if (*s == (char)c)
            return (char *)s;
        s++;
    }
    return (c == '\0') ? (char *)s : 0;
}

void *kmemcpy(void *dest, const void *src, int n)
{
    char *d = dest;
    const char *s = src;
    for (int i = 0; i < n; i++)
        d[i] = s[i];
    return dest;
}

void *kmemset(void *s, int c, int n)
{
    char *p = s;
    for (int i = 0; i < n; i++)
        p[i] = (char)c;
    return s;
}

int kmemcmp(const void *s1, const void *s2, int n)
{
    const char *a = s1, *b = s2;
    for (int i = 0; i < n; i++)
    {
        if (a[i] != b[i])
            return (a[i] - b[i]);
    }
    return 0;
}

char *kitoa(int num, int base)
{
    static char buf[32];
    char *p = buf + 31;
    *p = '\0';
    if (num == 0)
    {
        *(--p) = '0';
        return p;
    }
    int neg = (num < 0 && base == 10);
    if (neg)
        num = -num;
    while (num)
    {
        *(--p) = "0123456789ABCDEF"[num % base];
        num /= base;
    }
    if (neg)
        *(--p) = '-';
    return p;
}