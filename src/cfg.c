/*
   cfg.c
   https://github.com/yinmus/ifm.git

*/

#include "cfg.h"

#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "ifm.h"

void create_default_config() {
  const char *home_dir = getenv("HOME");
  if (!home_dir) {
    struct passwd *pw = getpwuid(getuid());
    home_dir = pw->pw_dir;
  }

  char config_dir[MAX_PATH];
  snprintf(config_dir, sizeof(config_dir), "%s/.config/ifm", home_dir);
  mkdir(config_dir, 0755);

  char config_path[MAX_PATH];
  snprintf(config_path, sizeof(config_path), "%s/config", config_dir);

  if (access(config_path, F_OK) != 0) {
    FILE *file = fopen(config_path, "w");
    if (file) {
      fprintf(file, "nxtaftlab 0\n");
      fclose(file);
    }
  }
}

void load_config(Config *config) {
  strcpy(config->nxtaftlab, "nxtaftlab");
  const char *home_dir = getenv("HOME");
  if (!home_dir) {
    struct passwd *pw = getpwuid(getuid());
    home_dir = pw->pw_dir;
  }

  char config_path[MAX_PATH];
  snprintf(config_path, sizeof(config_path), "%s/.config/ifm/config", home_dir);

  FILE *file = fopen(config_path, "r");
  if (!file) {
    return;
  }

  char line[MAX_PATH];
  while (fgets(line, sizeof(line), file)) {
    line[strcspn(line, "\n")] = '\0';

    if (line[0] == '\0' || line[0] == '#') {
      continue;
    }

    char *key = strtok(line, " ");
    char *value = strtok(NULL, " ");

    if (key && value) {
      key = strtok(key, " \t");
      value = strtok(value, " \t");

      if (strcmp(key, "nxtaftlab") == 0) {
        strncpy(config->nxtaftlab, value, MAX_NAME);
      }
    }

    fclose(file);
  }
}
