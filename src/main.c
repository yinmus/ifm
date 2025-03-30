/*
======================================
    IFM by yinmus (c) 2025-2025
======================================

Relative path : ifm/src/main.c
Github url : https://github.com/yinmus/ifm.git
License : GPLv3

*/

#include <errno.h>
#include <locale.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "fmh.h"
#include "ifm.h"
#include "ui.h"

int main(int argc, char* argv[]) {
  setlocale(LC_ALL, "ru_RU.UTF-8");

  create_default_config();

  if (argc > 1) {
    if (strcmp(argv[1], "-h") == 0) {
      reference();
      return 0;
    } else if (strcmp(argv[1], "-?") == 0) {
      initscr();
      noecho();
      curs_set(0);
      keypad(stdscr, TRUE);
      start_color();
      init_pair(1, COLOR_CYAN, COLOR_BLACK);
      init_pair(2, COLOR_GREEN, COLOR_BLACK);

      doc_menu();

      endwin();
      return 0;
    } else if (strcmp(argv[1], "-V") == 0) {
      Version();
      return 0;
    } else {
      hs_path(argv[1], path);
      struct stat st;
      if (stat(path, &st) != 0 || !S_ISDIR(st.st_mode)) {
        getcwd(path, sizeof(path));
      }
    }
  } else {
    getcwd(path, sizeof(path));
  }

  initscr();
  noecho();
  curs_set(0);
  keypad(stdscr, TRUE);
  start_color();
  init_pair(1, COLOR_CYAN, COLOR_BLACK);
  init_pair(2, COLOR_GREEN, COLOR_BLACK);

  mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
  mouseinterval(0);

  list(path);
  strncpy(lpath, path, sizeof(lpath));

  while (1) {
    UI();
    int ch = getch();

    if (ch == KEY_MOUSE) {
      MEVENT event;
      if (getmouse(&event) == OK) {
        if (event.bstate & BUTTON1_PRESSED) {
          int y = event.y - 2;
          if (y >= 0 && y < file_count) {
            int clicked_item = y + offset;

            if (clicked_item == last_clicked &&
                (time(NULL) - last_click_time) * 1000 < 2000) {
              char full_path[MAX_PATH];
              snprintf(full_path, sizeof(full_path), "%s/%s", path,
                       files[clicked_item]);

              if (dirt(full_path)) {
                strncpy(lpath, path, sizeof(lpath));
                chdir(full_path);
                getcwd(path, sizeof(path));
                list(path);
                selected = 0;
                offset = 0;
              } else {
                open_file(files[clicked_item]);
              }
              last_clicked = -1;
            } else {
              selected = clicked_item;
              last_clicked = clicked_item;
              last_click_time = time(NULL);
            }
          }
        } else if (event.bstate & BUTTON3_PRESSED) {
          to_back();
        } else if (event.bstate & BUTTON4_PRESSED) {
          if (selected > 0)
            selected--;
        } else if (event.bstate & BUTTON5_PRESSED) {
          if (selected < file_count - 1)
            selected++;
        }
      }
    } else {
      switch (ch) {
        case 'q':
          clear();
          endwin();
          return 0;
          break;
        case 'h':
        case KEY_LEFT:
          to_back();
          break;
        case 'k':
        case KEY_UP:
          if (selected > 0)
            selected--;
          break;

        case 'K':
          if (selected > 0) {
            selected -= 10;
          }
          if (selected < 0)
            selected = 0;

          break;

        case 'j':
        case KEY_DOWN:
          if (selected < file_count - 1)
            selected++;
          break;
        case 'J':
          if (selected < file_count - 1) {
            selected += 10;
          }
          if (selected >= file_count)
            selected = file_count - 1;

          break;

        case 'G':
          selected = file_count - 1;
          break;
        case 'g': {
          goto_help();
          break;
        }
        case 8:
          s_hidden = !s_hidden;
          list(path);
          selected = 0;
          offset = 0;
          clear();
          UI();
          break;
        case 10:
        case 'l':
        case KEY_RIGHT: {
          char full_path[MAX_PATH];
          snprintf(full_path, sizeof(full_path), "%s/%s", path,
                   files[selected]);

          if (dirt(full_path)) {
            strncpy(lpath, path, sizeof(lpath));
            chdir(full_path);
            getcwd(path, sizeof(path));
            list(path);
            selected = 0;
            offset = 0;
          } else {
            open_file(files[selected]);
          }
          break;
        }

        case 'c': {
          char full_path[MAX_PATH];
          snprintf(full_path, sizeof(full_path), "%s/%s", path,
                   files[selected]);

          struct stat st;
          if (stat(full_path, &st) != 0) {
            break;
          }

          int found = -1;
          for (int i = 0; i < MAX_FILES; i++) {
            if (marked_files[i].marked) {
              struct stat marked_st;
              if (stat(marked_files[i].path, &marked_st) == 0 &&
                  marked_st.st_ino == st.st_ino) {
                found = i;
                break;
              }
            }
          }

          if (found >= 0) {
            marked_files[found].marked = 0;
            memset(marked_files[found].path, 0, MAX_PATH);
          } else {
            for (int i = 0; i < MAX_FILES; i++) {
              if (!marked_files[i].marked) {
                strncpy(marked_files[i].path, full_path, MAX_PATH);
                marked_files[i].marked = 1;
                break;
              }
            }
          }

          if (selected < file_count - 1) {
            selected++;
          }
          break;
        }

        case KEY_DC: {
          int any_marked = 0;
          for (int i = 0; i < MAX_FILES; i++) {
            if (marked_files[i].marked)
              any_marked = 1;
          }

          if (any_marked) {
            if (confrim_delete("marked files")) {
              for (int i = 0; i < MAX_FILES; i++) {
                if (marked_files[i].marked) {
                  rm(marked_files[i].path);
                  memset(marked_files[i].path, 0, MAX_PATH);
                  marked_files[i].marked = 0;
                }
              }
              list(path);
              selected = 0;
            }
          } else {
            char full_path[MAX_PATH];
            snprintf(full_path, sizeof(full_path), "%s/%s", path,
                     files[selected]);
            if (confrim_delete(files[selected])) {
              rm(full_path);
              list(path);
              selected = 0;
            }
          }
          break;
        }

        case 'm':
          cr_dir();
          list(path);
          break;
        case 't':
          cr_file();
          list(path);
          break;
        case 'r':
          ren(files[selected]);
          break;
        case 'o':
          open_with(files[selected]);
          break;

        case 'i':
          doc_menu();
          break;

        case 'M':
          show_marked_files();
          break;

        case 'e':
          mark_help();
          break;

        case KEY_PPAGE:  // Page Up
          if (selected > 34) {
            selected -= 35;
          } else {
            selected = 0;
          }
          break;

        case KEY_NPAGE:  // Page Down
          if (selected < file_count - 35) {
            selected += 35;
          } else {
            selected = file_count - 1;
          }
          break;
      }
    }
  }

  endwin();
  return 0;
}
