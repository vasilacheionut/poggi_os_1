// kernel/task.c
#include "task.h"
#include "../drivers/vga.h"

// Declarăm funcția din asamblare
extern void switch_context(unsigned long *old_rsp, unsigned long new_rsp);

TaskControlBlock task_table[MAX_TASKS];
int current_task_idx = 0;
int total_tasks = 0;

// Inițializăm tabela (Task 0 este chiar Kernel Main)
void task_init(void)
{
    for (int i = 0; i < MAX_TASKS; i++)
    {
        task_table[i].id = i;
        task_table[i].state = 0; // State: Free
        task_table[i].rsp = 0;
    }

    // Înregistrăm Kernel Main drept Task 0
    task_table[0].state = 2; // Running
    total_tasks = 1;
    current_task_idx = 0;
}

// Creează un task nou și îi pregătește stiva ca și cum ar fi fost suspendat
void task_create(void (*entry_point)(void))
{
    if (total_tasks >= MAX_TASKS)
        return;

    int idx = total_tasks;
    TaskControlBlock *t = &task_table[idx];

    t->state = 1; // Ready

    // Configurăm vârful stivei (stiva crește în jos în memorie)
    unsigned long *stack_top = (unsigned long *)&t->stack[STACK_SIZE];

    // Simulăm stiva în momentul unui switch:
    // Când noul task va primi prima dată 'ret', va sări direct la entry_point
    stack_top--;
    *stack_top = (unsigned long)entry_point; // RIP initial

    // Adăugăm spațiu gol pentru cele 15 registre pe care 'switch_context' le va face POP
    for (int i = 0; i < 15; i++)
    {
        stack_top--;
        *stack_top = 0; // Inițializăm registrele simulate cu 0
    }

    t->rsp = (unsigned long)stack_top;
    total_tasks++;
}

// Schimbă manual și de bunăvoie execuția către următorul task disponibil
void task_yield(void)
{
    int old_idx = current_task_idx;
    int next_idx = (current_task_idx + 1) % total_tasks;

    // Căutăm următorul task gata de rulare
    while (task_table[next_idx].state != 1 && task_table[next_idx].state != 2)
    {
        next_idx = (next_idx + 1) % total_tasks;
    }

    if (next_idx == old_idx)
        return; // Nu avem alt task disponibil

    // Actualizăm stările
    if (task_table[old_idx].state == 2)
    {
        task_table[old_idx].state = 1; // Din Running devine Ready
    }
    task_table[next_idx].state = 2; // Noul task devine Running
    current_task_idx = next_idx;

    // Apelăm funcția hardware din asamblare pentru switch-ul fizic
    switch_context(&task_table[old_idx].rsp, task_table[next_idx].rsp);
}