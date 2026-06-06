// kernel/task.c
#include "task.h"
#include "../drivers/vga.h"
#include "../include/string.h"
#include <stdint.h>

// Declară funcțiile externe (din heap)
extern void *kmalloc(int size);
extern void kfree(void *ptr);

#define MAX_TASKS 16
#define TASK_RUNNING 0
#define TASK_READY 1
#define TASK_WAITING 2

typedef struct
{
    void *stack;
    uint64_t rsp;
    uint64_t rbp;
    int state;
    int id;
} task_t;

static task_t tasks[MAX_TASKS];
static int current_task = 0;
static int next_task_id = 1;

void task_init(void)
{
    for (int i = 0; i < MAX_TASKS; i++)
    {
        tasks[i].stack = 0;
        tasks[i].state = TASK_WAITING;
        tasks[i].id = -1;
    }
    tasks[0].stack = 0;
    tasks[0].state = TASK_RUNNING;
    tasks[0].id = 0;
    current_task = 0;
    kprint("Task manager initializat.\n");
}

int task_create(void (*fn)(void))
{
    int id = -1;
    for (int i = 1; i < MAX_TASKS; i++)
    {
        if (tasks[i].stack == 0)
        {
            id = i;
            break;
        }
    }
    if (id == -1)
    {
        kprint("Eroare: prea multe task-uri!\n");
        return -1;
    }
    tasks[id].stack = (void *)kmalloc(8192);
    if (!tasks[id].stack)
    {
        kprint("Eroare: nu se poate aloca stiva pentru task!\n");
        return -1;
    }
    uint64_t *stack_top = (uint64_t *)((uint64_t)tasks[id].stack + 8192);
    *(--stack_top) = (uint64_t)fn;
    *(--stack_top) = 0;
    tasks[id].rsp = (uint64_t)stack_top;
    tasks[id].rbp = (uint64_t)stack_top;
    tasks[id].state = TASK_READY;
    tasks[id].id = next_task_id++;
    kprint("Task creat cu ID ");
    kprint(itoa(tasks[id].id, 10));
    kprint("\n");
    return tasks[id].id;
}

void task_yield(void)
{
    __asm__ volatile(
        "mov %%rsp, %0\n"
        "mov %%rbp, %1\n"
        : "=m"(tasks[current_task].rsp), "=m"(tasks[current_task].rbp));
    int next = (current_task + 1) % MAX_TASKS;
    int found = 0;
    for (int i = 0; i < MAX_TASKS; i++)
    {
        if (tasks[next].state == TASK_READY)
        {
            found = 1;
            break;
        }
        next = (next + 1) % MAX_TASKS;
    }
    if (!found)
        return;
    tasks[current_task].state = TASK_READY;
    tasks[next].state = TASK_RUNNING;
    current_task = next;
    __asm__ volatile(
        "mov %0, %%rsp\n"
        "mov %1, %%rbp\n"
        :
        : "r"(tasks[current_task].rsp), "r"(tasks[current_task].rbp));
}

void task_list(void)
{
    kprint("Task list:\n");
    for (int i = 0; i < MAX_TASKS; i++)
    {
        if (tasks[i].stack != 0 || i == 0)
        {
            kprint("  ID ");
            kprint(itoa(tasks[i].id, 10));
            kprint(": ");
            if (tasks[i].state == TASK_RUNNING)
                kprint("RUNNING");
            else if (tasks[i].state == TASK_READY)
                kprint("READY");
            else if (tasks[i].state == TASK_WAITING)
                kprint("WAITING");
            else
                kprint("INACTIVE");
            if (i == 0)
                kprint(" (kernel)");
            kprint("\n");
        }
    }
}