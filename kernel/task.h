// kernel/task.h
#ifndef TASK_H
#define TASK_H

#define MAX_TASKS 4
#define STACK_SIZE 4096

// Structură care mapează ordinea în care salvăm registrele pe stivă
typedef struct {
    unsigned long r15, r14, r13, r12, r11, r10, r9, r8;
    unsigned long rbp, rdi, rsi, rdx, rcx, rbx, rax;
    unsigned long rip, cs, rflags, rsp, ss;
} __attribute__((packed)) CpuContext;

// Task Control Block (TCB)
typedef struct {
    int id;
    int state;               // 0 = Liber, 1 = Gata de rulare (Ready), 2 = Rulează (Running)
    unsigned long rsp;       // Pointerul salvat către stiva proprie a task-ului
    unsigned char stack[STACK_SIZE]; // Stiva fizică alocată acestui task
} TaskControlBlock;

// Funcțiile expuse ale planificatorului
void task_init(void);
void task_create(void (*entry_point)(void));
void task_yield(void);

#endif