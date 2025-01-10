#ifndef UTILS_H
#define UTILS_H

#define MAX_INPUT_SIZE 1024 // Maximum input size

#include "types.h"
#include <ctype.h>

char *find_command(char *command); // Find a command in PATH

char *parse_string(char **args, arg *current, command *command_head); // Parse string return heap allocated strin

int get_fd_in(char **args); // Get input file descriptor

int get_fd_out(char *args); // Get output file descriptor append or truncate

#endif