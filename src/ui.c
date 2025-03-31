/*
======================================
    IFM by yinmus (c) 2025-2025
======================================

Relative path : ifm/src/ui.c
Github url : https://github.com/yinmus/ifm.git
License : GPLv3

*/

#include "ui.h"
#include "fmh.h"
#include "icons.h"
#include "ifm.h"
#include <locale.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <wchar.h>

#define MAX_COLORS 256
static char *color_pairs[MAX_COLORS] = {0};
static int next_color_pair = 20;

int get_hex(const char *hex_color) {
  if (hex_color == NULL)
    return 0;

  for (int i = 0; i < next_color_pair; i++) {
    if (color_pairs[i] && strcmp(color_pairs[i], hex_color) == 0) {
      return i + 1;
    }
  }

  if (next_color_pair >= MAX_COLORS)
    return 0;

  const char *hex = (hex_color[0] == '#') ? hex_color + 1 : hex_color;
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

void UI() {
  curs_set(0);
  clear();
  setlocale(LC_ALL, "");

  init_pair(1, COLOR_CYAN, COLOR_BLACK);   
  init_pair(2, COLOR_GREEN, COLOR_BLACK);  
  init_pair(3, COLOR_CYAN, COLOR_BLACK);   
  init_pair(4, COLOR_YELLOW, COLOR_BLACK); 
  init_pair(5, COLOR_WHITE, COLOR_BLACK);  
  init_pair(6, COLOR_RED, COLOR_BLACK);    
  init_pair(7, COLOR_BLACK, COLOR_WHITE);  
  init_pair(8, COLOR_BLUE, COLOR_BLACK);   

  attron(COLOR_PAIR(1) | A_BOLD);
  printw("%s", path);

  if (file_count > 0 && selected >= 0 && selected < file_count) {
    if (strcmp(path, "/") != 0 && path[strlen(path) - 1] != '/') {
      printw("/");
    }

    attron(COLOR_PAIR(5));
    printw("%s", files[selected]);
    attroff(COLOR_PAIR(5));
  }

  int current_length = getcurx(stdscr);
  if (current_length > COLS) {
    move(0, 0);
    clrtoeol();

    int max_path = COLS - 11 - strlen(files[selected]) - 1;
    if (max_path > 0) {
      attron(COLOR_PAIR(1));
      if (strlen(path) > max_path) {
        printw("...%s", path + strlen(path) - max_path);
      } else {
        printw("%s", path);
      }

      if (strcmp(path, "/") != 0 && path[strlen(path) - 1] != '/') {
        printw("/");
      }

      attron(COLOR_PAIR(5));
      printw("%s", files[selected]);
      attroff(COLOR_PAIR(5));
    } else {
      attron(COLOR_PAIR(5));
      printw("%.*s", COLS - 11, files[selected]);
      attroff(COLOR_PAIR(5));
    }
  }

  attroff(COLOR_PAIR(1) | A_BOLD);

  int height = LINES - 4;
  if (selected < offset) {
    offset = selected;
  } else if (selected >= offset + height) {
    offset = selected - height + 1;
  }

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

    char truncated_name[MAX_DISPLAY_NAME + 1];
    if (strlen(files[i + offset]) > MAX_DISPLAY_NAME) {
      snprintf(truncated_name, sizeof(truncated_name), "%.*s...",
               MAX_DISPLAY_NAME - 3, files[i + offset]);
    } else {
      strncpy(truncated_name, files[i + offset], MAX_DISPLAY_NAME);
      truncated_name[MAX_DISPLAY_NAME] = '\0';
    }

    struct stat st;
    IconResult icon_result = {"", NULL};
    int color_pair = 5; 

    if (stat(full_path, &st) == 0) {
      if (S_ISDIR(st.st_mode)) {
          icon_result.icon = "";
          icon_result.color = "#7c91b4";
          color_pair = 1;
      } else if (S_ISREG(st.st_mode)) {
          if (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) {
              color_pair = 2;
          } else {
              color_pair = 5;
          }
          const char *ext = strrchr(files[i + offset], '.');
          if (ext) {
              icon_result = icon_ext(ext + 1);
          }
      } else if (S_ISLNK(st.st_mode)) {
          color_pair = 8; 
      }
  }

    int icon_color_pair = color_pair;
    if (icon_result.color) {
      icon_color_pair = get_hex(icon_result.color);
      if (icon_color_pair == 0) {
        icon_color_pair = color_pair;
      }
    }

    if (i + offset == selected) {
      attron(COLOR_PAIR(7));
    }

    if (is_marked) {
      attron(COLOR_PAIR(6));
      mvaddch(i + 2, 2, '*');
      attroff(COLOR_PAIR(6));
    } else {
      mvaddch(i + 2, 2, ' ');
    }

    attron(COLOR_PAIR(icon_color_pair));
    mvprintw(i + 2, 3, "%s", icon_result.icon);
    attroff(COLOR_PAIR(icon_color_pair));

    attron(COLOR_PAIR(color_pair));
    mvprintw(i + 2, 5, "%-*s", MAX_DISPLAY_NAME, files[i + offset]);
    attroff(COLOR_PAIR(color_pair));

    if (i + offset == selected) {
      attroff(COLOR_PAIR(7));
    }
  }

  if (file_count > 0 && selected >= 0 && selected < file_count) {
    char full_path[MAX_PATH];
    snprintf(full_path, sizeof(full_path), "%s/%s", path, files[selected]);

    struct stat st;
    if (stat(full_path, &st) == 0) {
      double size = st.st_size;
      const char *unit = "B";
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

      int current_len = snprintf(NULL, 0, "%d/%d %s %s %.0f%s", selected + 1,
                                 file_count, date_str, perms, size, unit);
      for (int i = current_len; i < COLS; i++) {
        addch(' ');
      }
      attroff(COLOR_PAIR(3) | A_BOLD);
    }
  } else {
    attron(COLOR_PAIR(3));
    for (int i = 0; i < COLS; i++) {
      mvaddch(LINES - 2, i, ' ');
    }
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

  WINDOW *win = newwin(win_height, win_width, start_y, start_x);
  keypad(win, TRUE);
  wattron(win, COLOR_PAIR(1));
  box(win, 0, 0);
  wattroff(win, COLOR_PAIR(1));
  mvwprintw(win, 0, 2, " Marked Files (%d) ", marked_count);

  int line = 1;
  for (int i = 0; i < MAX_FILES && line < win_height - 1; i++) {
    if (marked_files[i].marked) {
      char *filename = basename(marked_files[i].path);

      IconResult icon_result = {"", NULL};
      struct stat st;
      if (stat(marked_files[i].path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
          icon_result.icon = "";
          icon_result.color = "#5391e9";
        } else if (S_ISREG(st.st_mode)) {
          const char *ext = strrchr(filename, '.');
          if (ext) {
            icon_result = icon_ext(ext + 1);
          }
        }
      }

      int icon_color_pair = 5;
      if (icon_result.color) {
        icon_color_pair = get_hex(icon_result.color);
        if (icon_color_pair == 0) {
          icon_color_pair = 5;
        }
      }

      wattron(win, COLOR_PAIR(icon_color_pair));
      mvwprintw(win, line, 2, "%s", icon_result.icon);
      wattroff(win, COLOR_PAIR(icon_color_pair));

      wattron(win, COLOR_PAIR(5));
      mvwprintw(win, line, 4, "%s", marked_files[i].path);
      wattroff(win, COLOR_PAIR(5));

      line++;
    }
  }

  wrefresh(win);
  wgetch(win);
  delwin(win);
}