/*
======================================
    IFM by yinmus (c) 2025-2025
======================================

Relative path : ifm/src/fmh.c
Github url : https://github.com/yinmus/ifm.git
License : GPLv3

*/

#include "fmh.h"

#include <locale.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "ifm.h"

#define IFM_VERSION "0.0.6"

void goto_help() {
  int win_height = 16;
  int win_width = 40;
  int start_y = (LINES - win_height) / 1.3;
  int start_x = 0;

  WINDOW* win = newwin(win_height, win_width, start_y, start_x);
  keypad(win, TRUE);
  wattron(win, COLOR_PAIR(1));
  box(win, 0, 0);
  wattroff(win, COLOR_PAIR(1));

  mvwprintw(win, 1, 2, "Go to:");
  mvwprintw(win, 2, 2, "h - cd ~");
  mvwprintw(win, 3, 2, "/ - cd /");
  mvwprintw(win, 4, 2, "d - cd /dev");
  mvwprintw(win, 5, 2, "e - cd /etc");
  mvwprintw(win, 6, 2, "m - cd /media or /run/media");
  mvwprintw(win, 7, 2, "M - cd /mnt");
  mvwprintw(win, 8, 2, "o - cd /opt");
  mvwprintw(win, 9, 2, "t - cd /tmp");
  mvwprintw(win, 10, 2, "u - cd /usr");
  mvwprintw(win, 11, 2, "s - cd /srv");
  mvwprintw(win, 12, 2, "? - cd /usr/share/doc/ifm");
  mvwprintw(win, 13, 2, "g - goto first file");

  wrefresh(win);

  int ch = getch();
  delwin(win);
  touchwin(stdscr);
  refresh();

  if (ch != ERR) {
    goto_cmd(ch);
  }
}

void reference() {
  printf("IFM - Lightweight Ncurses File Manager\n");
  printf("Version: %s\n\n", IFM_VERSION);
  printf("Usage: ifm [OPTION] [PATH]\n\n");
  printf("Options:\n");
  printf("  -h, -?    Show this help message\n");
  printf("  -V        Show version information\n");
  printf(
      "  PATH      Open the specified directory (default: current "
      "directory)\n\n");
  printf("Examples:\n");
  printf("  ifm                     Open current directory\n");
  printf("  ifm Documents           Open 'Documents' directory\n");
  printf("  ifm -h                  Show this help message\n");
  printf("  ifm -?                  Open the menu\n");
  printf("  ifm -V                  Show version information\n");
}

void Version() {
  printf("ifm %s\n\n", IFM_VERSION);
  printf("Copyright (c) 2025 YINMUS-IFM\n");
  printf("Released under the MIT License.\n\n");
  printf("Author: Yinmus <https://github.com/yinmus/>\n");
  printf("Please report bugs: <https://github.com/yinmus/ifm/issues>\n");
}

void cls() {
  clear();
  refresh();
}

int compare(const void* a, const void* b) {
  const char* name1 = (const char*)a;
  const char* name2 = (const char*)b;

  char path1[MAX_PATH], path2[MAX_PATH];
  snprintf(path1, sizeof(path1), "%s/%s", path, name1);
  snprintf(path2, sizeof(path2), "%s/%s", path, name2);

  struct stat stat1, stat2;
  int is_dir1 = (stat(path1, &stat1) == 0) && S_ISDIR(stat1.st_mode);
  int is_dir2 = (stat(path2, &stat2) == 0) && S_ISDIR(stat2.st_mode);

  if (is_dir1 && !is_dir2)
    return -1;
  if (!is_dir1 && is_dir2)
    return 1;

  return strcasecmp(name1, name2);
}

int dirt(const char* path) {
  struct stat statbuf;
  return (stat(path, &statbuf) == 0 && S_ISDIR(statbuf.st_mode));
}

int dist_s(const char* str) {
  setlocale(LC_ALL, "");
  int width = 0;
  wchar_t wc;
  const char* ptr = str;
  size_t len = strlen(str);

  while (*ptr != '\0' && len > 0) {
    int consumed = mbtowc(&wc, ptr, len);
    if (consumed <= 0)
      break;
    width += wcwidth(wc);
    ptr += consumed;
    len -= consumed;
  }
  return width;
}
