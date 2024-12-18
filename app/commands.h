#ifndef COMMANDS_H
#define COMMANDS_H

#include "types.h"
#include "utils.h"
#include "parser.h"

void type(arg *head); // Type command implementation

void change_directory(arg *head); // Change directory command implementation

void pwd(); // Print working directory command implementation

void echo(arg *head); // Echo command implementation

#endif