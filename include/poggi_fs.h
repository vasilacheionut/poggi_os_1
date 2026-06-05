#ifndef POGGI_FS_H
#define POGGI_FS_H

#define MAX_FILES 10
#define MAX_FILENAME 32
#define MAX_FILE_SIZE 256

typedef struct
{
    char name[MAX_FILENAME];
    char content[MAX_FILE_SIZE];
    int size;
    int is_used;
} PoggiFile;

void poggi_fs_init(void);
void poggi_fs_list(void);
void poggi_fs_read(const char *name);
int poggi_fs_create(const char *name, const char *content);

#endif // POGGI_FS_H