#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

// arg struct - linked list
typedef struct arg {
  char *argstr;
  struct arg *next;
  int is_quote;
} arg;

void free_args(arg *head) {
  arg *current = head;
  while (current != NULL) {
    arg *next = current->next;
    free(current->argstr);
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

void build_args(char *args, arg *head) {
  arg *current = head;
  // Skip leading whitespace
  while (*args == ' ') {
    args++;
  }
  // Build arg linked list
  while (*args != '\0') {
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
      } else {
        buffer[len++] = *args++;
      }
    }
    buffer[len] = '\0';
    // Assign the argument string
    current->argstr = strdup(buffer);
    if (current->argstr == NULL) {
      fprintf(stderr, "Failed to allocate memory for argstr\n");
      free_args(head);
      exit(1);
    }
    current->is_quote = in_quotes || in_double_quotes;
    in_double_quotes = 0;
    in_quotes = 0;
    // Skip trailing whitespace
    while (*args == ' ') {
      args++;
    }
    // Build next arg
    if (*args != '\0') {
      current->next = build_arg(head);
      current = current->next;
    }
  }
}

void type(arg *head) {
  if (head->next == NULL) {
    printf("type: too few arguments\n");
    free_args(head);
    return;
  }
  char *curarg = head->next->argstr;
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
  if (head->next == NULL) {
    char *home = getenv("HOME");
    if (chdir(home) != 0) {
      printf("cd: %s: No such file or directory\n", home);
    }
  }
  else if (strchr(head->next->argstr, '~') != NULL) {
    char *home = getenv("HOME");
    char *new_arg = malloc(strlen(home) + strlen(head->argstr) + 1);
    if (new_arg == NULL) {
      fprintf(stderr, "Failed to allocate memory for new_arg\n");
      free_args(head);
      exit(1);
    }
    strcpy(new_arg, home);
    strcat(new_arg, head->next->argstr + 1);
    if (chdir(new_arg) != 0) {
      printf("cd: %s: No such file or directory\n", new_arg);
    }
    free(new_arg);
  } else if (chdir(head->next->argstr) != 0) {
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
  arg *current = head->next;
  while (current != NULL) {
    printf("%s", current->argstr);
    current = current->next;
    if (current != NULL) {
      printf(" ");
    }
  }
  printf("\n");
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
    // Build arg linked list
    arg *head = build_arg(NULL);
    build_args(args, head);
    // BUILTIN COMMANDS
    if (strcmp(head->argstr, "exit") == 0) {
      free_args(head);
      exit(0);
    } else if (strcmp(head->argstr, "cd") == 0) {
      change_directory(head);
    } else if (strcmp(head->argstr, "echo") == 0) {
      echo(head);
    } else if (strcmp(head->argstr, "pwd") == 0) {
      pwd();
    } else if (strcmp(head->argstr, "type") == 0) {
      type(head);
    }
    // EXTERNAL COMMANDS
    else if (input[0] != '\0') {
      // Fork and exec external command
      char *command = find_command(head->argstr);
      if (command != NULL) {
        pid_t pid = fork();
        if (pid == 0) {
          int argc = 0;
          arg *current = head->next;
          while (current != NULL) {
            argc++;
            current = current->next;
          }
          char *argv[argc + 2];
          argv[0] = command;
          current = head->next;
          int i = 1;
          while (current != NULL) {
            argv[i++] = current->argstr;
            current = current->next;
          }
          argv[i] = NULL;
          execv(command, argv);
          fprintf(stderr, "Failed to execute %s\n", command);
          free_args(head);
          free(command);
          exit(2);
        } else if (pid > 0){
          int status;
          waitpid(pid, &status, 0);
          free(command);
        } else {
          // fork fails
          fprintf(stderr, "Failed to fork\n");
          free_args(head);
          free(command);
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
    free_args(head);
  }
  return 0;
}
