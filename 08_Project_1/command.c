#include "command.h"
#include "list.h"
#include "util.h"
#include <stdlib.h>
#include <stdio.h>

commandlist * new_commandlist()
{
   commandlist *clist = (commandlist *)safe_malloc(sizeof(commandlist));
   clist->head = NULL;
   clist->tail = NULL;

   return clist;
}

command * new_command()
{
   command *tmp = (command *)safe_malloc(sizeof(command));
   tmp->args = (struct list*)safe_malloc(sizeof(struct list));
   tmp->args->len = 0;
   tmp->args->head = tmp->args->tail = NULL;
   tmp->cmd = tmp->in = tmp->out = NULL;
   tmp->next_one = NULL;

   return tmp;
}

void delete_commandlist(commandlist *clist)
{
   command *cur = clist->head;

   while (cur != NULL)
   {
      command *tmp = cur;
      cur = cur->next_one;
      delete_command(tmp);
      free(tmp);
   }
   clist->head = NULL;
}

void delete_command(command *cmd)
{
   if (cmd->cmd)
   {
      free(cmd->cmd);
   }
   if (cmd->in)
   {
      free(cmd->in);
   }
   if (cmd->out)
   {
      free(cmd->out);
   }
   delete_list(cmd->args);
   free(cmd->args);
}

void insert_command(commandlist *clist, command *cmd)
{
   if (clist->head == NULL)
   {
      clist->head = clist->tail = cmd;
   }
   else
   {
      clist->tail->next_one = cmd;
      clist->tail = cmd;
   }
}

void print_commandlist(commandlist *clist)
{
   command *cur = clist->head;

   while (cur != NULL)
   {
      struct listnode *arg;
      printf("Command: %s\n", cur->cmd);
      printf("Args: [\n");
      for (arg = cur->args->head; arg != NULL; arg = arg->next)
      {
         printf("\t%s\n", arg->str);
      }
      printf("      ]\n");
      printf("stdin: %s\n", cur->in ? cur->in : "");
      printf("stdout: %s\n", cur->out ? cur->out : "");

      printf("\n");
      cur = cur->next_one;
   }
}

int valid_commandlist(commandlist *clist)
{
   command *cur = clist->head, *prev;

   if (cur == NULL)
   {
      return 0;
   }

   while (cur->next_one != NULL)
   {
      prev = cur;
      cur = cur->next_one;

      if (prev->out || cur->in)
      {
         if (prev->out)
         {
            fprintf(stderr, "Ambigous output redirect.\n");
         }
         else if (cur->in)
         {
            fprintf(stderr, "Ambigous input redirect.\n");
         }
         return 0;
      }
   }

   return 1;
}
