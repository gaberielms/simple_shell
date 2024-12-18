#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

// arg struct - linked list
typedef struct arg {
  char *argstr;
  struct arg *next;
  int is_quote;
} arg;

// command struct
typedef struct command {
  char *name;
  arg *args;
  int fd_in;
  int fd_out;
  struct command *next;
} command;

void free_args(arg *head); // Free args linked list

void free_commands(command *head); // Free commands linked list

arg *build_arg(arg *head); // Build arg struct

command *build_command(command *head); // Build command struct

char *find_command(char *command); // Find a command in PATH

char *parse_string(char *args, arg *current); // Parse string return heap allocated string

int build_args(char *args, arg *head); // Build args linked list

void type(arg *head); // Type command implementation

void change_directory(arg *head); // Change directory command implementation

void pwd(); // Print working directory command implementation

void echo(arg *head); // Echo command implementation

int get_fd_in(char *args); // Get input file descriptor

int get_fd_out(char *args); // Get output file descriptor append or truncate

void build_commands(char *args, command *command_head); // Build commands linked list

void redirect_io(command *command_head); // Redirect IO for commands

void execute_command(command *command_head); // Execute commands

int main(); // Main function

#endif
