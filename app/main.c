#include "main.h"

int main() {
  char *user = getenv("USER");
  if (user == NULL) {
    perror("Failed to get user\n");
    exit(1);
  }
  char host[MAX_USERNAME_SIZE];
  if (gethostname(host, sizeof(host)) != 0) {
    perror("Failed to get hostname\n");
    exit(1);
  }
  while (1) {
    printf("%s@%s$ ", user, host);
    fflush(stdout);
    // Wait for user input
    char input[MAX_INPUT_SIZE];
    if (fgets(input, MAX_INPUT_SIZE, stdin) == NULL) {
      break; // EOF
    }
    input[strlen(input) - 1] = '\0';
    char *args = input;
    // Build command linked list
    command *command_head = build_command(NULL);
    build_commands(args, command_head);
    int num_commands = 0;
    command *current = command_head;
    while (current != NULL) {
      num_commands++;
      current = current->next;
    }
    if (num_commands == 0) {
      free_commands(command_head);
      continue;
    }
    // BUILTIN COMMANDS
    if (strcmp(command_head->name, "exit") == 0) {
      free_commands(command_head);
      exit(0);
    } else if (strcmp(command_head->name, "cd") == 0) {
      change_directory(command_head->args);
    }
    // OTHER COMMANDS
    else if (command_head->name != NULL) {
      int num_pipes = num_commands - 1;
      int pipefds[num_pipes * 2];
      for (int i = 0; i < num_pipes; i++) {
        if (pipe(pipefds + i * 2) < 0) {
          perror("Failed to create pipe\n");
          free_commands(command_head);
          exit(1);
        }
      }
      int process_count = 0;
      command *current = command_head;
      while (current != NULL) {
        pid_t pid = fork();
        if (pid == 0) { // child
          if (process_count != 0) { // if not first
            if (dup2(pipefds[(process_count - 1) * 2], STDIN_FILENO) < 0) {
              perror("Failed to duplicate file descriptor\n");
              exit(1);
            }
          }
          if (current->next != NULL) { // if not last
            if (dup2(pipefds[process_count * 2 + 1], STDOUT_FILENO) < 0) {
              perror("Failed to duplicate file descriptor\n");
              exit(1);
            }
          }
          for (int i = 0; i < num_pipes * 2; i++) {
            close(pipefds[i]);
          }
          execute_command(current);
          exit(1);
        } else if (pid < 0) { // fork failed
          perror("Failed to fork\n");
          free_commands(command_head);
          exit(1);
        }
        // parent
        current = current->next;
        process_count++;
      }
      for (int i = 0; i < num_pipes * 2; i++) {
        close(pipefds[i]);
      }
      for (int i = 0; i < num_commands; i++) {
        wait(NULL);
      }
    } else {
      // Should never reach this point
      perror("error: unknown error\n");
      exit(99);
    }
  free_commands(command_head);
  }
  return 0;
}
