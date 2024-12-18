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
    } else if (*args == '|' && !in_double_quotes && !in_quotes) {
      break;
    } else if (*args == '<' && !in_double_quotes && !in_quotes) {
      break;
    } else if (*args == '>' && !in_double_quotes && !in_quotes) {
      break;
    } else {
      buffer[len++] = *args++;
    }
  }
  buffer[len] = '\0';
  if (current != NULL) {
    current->is_quote = in_quotes || in_double_quotes;
  }
  return strdup(buffer);
}

int get_fd_in(char *args) {
  int fd = 0;
  while (*args != '<') { // Skip to <
    args++;
  }
  if (*args == '\0') {
    return -1;
  }
  args++;
  while (*args == ' ') { // Skip whitespace
    args++;
  }
  if (*args == '\0') {
    return -1;
  }
  char *file_name = parse_string(args, NULL);
  if (file_name == NULL) {
    perror("Failed to allocate memory for file_name\n");
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
  char *file_name = parse_string(args, NULL);
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
