#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include "command.h"
#include "getcommand.h"
#include "util.h"
#include "cd.h"
#include "signal_handling.h"
#include "execute_commandlist.h"

#define PROMPT "->"
#define DEBUG 0

static int iscd(commandlist *);

int main(void)
{
   if (setup_signal_handling()) {
	   fprintf(stderr, "seash: Failed to set up signal handling\n");
	   return -1;
   }
   commandlist *clist;

   while (1)
   {
      printf("%s ", PROMPT);
      fflush(stdout);
      clist = getcommandlist(stdin);
      if (clist == NULL)
      {
         if (feof(stdin))
         {
            break;
         }
      }
      else
      {
#if DEBUG
         print_commandlist(clist);
#endif
         if (valid_commandlist(clist))
         {
            if (iscd(clist))
            {
               // Handle changing directories
	       cd(clist->head);
            }
            else
            {
               // Execute the command list
	       execute_commandlist(clist);
            }
         }
         delete_commandlist(clist);
         free(clist);
      }
   }
   return 0;
}

static int iscd(commandlist *clist)
{
   return clist != NULL && clist->head != NULL && clist->head->cmd != NULL
      && (strcmp(clist->head->cmd, "cd") == 0);
}
