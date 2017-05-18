#include <stdlib.h>

void *safe_malloc(size_t size);
void *safe_realloc(void *ptr, size_t size);
char *safe_strdup(char *);
int safe_close(int fd);
