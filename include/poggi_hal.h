#ifndef POGGI_HAL_H
#define POGGI_HAL_H

void hal_init(void);
void hal_print_char(char c);
void hal_print_string(const char *str);
char hal_get_char(void);
void hal_read_line(char *buffer, int max_len);
int hal_get_uptime(void);
int hal_get_total_memory(void);

#endif // POGGI_HAL_H