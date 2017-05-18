#ifndef SIGINT_HANDLER_H
#define SIGINT_HANDLER_H

/**
 * Setup signal handling.
 * SIGINT is forwarded to all child processes but does not cause the shell to terminate.
 * 
 * @return 0 if signal handling has been setup correctly, otherwise < 0 and an error message is printed to stderr
 */
int setup_signal_handling();

#endif
