#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"
#include "util.h"

void delete_list(struct list *l)
{
   struct listnode *iter;

   if (!l)
   {
      return;
   }

   iter = l->head;
   while (iter)
   {
      struct listnode *tmp = iter;
      iter = iter->next;
      free(tmp->str);
      free(tmp);
   }

   l->head = NULL;
   l->tail = NULL;
}

void insert_last(struct list *l, char *str)
{
   struct listnode *node =
      (struct listnode *) safe_malloc(sizeof(struct listnode));
   node->str = str;
   node->next = NULL;

   if (l->head == NULL)
   {
      l->len = 1;
      l->head = l->tail = node;
   }
   else
   {
      l->len++;
      l->tail->next = node;
      l->tail = node;
   }
}
