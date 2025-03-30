/*
======================================
    IFM by yinmus (c) 2025-2025
======================================

Relative path : ifm/src/ui.c
Github url : https://github.com/yinmus/ifm.git
License : GPLv3

*/

#include "ui.h"

#include <libgen.h>
#include <locale.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <wchar.h>

#include "icons.h"
#include "ifm.h"

#define MAX_COLORS 256
static char* color_pairs[MAX_COLORS] = {0};
static int next_color_pair = 20;

int get_hex(const char* hex_color) {
  if (hex_color == NULL)
    return 0;

  for (int i = 0; i < next_color_pair; i++) {
    if (color_pairs[i] && strcmp(color_pairs[i], hex_color) == 0) {
      return i + 1;
    }
  }

  if (next_color_pair >= MAX_COLORS)
    return 0;

  const char* hex = (hex_color[0] == '#') ? hex_color + 1 : hex_color;
  int r, g, b;
  if (sscanf(hex, "%02x%02x%02x", &r, &g, &b) != 3) {
    return 0;
  }

  r = r * 1000 / 255;
  g = g * 1000 / 255;
  b = b * 1000 / 255;

  init_color(next_color_pair + 8, r, g, b);
  init_pair(next_color_pair + 1, next_color_pair + 8, COLOR_BLACK);
  color_pairs[next_color_pair] = strdup(hex_color);

  return next_color_pair++ + 1;
}

void init_ui_colors() {
  start_color();
  use_default_colors();

  init_pair(1, COLOR_CYAN, COLOR_BLACK);
  init_pair(2, COLOR_GREEN, COLOR_BLACK);
  init_pair(3, COLOR_CYAN, COLOR_BLACK);
  init_pair(4, COLOR_YELLOW, COLOR_BLACK);
  init_pair(5, COLOR_WHITE, COLOR_BLACK);
  init_pair(6, COLOR_RED, COLOR_BLACK);
  init_pair(7, COLOR_BLACK, COLOR_WHITE);
  init_color(8, 518, 627, 776);
  init_pair(8, 8, COLOR_BLACK);
  init_pair(9, COLOR_WHITE, COLOR_BLACK);
  curs_set(0);
}

void UI() {
  clear();
  setlocale(LC_ALL, "");
  init_ui_colors();

  attron(COLOR_PAIR(5) | A_BOLD);
  mvprintw(0, 0, "[ IFM ] - ");
  if (strcmp(path, "/") == 0) {
    printw("/");
  } else {
    printw("%s", path);
  }

  if (file_count > 0 && selected >= 0 && selected < file_count) {
    if (strcmp(path, "/") != 0 && path[strlen(path) - 1] != '/') {
      printw("/");
    }
    attron(COLOR_PAIR(1));
    printw("%s", files[selected]);
  }
  attroff(A_BOLD);

  int height = LINES - 4;
  if (selected < offset)
    offset = selected;
  else if (selected >= offset + height)
    offset = selected - height + 1;

  for (int i = 0; i < height && i + offset < file_count; i++) {
    char full_path[MAX_PATH];
    snprintf(full_path, sizeof(full_path), "%s/%s", path, files[i + offset]);

    int is_marked = 0;
    for (int j = 0; j < MAX_FILES; j++) {
      if (marked_files[j].marked &&
          strcmp(marked_files[j].path, full_path) == 0) {
        is_marked = 1;
        break;
      }
    }

    struct stat st;
    IconResult icon_result = {"", NULL};
    int color_pair = 8;

    if (stat(full_path, &st) == 0) {
      if (S_ISDIR(st.st_mode)) {
        icon_result.icon = "";
        icon_result.color = "#7587a6";
        color_pair = 8;
      } else if (S_ISREG(st.st_mode)) {
        color_pair = 2;
        const char* ext = strrchr(files[i + offset], '.');
        if (ext)
          icon_result = icon_ext(ext + 1);
      }
    }

    if (i + offset == selected) {
      attron(COLOR_PAIR(9) | A_BOLD);
      mvaddch(i + 2, 1, '|');
      attroff(COLOR_PAIR(9) | A_BOLD);
    }

    if (is_marked) {
      attron(COLOR_PAIR(6));
      mvaddch(i + 2, 2, '*');
      attroff(COLOR_PAIR(6));
    } else {
      mvaddch(i + 2, 2, ' ');
    }

    if (icon_result.color) {
      int cp = get_hex(icon_result.color);
      if (cp)
        attron(COLOR_PAIR(cp));
    }
    mvprintw(i + 2, 3, "%s", icon_result.icon);
    if (icon_result.color) {
      int cp = get_hex(icon_result.color);
      if (cp)
        attroff(COLOR_PAIR(cp));
    }

    if (color_pair)
      attron(COLOR_PAIR(color_pair));
    printw(" %-*s", MAX_DISPLAY_NAME, files[i + offset]);
    if (color_pair)
      attroff(COLOR_PAIR(color_pair));
  }

  if (file_count > 0 && selected >= 0 && selected < file_count) {
    char full_path[MAX_PATH];
    snprintf(full_path, sizeof(full_path), "%s/%s", path, files[selected]);

    struct stat st;
    if (stat(full_path, &st) == 0) {
      double size = st.st_size;
      const char* unit = "B";
      if (size > 1024) {
        size /= 1024;
        unit = "K";
      }
      if (size > 1024) {
        size /= 1024;
        unit = "M";
      }
      if (size > 1024) {
        size /= 1024;
        unit = "G";
      }

      char date_str[20];
      strftime(date_str, sizeof(date_str), "%Y-%m-%d %H:%M",
               localtime(&st.st_mtime));

      char perms[10];
      snprintf(perms, sizeof(perms), "%c%c%c%c%c%c%c%c%c",
               (S_ISDIR(st.st_mode)) ? 'd' : '-',
               (st.st_mode & S_IRUSR) ? 'r' : '-',
               (st.st_mode & S_IWUSR) ? 'w' : '-',
               (st.st_mode & S_IXUSR) ? 'x' : '-',
               (st.st_mode & S_IRGRP) ? 'r' : '-',
               (st.st_mode & S_IWGRP) ? 'w' : '-',
               (st.st_mode & S_IXGRP) ? 'x' : '-',
               (st.st_mode & S_IROTH) ? 'r' : '-',
               (st.st_mode & S_IWOTH) ? 'w' : '-',
               (st.st_mode & S_IXOTH) ? 'x' : '-');

      attron(COLOR_PAIR(3) | A_BOLD);
      mvprintw(LINES - 2, 0, "%d/%d %s %s %.0f%s", selected + 1, file_count,
               date_str, perms, size, unit);
      clrtoeol();
      attroff(COLOR_PAIR(3) | A_BOLD);
    }
  } else {
    attron(COLOR_PAIR(3));
    move(LINES - 2, 0);
    clrtoeol();
    attroff(COLOR_PAIR(3));
  }

  refresh();
}

void show_marked_files() {
  int marked_count = 0;
  for (int i = 0; i < MAX_FILES; i++) {
    if (marked_files[i].marked)
      marked_count++;
  }

  if (marked_count == 0) {
    mvprintw(LINES - 1, 0, "No files marked");
    refresh();
    getch();
    return;
  }

  int win_height =
      (marked_count + 4 < LINES - 4) ? marked_count + 4 : LINES - 4;
  int win_width = COLS - 10;
  int start_y = (LINES - win_height) / 2;
  int start_x = (COLS - win_width) / 2;

  WINDOW* win = newwin(win_height, win_width, start_y, start_x);
  keypad(win, TRUE);

  wattron(win, COLOR_PAIR(1));
  box(win, 0, 0);
  wattroff(win, COLOR_PAIR(1));
  mvwprintw(win, 0, 2, " Marked Files (%d) ", marked_count);

  int line = 1;
  for (int i = 0; i < MAX_FILES && line < win_height - 1; i++) {
    if (marked_files[i].marked) {
      char* filename = basename(marked_files[i].path);

      IconResult icon_result = {"", NULL};
      struct stat st;
      if (stat(marked_files[i].path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
          icon_result.icon = "";
          icon_result.color = "#04a1eb";
        } else if (S_ISREG(st.st_mode)) {
          const char* ext = strrchr(filename, '.');
          if (ext)
            icon_result = icon_ext(ext + 1);
        }
      }

      if (icon_result.color) {
        int cp = get_hex(icon_result.color);
        if (cp)
          wattron(win, COLOR_PAIR(cp));
      }
      wprintw(win, "%s", icon_result.icon);
      if (icon_result.color) {
        int cp = get_hex(icon_result.color);
        if (cp)
          wattroff(win, COLOR_PAIR(cp));
      }

      // Путь
      wprintw(win, " %s", marked_files[i].path);
      line++;
    }
  }

  wrefresh(win);
  wgetch(win);
  delwin(win);
}