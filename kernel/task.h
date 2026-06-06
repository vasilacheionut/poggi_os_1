#ifndef TASK_H
#define TASK_H

void task_init(void);
int task_create(void (*fn)(void));
void task_yield(void);
void task_list(void);

#endif