#include "commands.h"

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
  } else if (strchr(head->argstr, '~') != NULL) {
    char *home = getenv("HOME");
    char *new_arg = malloc(strlen(home) + strlen(head->argstr) + 1);
    if (new_arg == NULL) {
      perror("Failed to allocate memory for new_arg\n");
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
    printf("cd: %s: No such file or directory\n", head->argstr);
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
