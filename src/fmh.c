
#include "fmh.h"

#include <limits.h>
#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ifm.h"

void reference() {
  printf("IFM - Lightweight File Manager\n");
  printf("Usage: ifm [PATH] OPTION] \n\n");
  printf("Options:\n");
  printf("  --help                  show this help message\n");
  printf("  -V                      show version information\n");
  printf("  PATH                    open the specified directory\n\n");
  printf("\n\nVersion: %s\n\n", IFM_VERSION);
}

void Version() {
  printf("ifm %s\n\n", IFM_VERSION);
  printf("Copyright (c) 2025 YINMUS-IFM\n");
  printf("Released under the MIT License.\n\n");
  printf("Author: Yinmus <https://github.com/yinmus/>\n");
  printf("Please report bugs: <https://github.com/yinmus/ifm/issues>\n");
}

int compare(const void *a, const void *b) {
  const char *name_a = (const char *)a;
  const char *name_b = (const char *)b;

  char path_a[MAX_PATH], path_b[MAX_PATH];
  snprintf(path_a, sizeof(path_a), "%s/%s", path, name_a);
  snprintf(path_b, sizeof(path_b), "%s/%s", path, name_b);

  struct stat stat_a, stat_b;
  int is_dir_a = (stat(path_a, &stat_a) == 0 && S_ISDIR(stat_a.st_mode));
  int is_dir_b = (stat(path_b, &stat_b) == 0 && S_ISDIR(stat_b.st_mode));

  if (is_dir_a == is_dir_b) {
    return strcasecmp(name_a, name_b);
  }
  return is_dir_b - is_dir_a;
}

void reset_terminal(int sig) {
  def_prog_mode();
  endwin();
  refresh();
}
