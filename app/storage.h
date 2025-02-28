#ifndef STORAGE_H
#define STORAGE_H

#define UPARROW 1
#define DOWNARROW 2
#define RIGHTARROW 3
#define LEFTARROW 4

#include "types.h"
#include <termios.h>

void store_command(char *command); // Store command in history
char *get_previous_command(); // Get previous command from history
char *get_next_command(); // Get next command from history

#endif