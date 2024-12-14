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
    fprintf(stderr, "Failed to allocate memory for new_arg\n");
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
    fprintf(stderr, "Failed to allocate memory for new_command\n");
    free_commands(head);
    exit(1);
  }
  new_command->name = NULL;
  new_command->args = build_arg(NULL);
  new_command->fd_in = 0;
  new_command->fd_out = 1;
  new_command->next = NULL;
  return new_command;
}

void print_args(arg *head) {
  arg *current = head;
  int i = 0;
  printf("[\n");
  while (current != NULL) {
    printf("  %d: %s\n", i++, current->argstr);
    current = current->next;
  }
  printf("]\n");
}

char *find_command(char *command) {
  char *path = getenv("PATH");
  char *path_copy = strdup(path);
  if (path_copy == NULL) {
    fprintf(stderr, "Failed to allocate memory for path_copy\n");
    return NULL;
  }
  char *dir = strtok(path_copy, ":");
  while (dir != NULL) {
    char *full_path = malloc(strlen(dir) + strlen(command) + 2);
    if (full_path == NULL) {
      fprintf(stderr, "Failed to allocate memory for full_path\n");
      free(path_copy);
      return NULL;
    }    
    strcpy(full_path, dir);
    strcat(full_path, "/");
    strcat(full_path, command);
    if (access(full_path, F_OK) == 0) {
      free(path_copy);
      return full_path;
    }
    free(full_path);
    dir = strtok(NULL, ":");
  }
  free(path_copy);
  return NULL;
}

char *parse_string(char *args, arg *current) {
  char buffer[1024];
  int len = 0;
  int in_quotes = 0;
  int in_double_quotes = 0;
  // Collect the argument characters
  while (*args != '\0' && (*args != ' ' || in_double_quotes || in_quotes)) {
    if (*args == '"' && !in_double_quotes && !in_quotes) {
      in_double_quotes = 1;
      args++;
    } else if (*args == '\'' && !in_quotes && !in_double_quotes) {
      in_quotes = 1;
      args++;
    } else if (*args == '"' && in_double_quotes) {
      in_double_quotes = 0;
      args++;
    } else if (*args == '\'' && in_quotes) {
      in_quotes = 0;
      args++;
    } else if (*args == '\\' && in_double_quotes) {
      args++;
      if (*args == '\0') {
        break;
      }
      if (*args == '\\' || *args == '"' || *args == '$' || *args == '\n') {
        buffer[len++] = *args++;
      } else {
        buffer[len++] = '\\';
        buffer[len++] = *args++;
      }
    } else if (*args == '\\' && (!in_double_quotes && !in_quotes)) {
      args++;
      if (*args == '\0') {
        break;
      }
      buffer[len++] = *args++;
    } else if ((*args == '|' || *args == '<' || *args == '>') && !in_double_quotes && !in_quotes) {
      break;
    } else {
      buffer[len++] = *args++;
    }
  }
  buffer[len] = '\0';
  current->is_quote = in_quotes || in_double_quotes;
  return strdup(buffer);
}

int build_args(char *args, arg *head) {
  arg *current = head;
  while (*args == ' ') { // Skip whitespace
    args++;
  }
  int i = 1;
  while (*args != '\0') {
    // leave loop if special character is found
    if (*args == '|' || *args == '<' || *args == '>') {
      return i;
    } 
    // Assign the argument string
    current->argstr = parse_string(args, current);
    if (current->argstr == NULL) {
      fprintf(stderr, "Failed to allocate memory for argstr\n");
      free_args(head);
      exit(1);
    }
    args += strlen(current->argstr);
    while (*args == ' ') { // Skip whitespace
      args++;
    }
    // Build next arg
    if (*args != '\0') {
      current->next = build_arg(head);
      current = current->next;
    }
    i++;
  }
  return 0;
}

void type(arg *head) {
  if (head == NULL) {
    printf("type: too few arguments\n");
    return;
  }
  char *curarg = head->argstr;
  char *command = find_command(curarg);
  if (strcmp(curarg, "cd") == 0) {
    printf("cd is a shell builtin\n");
  } else if (strcmp(curarg, "pwd") == 0) {
    printf("pwd is a shell builtin\n");
  } else if (strcmp(curarg, "echo") == 0) {
    printf("echo is a shell builtin\n");
  } else if (strcmp(curarg, "type") == 0) {
    printf("type is a shell builtin\n");
  } else if (strcmp(curarg, "exit") == 0) {
    printf("exit is a shell builtin\n");
  } else if (command != NULL) {
    printf("%s is %s\n", curarg, command);
    free(command);
  } else {
    printf("%s: not found\n", curarg);
  }
}

void change_directory(arg *head) {
  if (head == NULL) {
    char *home = getenv("HOME");
    if (chdir(home) != 0) {
      printf("cd: %s: No such file or directory\n", home);
    }
  }
  else if (strchr(head->argstr, '~') != NULL) {
    char *home = getenv("HOME");
    char *new_arg = malloc(strlen(home) + strlen(head->argstr) + 1);
    if (new_arg == NULL) {
      fprintf(stderr, "Failed to allocate memory for new_arg\n");
      free_args(head);
      exit(1);
    }
    strcpy(new_arg, home);
    strcat(new_arg, head->argstr + 1);
    if (chdir(new_arg) != 0) {
      printf("cd: %s: No such file or directory\n", new_arg);
    }
    free(new_arg);
  } else if (chdir(head->argstr) != 0) {
    printf("cd: %s: No such file or directory\n", head->next->argstr);
  }
}

void pwd() {
  char cwd[100];
  if (getcwd(cwd, sizeof(cwd)) != NULL) {
    printf("%s\n", cwd);
  } else {
    printf("Failed to get current working directory\n");
  }
}

void echo(arg *head) {
  arg *current = head;
  while (current != NULL) {
    printf("%s", current->argstr);
    current = current->next;
    if (current != NULL) {
      printf(" ");
    }
  }
  printf("\n");
}

int get_fd_in(char *args) {
  int fd = 0;
  args++;
  while (*args == ' ') { // Skip whitespace
    args++;
  }
  if (*args == '\0') {
    return -1;
  }
  char *file_name = parse_string(args, NULL);
  if (file_name == NULL) {
    fprintf(stderr, "Failed to allocate memory for file_name\n");
    exit(1);
  }
  if (access(file_name, F_OK) != 0) {
    printf("%s: No such file or directory\n", file_name);
    free(file_name);
    return -1;
  }
  fd = open(file_name, O_RDONLY);
  if (fd == -1) {
    printf("Failed to open %s\n", file_name);
    free(file_name);
    return -1;
  }
  free(file_name);
  return fd;
}

int get_fd_out(char *args) {
  int fd = 0;
  args++;
  while (*args == ' ') { // Skip whitespace
    args++;
  }
  if (*args == '\0') {
    return -1;
  }
  char *file_name = parse_string(args, NULL);
  if (file_name == NULL) {
    fprintf(stderr, "Failed to allocate memory for file_name\n");
    exit(1);
  }
  fd = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd == -1) {
    printf("Failed to open %s\n", file_name);
    free(file_name);
    return -1;
  }
  free(file_name);
  return fd;
}

int main() {
  while (1) {
    printf("$ ");
    fflush(stdout);
    // Wait for user input
    char input[1024];
    if (fgets(input, 1024, stdin) == NULL) {
      break; // EOF
    }
    input[strlen(input) - 1] = '\0';
    char *args = input;
    // Build command linked list
    command *command_head = build_command(NULL);
    command *current_command = command_head;
    int i = build_args(args, current_command->args);
    while (i != 0) {
      printf("i: %d\n", i);
      if (i == 1) {
        printf("input may not start with a pipe or redirection\n");
        break;
      } else if (i == 2 && args[0] == '>') {
        printf("input may not start with a redirection\n");
        break;
      } else if (args[0] == '|') { // pipe
        current_command->name = current_command->args->argstr;
        arg *temp = current_command->args;
        current_command->args = current_command->args->next;
        free(temp->argstr);
        free(temp);
        current_command->next = build_command(command_head);
        current_command = current_command->next;
        args++;
        i = build_args(args, current_command->args);
      } else if (args[0] == '<') { // input redirection
        current_command->name = current_command->args->argstr;
        arg *temp = current_command->args;
        current_command->args = current_command->args->next;
        free(temp->argstr);
        free(temp);
        current_command->fd_in = get_fd_in(args);
        if (current_command->fd_in == -1) {
          break;
        }
        current_command->next = build_command(command_head);
        current_command = current_command->next;
        i = build_args(args, current_command->args);
      } else if (args[0] == '>') { // output redirection
        current_command->name = current_command->args->argstr;
        arg *temp = current_command->args;
        current_command->args = current_command->args->next;
        free(temp->argstr);
        free(temp);
        current_command->fd_out = get_fd_out(args);
        if (current_command->fd_out == -1) {
          break;
        }
        current_command->next = build_command(command_head);
        current_command = current_command->next;
        i = build_args(args, current_command->args);
      }
    }
    if (i == 0) {
      current_command->name = current_command->args->argstr;
      arg *temp = current_command->args;
      current_command->args = current_command->args->next;
      free(temp);
    } else {
      free_commands(command_head);
      continue;
    }
    // BUILTIN COMMANDS
    if (strcmp(command_head->name, "exit") == 0) {
      free_commands(command_head);
      exit(0);
    } else if (strcmp(command_head->name, "cd") == 0) {
      change_directory(command_head->args);
    } else if (strcmp(command_head->name, "echo") == 0) {
      echo(command_head->args);
    } else if (strcmp(command_head->name, "pwd") == 0) {
      pwd();
    } else if (strcmp(command_head->name, "type") == 0) {
      type(command_head->args);
    }
    // EXTERNAL COMMANDS
    else if (input[0] != '\0') {
      // Fork and exec external command
      char *task = find_command(command_head->name);
      if (task != NULL) {
        pid_t pid = fork();
        if (pid == 0) {
          int argc = 0;
          arg *current = command_head->args;
          while (current != NULL) {
            argc++;
            current = current->next;
          }
          char *argv[argc + 2];
          argv[0] = task;
          current = command_head->args;
          int i = 1;
          while (current != NULL) {
            argv[i++] = current->argstr;
            current = current->next;
          }
          argv[i] = NULL;
          execv(task, argv);
          fprintf(stderr, "Failed to execute %s\n", task);
          free_commands(command_head);
          free(task);
          exit(2);
        } else if (pid > 0){
          int status;
          waitpid(pid, &status, 0);
          free(task);
        } else {
          // fork fails
          fprintf(stderr, "Failed to fork\n");
          free_commands(command_head);
          free(task);
          exit(3);
        }
      } else {
        printf("%s: command not found\n", input);
      }
    } else {
      // Should never reach this point
      fprintf(stderr, "error: unknown error\n");
      exit(99);
    }
    free_commands(command_head);
  }
  return 0;
}
