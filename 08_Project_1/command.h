#ifndef COMMAND
#define COMMAND

#include "list.h"

typedef struct com
{
   char *cmd;
   char *in;
   char *out;
   struct list *args;
   struct com * next_one;
} command;

typedef struct comlist
{
   struct com * head;
   struct com * tail;
} commandlist;

void insert_command(commandlist *, command *cmd);
commandlist * new_commandlist();
command * new_command();
void delete_commandlist(commandlist *);
void delete_command(command *);
void print_commandlist(commandlist *);
int valid_commandlist(commandlist *);

#endif

