// drivers/heap.h
#ifndef HEAP_H
#define HEAP_H

// Inițializează heap-ul la o adresă specifică și cu o anumită mărime
void kheap_init(unsigned long start_address, unsigned long size);

// Alocă dinamic un bloc de memorie de mărimea 'size' (în bytes)
void* kmalloc(unsigned long size);

// Eliberează un bloc de memorie alocat anterior
void kfree(void* ptr);

// Comandă utilitară opțională pentru a vedea starea memoriei în Shell
void kheap_dump(void);

#endif