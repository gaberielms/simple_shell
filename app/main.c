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

char *find_command(char *command) {
  char *path = getenv("PATH");
  char *path_copy = strdup(path);
  if (path_copy == NULL) {
    printf("Failed to allocate memory for path_copy\n");
    return NULL;
  }
  char *dir = strtok(path_copy, ":");
  while (dir != NULL) {
    char *full_path = malloc(strlen(dir) + strlen(command) + 2);
    if (full_path == NULL) {
      printf("Failed to allocate memory for full_path\n");
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

int main() {
  while (1) {
    printf("$ ");
    fflush(stdout);
    // Wait for user input
    char input[100];
    fgets(input, 100, stdin);
    input[strlen(input) - 1] = '\0';
    // Parse user input
    char *args = input;
    // Build arg linked list
    arg *head = NULL;
    arg *current = NULL;
    char *ptr = args;
    while (*ptr != '\0') {
      // Skip any leading spaces
      while (*ptr == ' ') {
        ptr++;
      }
      if (*ptr == '\0') {
        break;
      }
      // Allocate a new arg node
      arg *new_arg = malloc(sizeof(arg));
      if (new_arg == NULL) {
        printf("Failed to allocate memory for new_arg\n");
        free_args(head);
        exit(1);
      }
      new_arg->next = NULL;
      char buffer[1024];
      int len = 0;
      int in_quotes = 0;
      int in_double_quotes = 0;
      // Collect the argument characters
      while (*ptr != '\0' && (*ptr != ' ' || in_double_quotes || in_quotes)) {
        if (*ptr == '"' && !in_double_quotes && !in_quotes) {
          in_double_quotes = 1;
          ptr++; // Skip the opening quote
        } else if (*ptr == '\'' && !in_quotes && !in_double_quotes) {
          in_quotes = 1;
          ptr++; // Skip the opening quote
        } else if (*ptr == '"' && in_double_quotes) {
          in_double_quotes = 0;
          ptr++; // Skip the closing quote
        } else if (*ptr == '\'' && in_quotes) {
          in_quotes = 0;
          ptr++; // Skip the closing quote
        } else if (*ptr == '\\' && in_double_quotes) {
          ptr++; // Skip the escape character
          if (*ptr == '\0') {
            break;
          }
          if (*ptr == '\\' || *ptr == '"' || *ptr == '$' || *ptr == '\n') {
            buffer[len++] = *ptr++;
          } else {
            buffer[len++] = '\\';
            buffer[len++] = *ptr++;
          }
        } else if (*ptr == '\\' && (!in_double_quotes && !in_quotes)) {
          ptr++; // Skip the escape character
          if (*ptr == '\0') {
            break;
          }
          buffer[len++] = *ptr++;
        } else {
          buffer[len++] = *ptr++;
        }
      }
      buffer[len] = '\0';
      // Assign the argument string
      new_arg->argstr = strdup(buffer);
      new_arg->is_quote = in_quotes || in_double_quotes;
      in_double_quotes = 0;
      
      if (new_arg->argstr == NULL) {
        printf("Failed to allocate memory for argstr\n");
        free_args(head);
        exit(1);
      }
      // Add the new arg to the linked list
      if (head == NULL) {
        head = new_arg;
        current = new_arg;
      } else {
        current->next = new_arg;
        current = new_arg;
      }
    }
    // BUILTIN COMMANDS
    if (strcmp(head->argstr, "exit") == 0) {
      exit(0);
    } else if (strcmp(head->argstr, "cd") == 0) {
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
          printf("Failed to allocate memory for new_arg\n");
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
        printf("cd: %s: No such file or directory\n", head->argstr);
      }
    } else if (strcmp(head->argstr, "echo") == 0) {
      current = head->next;
      while (current != NULL) {
        printf("%s", current->argstr);
        current = current->next;
        if (current != NULL) {
          printf(" ");
        }
      }
      printf("\n");
    } else if (strcmp(head->argstr, "pwd") == 0) {
      char cwd[100];
      if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
      } else {
        printf("Failed to get current working directory\n");
      }
    } else if (strcmp(head->argstr, "type") == 0) {
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
    // EXTERNAL COMMANDS
    } else if (input[0] != '\0') {
      // Fork and exec external command
      char *command = find_command(head->argstr);
      if (command != NULL) {
        pid_t pid = fork();
        if (pid == 0) {
          int argc = 0;
          current = head->next;
          while (current != NULL) {
            argc++;
            current = current->next;
          }
          char **argv = malloc((argc + 2) * sizeof(char *));
          if (argv == NULL) {
            printf("Failed to allocate memory for argv\n");
            free(command);
            free_args(head);
            exit(1);
          }
          argv[0] = command;
          current = head->next;
          int i = 1;
          while (current != NULL) {
            argv[i++] = current->argstr;
            current = current->next;
          }
          argv[i] = NULL;
          execv(command, argv);
          printf("Failed to execute %s\n", command);
          free(command);
          free_args(head);
          exit(1);
        } else {
          int status;
          waitpid(pid, &status, 0);
          free(command);
          free_args(head);
      }
      } else {
        printf("%s: command not found\n", input);
        free_args(head);
      }
    } else {
      // Should never reach this point
      printf("error: unknown error\n");
      exit(1);
    }
  }
  return 0;
}
