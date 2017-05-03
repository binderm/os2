#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
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
