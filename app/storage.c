#include "storage.h"

void store_command(char *command) {
    printf("Storing command: %s\n", command);
    char *command_history = getenv("COMMAND_HISTORY");
    if (command_history == NULL) {
        perror("Failed to get command history\n");
        exit(1);
    }
    FILE *file = fopen(command_history, "a");
    if (file == NULL) {
        perror("Failed to open command history file\n");
        exit(1);
    }
    fprintf(file, "%s\n", command);
    fclose(file);
}

char *get_previous_command() {
  // Get previous command from history
  printf("getting previous command\n");
  
  return NULL;
}

char *get_next_command() {
  // Get next command from history
  return NULL;
}
