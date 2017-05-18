#ifndef PIPING_H
#define PIPING_H

#include "command.h"

#define PIPELINE_INTERMEDIATE 0x0
#define PIPELINE_START 0x1
#define PIPELINE_END 0x2
#define IS_PIPELINE_START(command_location) ((command_location & PIPELINE_START) == PIPELINE_START)
#define IS_PIPELINE_END(command_location) ((command_location & PIPELINE_END) == PIPELINE_END)

int redirect(int, int);

/**
 * Setup the streams for the current and next command.
 *
 * @param com the current command
 * @param command_location the location of the command within the pipeline
 * @param in the file descriptor from which the output of the previous command can be read, ignored if this is the first command in the pipeline
 * @param out afterwards the file descriptor to which the output of the current command should be wrote
 * @param next_in the file descriptor from which the next command can read the output of the current command
 * @return 0 if setup was successful, != 0 otherwise
 */
int setup_piping(command *com, int command_location, int *in, int *out, int *next_in);

#endif

