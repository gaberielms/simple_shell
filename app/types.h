#ifndef TYPES_H
#define TYPES_H

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
  int currfd;
  int fd_in;
  int fd_out;
  int stderr_out;
  struct command *next;
} command;

#endif
