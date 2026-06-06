#ifndef STRING_H
#define STRING_H

int kstrlen(const char *s);
char *kstrchr(const char *s, int c);
void *kmemcpy(void *dest, const void *src, int n);
void *kmemset(void *s, int c, int n);
int kmemcmp(const void *s1, const void *s2, int n);
char *kitoa(int num, int base);

// Comode pentru utilizare
#define strlen kstrlen
#define strchr kstrchr
#define memcpy kmemcpy
#define memset kmemset
#define memcmp kmemcmp
#define itoa kitoa

#endif