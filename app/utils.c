#include "utils.h"

char *find_command(char *command) {
  char *path = getenv("PATH");
  char *path_copy = strdup(path);
  if (path_copy == NULL) {
    perror("Failed to allocate memory for path_copy\n");
    return NULL;
  }
  char *dir = strtok(path_copy, ":");
  while (dir != NULL) {
    char *full_path = malloc(strlen(dir) + strlen(command) + 2);
    if (full_path == NULL) {
      perror("Failed to allocate memory for full_path\n");
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

char *parse_string(char **args, arg *current, command *command_head) {
  char buffer[MAX_INPUT_SIZE];
  int len = 0;
  int in_quotes = 0;
  int in_double_quotes = 0;
  char *ptr = *args;
  // Collect the argument characters
  while (*ptr != '\0' && (*ptr != ' ' || in_double_quotes || in_quotes)) {
    if (*ptr == '"' && !in_double_quotes && !in_quotes) {
      in_double_quotes = 1;
      ptr++;
    } else if (*ptr == '\'' && !in_quotes && !in_double_quotes) {
      in_quotes = 1;
      ptr++;
    } else if (*ptr == '"' && in_double_quotes) {
      in_double_quotes = 0;
      ptr++;
    } else if (*ptr == '\'' && in_quotes) {
      in_quotes = 0;
      ptr++;
    } else if (*ptr == '\\' && in_double_quotes) {
      ptr++;
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
      ptr++;
      if (*ptr == '\0') {
        break;
      }
      buffer[len++] = *ptr++;
    } else if (*ptr == '|' && !in_double_quotes && !in_quotes) {
      break;
    } else if (*ptr == '<' && !in_double_quotes && !in_quotes) {
      break;
    } else if (*ptr == '>' && !in_double_quotes && !in_quotes) {
      if (*(ptr - 1) != ' ' && isdigit(*(ptr - 1))) {
        buffer[len--] = '\0';
        if (*(ptr - 1) - '0' == 2) {
          command_head->stderr_out = 1;
        }
      }
      break;
    } else {
      buffer[len++] = *ptr++;
    }
  }
  buffer[len] = '\0';
  if (current != NULL) {
    current->is_quote = in_quotes || in_double_quotes;
  }
  *args = ptr; // Update the original args pointer
  return strdup(buffer);
}

int get_fd_in(char **args) {
  char *ptr = *args;
  int fd = 0;
  while (*ptr != '<') { // Skip to <
    ptr++;
  }
  if (*ptr == '\0') {
    return -1;
  }
  ptr++;
  while (*ptr == ' ') { // Skip whitespace
    ptr++;
  }
  if (*ptr == '\0') {
    return -1;
  }
  char *file_name = parse_string(&ptr, NULL, NULL);
  if (file_name == NULL) {
    perror("Failed to allocate memory for file_name\n");
    exit(1);
  }
  if (access(file_name, F_OK) != 0) {
    printf("%s: No such file or directory\n", file_name);
    free(file_name);
    return -1;
  }
  fd = open(file_name, O_RDONLY | O_NONBLOCK);
  if (fd == -1) {
    printf("Failed to open %s\n", file_name);
    free(file_name);
    return -1;
  }
  ptr += strlen(file_name);
  free(file_name);
  *args = ptr;
  return fd;
}

int get_fd_out(char *args) {
  int fd = 0;
  int append = 0; // 0 = truncate, 1 = append
  while (*args != '>') { // Skip to >
    args++;
  }
  if (*args == '\0') {
    return -1;
  }
  args++;
  if (*args == '>') {
    append = 1;
    args++;
  }
  while (*args == ' ') { // Skip whitespace
    args++;
  }
  if (*args == '\0') {
    return -1;
  }
  char *file_name = parse_string(&args, NULL, NULL);
  if (file_name == NULL) {
    perror("Failed to allocate memory for file_name\n");
    exit(1);
  }
  int flags = O_WRONLY | O_CREAT;
  if (append) {
    flags |= O_APPEND;
  } else {
    flags |= O_TRUNC;
  }
  fd = open(file_name, flags, 0644);
  if (fd == -1) {
    printf("Failed to open %s\n", file_name);
    free(file_name);
    return -1;
  }
  free(file_name);
  return fd;
}
