#ifndef ARGUMENTS_H
#define ARGUMENTS_H

#include "command.h"

/**
 * Extract the argument values from a command.
 * The first element in the returned array is the path to the executable of the command.
 * Followed by the actual arguments. The array is NULL-terminated.
 * --> returned array has size of argument_count + 2
 *
 * @return array of argument values (dynamically allocated!)
 */
char **get_argv(command *);

#endif
