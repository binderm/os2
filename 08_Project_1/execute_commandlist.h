#ifndef EXECUTE_COMMANDLIST_H
#define EXECUTE_COMMANDLIST_H

#include "command.h"

int register_sigint_handler();
void execute_commandlist(commandlist *);

#endif
