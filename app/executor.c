#include "executor.h"

void redirect_io(command *command) {
  if (command->stderr_out) {
    dup2(command->fd_out, STDERR_FILENO);
    close(command->fd_out);
  }
  else if (command->fd_out != STDOUT_FILENO) {
    dup2(command->fd_out, STDOUT_FILENO);
    close(command->fd_out);
  }
  if (command->fd_in != STDIN_FILENO) {
    dup2(command->fd_in, STDIN_FILENO);
    close(command->fd_in);
  }
}

void execute_command(command *command_head) {
  redirect_io(command_head);
  if (strcmp(command_head->name, "echo") == 0) {
    echo(command_head->args);
    exit(0);
  } else if (strcmp(command_head->name, "pwd") == 0) {
    pwd();
    exit(0);
  } else if (strcmp(command_head->name, "type") == 0) {
    type(command_head->args);
    exit(0);
  } else if (find_command(command_head->name) == NULL) {
    fprintf(stderr, "%s: command not found\n", command_head->name);
    exit(0);
  }
  int argc = 0;
  arg *current = command_head->args;
  while (current != NULL) {
    argc++;
    current = current->next;
  }
  char *argv[argc + 2];
  argv[0] = command_head->name;
  current = command_head->args;
  int i = 1;
  while (current != NULL) {
    argv[i++] = current->argstr;
    current = current->next;
  }
  argv[i] = NULL;
  execvp(command_head->name, argv);
  fprintf(stderr, "Failed to execute %s\n", command_head->name);
  exit(1);
}
