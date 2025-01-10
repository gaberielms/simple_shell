#ifndef PARSER_H
#define PARSER_H

#include "types.h"
#include "utils.h"

void free_args(arg *head); // Free args linked list

void free_commands(command *head); // Free commands linked list

arg *build_arg(arg *head); // Build arg struct

command *build_command(command *head); // Build command struct

int build_args(char *args, arg *head, command *command_head); // Build args linked list

void build_commands(char *args, command *command_head); // Build commands linked list

#endif