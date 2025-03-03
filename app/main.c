#include "main.h"

char* history[MAX_HISTORY];
int history_count = 0;
int history_index = 0;

void disable_raw_mode() {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_term);
}

void enable_raw_mode() {
  tcgetattr(STDIN_FILENO, &orig_term);
  atexit(disable_raw_mode);
  struct termios raw = orig_term;
  raw.c_lflag &= ~(ECHO | ICANON);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void free_history() {
  for (int i = 0; i < history_count; i++) {
    free(history[i]);
  }
}

void add_history(const char *cmd) {
  if (history_count < MAX_HISTORY) {
    history[history_count++] = strdup(cmd);
  } else {
    free(history[0]);
    for (int i = 1; i < MAX_HISTORY; i++) {
      history[i - 1] = history[i];
    }
    history[MAX_HISTORY - 1] = strdup(cmd);
  }
  history_index = history_count; // reset navigation index
}

int main() {
  atexit(free_history);
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
  enable_raw_mode();
  char cmd[MAX_INPUT_SIZE];
  int cmd_len = 0;
  memset(cmd, 0, sizeof(cmd));
  printf("%s@%s$ ", user, host);
  fflush(stdout);
  while (1) {
    char c;
    ssize_t nread = read(STDIN_FILENO, &c, 1);
    if (nread == -1) {
      perror("Failed to read input\n");
      exit(1);
    }
    if (nread == 0) {
      break;
    }
    if (c == '\n') {
      cmd[cmd_len] = '\0';
      if (cmd_len > 0) {
        add_history(cmd);
      }
      printf("\n");
      // execute command
      char *args = cmd;
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
      }
      free_commands(command_head);

      enable_raw_mode();
      cmd_len = 0;
      memset(cmd, 0, sizeof(cmd));
      printf("%s@%s$ ", user, host);
      fflush(stdout);
    } else if (c == 127 || c == '\b') { // handle backspace
      if (cmd_len > 0) {
        cmd_len--;
        cmd[cmd_len] = '\0';
        printf("\b \b");
        fflush(stdout);
      }
    } else if (c == 27) { // start of an arrow key sequence
      char seq[2];
      if (read(STDIN_FILENO, &seq[0], 1) == 0) continue;
      if (read(STDIN_FILENO, &seq[1], 1) == 0) continue;
      if (seq[0] == '[') {
        if (seq[1] == 'A') { // up arrow pressed
          if (history_index > 0) {
            history_index--;
            // Clear the current line and print the history command
            printf("\33[2K\r%s@%s$ %s", user, host, history[history_index]);
            fflush(stdout);
            strcpy(cmd, history[history_index]);
            cmd_len = strlen(cmd);
          }
        } else if (seq[1] == 'B') { // down arrow pressed
          if (history_index < history_count - 1) {
            history_index++;
            printf("\33[2K\r%s@%s$ %s", user, host, history[history_index]);
            fflush(stdout);
            strcpy(cmd, history[history_index]);
            cmd_len = strlen(cmd);
          } else {
            // No more recent history; clear the line
            history_index = history_count;
            printf("\33[2K\r%s@%s$ ", user, host);
            fflush(stdout);
            memset(cmd, 0, sizeof(cmd));
            cmd_len = 0;
          }
        } else if (seq[1] == 'C') { // right arrow pressed
          if (cmd_len < strlen(cmd)) {
            write(STDOUT_FILENO, &cmd[cmd_len], 1);
            cmd_len++;
          }
        } else if (seq[1] == 'D') { // left arrow pressed
          if (cmd_len > 0) {
            cmd_len--;
            write(STDOUT_FILENO, "\b", 1);
          }
        }
      }
    } else if (c == 9) { // tab key
      char *matches[] = {"cd", "pwd", "echo", "type", "exit"};
      for (int i = 0; i<5; i++) {
        if (strncmp(matches[i], cmd, cmd_len) == 0) {
          write(STDOUT_FILENO, matches[i] + cmd_len, strlen(matches[i]) - cmd_len);
          cmd_len = strlen(matches[i]);
          strcpy(cmd, matches[i]);
          break;
        }
      }
    } else {
      cmd[cmd_len++] = c;
      write(STDOUT_FILENO, &c, 1);
    }
  }
  for (int i = 0; i < history_count; i++) {
    free(history[i]);
  }
  return 0;
}
