#ifndef POGGI_TASK_H
#define POGGI_TASK_H

#define MAX_TASKS 3

// Stările posibile ale unui task
typedef enum {
    TASK_READY,
    TASK_RUNNING,
    TASK_SLEEPING
} TaskState;

// Structura unui Task Control Block (TCB)
typedef struct {
    int id;
    const char* name;
    void (*task_function)(void); // Pointer către funcția pe care o execută task-ul
    TaskState state;
} PoggiTask;

// Inițializează sistemul de multitasking
void poggi_scheduler_init(void);

// Creează un task nou
int poggi_task_create(const char* name, void (*func)(void));

// Cedează controlul următorului task (Context Switch)
void poggi_yield(void);

// Porneste executia task-urilor în buclă
void poggi_scheduler_start(void);

#endif // POGGI_TASK_H