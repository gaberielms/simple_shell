#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "types.h"
#include "commands.h"

void redirect_io(command *command_head); // Redirect IO for commands

void execute_command(command *command_head); // Execute commands

#endif