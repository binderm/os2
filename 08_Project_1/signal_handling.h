#ifndef SIGINT_HANDLER_H
#define SIGINT_HANDLER_H

/**
 * Setup signal handling.
 * SIGINT is forwarded to all child processes but does not cause the shell to terminate.
 * 
 * @return 0 if signal handling has been setup correctly, otherwise < 0 and an error message is printed to stderr
 */
int setup_signal_handling();

/**
 * Reset signal handling to default behaviour.
 *
 * @return 0 if reset was successful, != 0 otherwise
 */
int reset_signal_handling();

/**
 * Block a single signal.
 *
 * @param signal the number of the signal to be blocked
 * @return 0 if blocking the signal was successful, != 0 otherwise
 */
int block(int);

/**
 * Unblock a single signal.
 *
 * @param signal the number of the signal to be unblocked
 * @return 0 if unblocking the signal was successful, != 0 otherwise
 */
int unblock(int);

#endif
