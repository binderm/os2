#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <unistd.h>
#include "util.h"

void *safe_malloc(size_t size)
{
   void *ptr = malloc(size);
   if (ptr == NULL)
   {
      perror(0);
      exit(-1);
   }

   return ptr;
}

void *safe_realloc(void *ptr, size_t size)
{
   ptr = realloc(ptr, size);
   if (ptr == NULL)
   {
      perror(0);
      exit(-1);
   }

   return ptr;
}

char *safe_strdup(char *orig)
{
   char *str = strdup(orig);
   if (str == NULL)
   {
      perror(0);
      exit(-1);
   }

   return str;
}

int safe_close(int fd) {
	if (fd < 0 || fd == STDIN_FILENO || fd == STDOUT_FILENO) {
		return 0;
	}
	if (close(fd)) {
		fprintf(stderr, "seash: Failed to close file descriptor %d: %s\n", fd, strerror(errno));
		return -1;
	}
	return 0;
}

