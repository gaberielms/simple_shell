#include "parser.h"

void free_args(arg *head) {
  arg *current = head;
  while (current != NULL) {
    arg *next = current->next;
    free(current->argstr);
    free(current);
    current = next;
  }
}

void free_commands(command *head) {
  command *current = head;
  while (current != NULL) {
    command *next = current->next;
    free(current->name);
    free_args(current->args);
    free(current);
    current = next;
  }
}

arg *build_arg(arg *head) {
  arg *new_arg = malloc(sizeof(arg));
  if (new_arg == NULL) {
    perror("Failed to allocate memory for new_arg\n");
    free_args(head);
    exit(1);
  }
  new_arg->argstr = NULL;
  new_arg->next = NULL;
  new_arg->is_quote = 0; 
  return new_arg;
}

command *build_command(command *head) {
  command *new_command = malloc(sizeof(command));
  if (new_command == NULL) {
    perror("Failed to allocate memory for new_command\n");
    free_commands(head);
    exit(1);
  }
  new_command->name = NULL;
  new_command->args = build_arg(NULL);
  new_command->currfd = 0;
  new_command->fd_in = 0;
  new_command->fd_out = 1;
  new_command->stderr_out = 0;
  new_command->next = NULL;
  return new_command;
}

int build_args(char *args, arg *head, command *command_head) {
  arg *current = head;
  while (*args == ' ') { // Skip whitespace
    args++;
  }
  while (*args != '\0') {
    // leave loop if special character is found
    if (*args == '|') {
      return 1;
    } else if (*args == '<') {
      return 2;
    } else if (*args == '>') {
      return 3;
    }
    current->argstr = parse_string(&args, current, command_head);
    if (current->argstr == NULL) {
      perror("Failed to allocate memory for argstr\n");
      free_args(head);
      exit(1);
    }
    while (*args == ' ') { // Skip whitespace
      args++;
    }
    // Build next arg
    if (*args != '\0') {
      current->next = build_arg(head);
      current = current->next;
    }
  }
  current = head;
  while (current->next != NULL) {
    if (current->argstr[0] == '\0') {
      arg *temp = current;
      current = current->next;
      free(temp->argstr);
      free(temp);
      head = current;
    } else {
      current = current->next;
    }
  }
  return 0;
}

void build_commands(char *args, command *command_head) {
  command *current_command = command_head;
  int i = build_args(args, current_command->args, current_command);
  while (i != 0) {
    switch (i) {
      case 1: // pipe
        current_command->name = strdup(current_command->args->argstr);
        if (current_command->name == NULL) {
          perror("Failed to allocate memory for current_command->name\n");
          free_commands(command_head);
          exit(1);
        }
        if (current_command->args->next->argstr) {
          arg *temp = current_command->args;
          current_command->args = current_command->args->next;
          free(temp->argstr);
          free(temp);
          arg *curr = current_command->args;
          arg *prev = NULL;
          while (curr) {
            if (curr->argstr) {
              prev = curr;
              curr = curr->next;
            } else {
              free(curr);
              if (prev) {
                prev->next = NULL;
              }
              break;
            }
          }
        } else {
          free_args(current_command->args);
          current_command->args = NULL;
        }
        current_command->next = build_command(command_head);
        current_command = current_command->next;
        while (*args != '|') { // Skip to |
          args++;
        }
        if (*args == '|') {
          args++;
        }
        while (*args == ' ') { // Skip whitespace
          args++;
        }
        i = build_args(args, current_command->args, current_command);
        break;
      case 2: // input redirection
        current_command->fd_in = get_fd_in(&args); // error checking in get_fd_in
        if (*args == '\0') {
          i = 0;
          break;
        }
        while (*args == ' ') { // Skip whitespace
          args++;
        }
        i = build_args(args, current_command->args, current_command);
        break;
      case 3: // output redirection
        current_command->fd_out = get_fd_out(args); // error checking in get_fd_out
        i = 0;
        break;
      default:
        perror("error: invalid argument return value\n");
        return;
    }
  }
  current_command->name = strdup(current_command->args->argstr);
  if (current_command->name == NULL) {
    perror("Failed to allocate memory for current_command->name\n");
    free_commands(command_head);
    exit(1);
  }
  arg *temp = current_command->args;
  while (temp != NULL && temp->next != NULL) {
    if (temp->next->argstr != NULL && temp->next->argstr[0] == '\0') {
      free(temp->next->argstr);
      arg *to_free = temp->next;
      temp->next = temp->next->next;
      free(to_free);
      break;
    }
    temp = temp->next;
  }
  if (current_command->args->next) {
    arg *temp = current_command->args;
    current_command->args = current_command->args->next;
    free(temp->argstr);
    free(temp);
    arg *curr = current_command->args;
    arg *prev = NULL;
    while (curr) {
      if (curr->argstr) {
        prev = curr;
        curr = curr->next;
      } else {
        free(curr->argstr);
        free(curr);
        if (prev) {
          prev->next = NULL;
        } else {
          current_command->args = NULL;
        }
        break;
      }
    }
  } else {
    free(current_command->args->argstr);
    free(current_command->args);
    current_command->args = NULL;
  }
}
