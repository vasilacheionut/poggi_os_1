#ifndef POGGI_MEM_H
#define POGGI_MEM_H

void poggi_mem_init(void);
void* poggi_alloc(int size);
int poggi_mem_used(void);
int poggi_mem_free(void);

#endif // POGGI_MEM_H