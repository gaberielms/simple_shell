#ifndef MAIN_H
#define MAIN_H

#define MAX_INPUT_SIZE 1024 // Maximum input size
#define MAX_USERNAME_SIZE 32 // Maximum username size
#define MAX_HISTORY 10 // Maximum history size

#include <unistd.h>
#include <termios.h>
#include "types.h"
#include "commands.h"
#include "parser.h"
#include "executor.h"
#include "utils.h"

void disable_raw_mode(); // Disable raw mode

void enable_raw_mode(); // Enable raw mode

int main(); // Main function

#endif
