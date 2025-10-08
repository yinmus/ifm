#include <assert.h>
#include <locale.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "fmh.h"
#include "ifm.h"
#include "ui.h"

int
main(int argc, char* argv[])
{
  setlocale(LC_ALL, "ru_RU.UTF-8");

  setenv("ESCDELAY", "25", 1);

  if (argc > 1) {
    if (strcmp(argv[1], "--help") == 0) {
      endwin();
      reference();
      return 0;
    } else if (strcmp(argv[1], "-V") == 0) {
      endwin();
      Version();
      return 0;
    } else {
      resolve_path(argv[1], path, sizeof(path));
      struct stat st;
      if (stat(path, &st) != 0 || !S_ISDIR(st.st_mode)) {
        printf("\n\n\033[1m!Cannot open\033[0m\n\n\n");
        return 1;
      }
    }
  } else {
    getcwd(path, sizeof(path));
    init_files();
  }

  assert(initscr() != NULL);
  start_color();

  noecho();
  curs_set(0);
  keypad(stdscr, TRUE);

  assert(mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL) != 0);
  mouseinterval(0);

  list(path, NULL, false, false);
  strncpy(lpath, path, sizeof(lpath));

  while (1) {
    Display();
    int move_count = 1;
    int last_clicked = -1;
    int ch = getch();

    if (ch == KEY_MOUSE) {
      MEVENT event;
      assert(getmouse(&event) == OK);

      static clock_t last_click_time = 0;
      static int last_clicked_item = -1;

      if (event.bstate & LMB) {
        int y = event.y - 2;
        if (y >= 0 && y < file_count) {
          int clicked_item = y + offset;
          clock_t now = clock();

          if (last_clicked_item == clicked_item &&
              (double)(now - last_click_time) / CLOCKS_PER_SEC < 0.3) {
            char full_path[MAX_PATH];
            snprintf(
              full_path, sizeof(full_path), "%s/%s", path, files[clicked_item]);

            if (DIRT(full_path)) {
              strncpy(lpath, path, sizeof(lpath));
              if (chdir(full_path) == 0) {
                getcwd(path, sizeof(path));
                list(path, NULL, false, false);
                selected = 0;
                offset = 0;
              } else {
                line_clear(LINES - 1);
                mvprintw(LINES - 1, 0, "E: Cannot access directory");
                refresh();
                gtimeout(500);
              }
            } else {
              open_file(files[clicked_item]);
            }
            last_clicked_item = -1;
          } else {
            selected = clicked_item;
            last_clicked_item = clicked_item;
          }
          last_click_time = now;
        }
      } else if (event.bstate & RMB) {
        to_back();
      } else if (event.bstate & MWU) {
        if (selected > 0)
          selected--;
      } else if (event.bstate & MWD) {
        if (selected < file_count - 1)
          selected++;
      }
    } else if (ch >= '0' && ch <= '9') {
      move_count = ch - '0';

      while ((ch = getch()) >= '0' && ch <= '9') {
        if (move_count < 1000000) {
          move_count = move_count * 10 + (ch - '0');
        }
      }

      if (ch == 'j' || ch == KEY_DOWN) {
        selected += move_count;
        if (selected >= file_count)
          selected = file_count - 1;
        continue;
      } else if (ch == 'k' || ch == KEY_UP) {
        selected -= move_count;
        if (selected < 0)
          selected = 0;
        continue;
      }
    } else {
      switch (ch) {
        case k_quit:
          __EXIT;
          return 0;
          break;

        case k_back:
        case KEY_LEFT:
          __BACK;
          break;

        case k_up:
        case KEY_UP:
          __SCROLL_UP;
          break;

        case klong_up:
          if (selected > 0) {
            selected -= 10;
          }
          if (selected < 0)
            selected = 0;
          break;

        case k_down:
        case KEY_DOWN:
          __SCROLL_DOWN;
          break;

        case klong_down:
          if (selected < file_count - 1) {
            selected += 10;
          }
          if (selected >= file_count)
            selected = file_count - 1;
          break;

        case k_tolast:
          __LAST_FILE;
          break;

        case k_goto:
          __GOTO;
          break;

        case k_dot:
        case CONTROL('H'):
          __HIDDEN_FILES;
          break;

        case k_enter:
        case k_forw:
        case KEY_RIGHT:
          __TO_FRWD;
          break;

        case k_mark:
          __MARK_FILE;
          break;
        case k_delete:
        case k_dc:
          __DELETE;
          break;

        case k_mkdir:
          __MAKE_DIR;
          break;

        case k_prev:
          __TO_PREV;
          break;

        case k_mkfile:
          __MAKE_FILE;
          break;

        case k_rename:
          __RENAME;
          break;

        case CONTROL('o'):
        case k_console:
          __CONSOLE;
          break;

        case k_markfmenu:
          __MARK_FILES_MENU;
          break;

        case k_markmhandle:
          __HANDLE_MARKED_FILES;
          break;

        case PAGE_UP:
        case CONTROL('U'):

          __PGUP_HANDLE;
          break;
        case PAGE_DOWN:
        case CONTROL('D'):

          __PGDN_HANDLE;
          break;

        case k_update:
          __UPDATE__;
          break;

        case TAB:
          __TAB_HANDLE;
          break;

        case k_yarn:
          __COPY;
          break;

        case k_cut:
          __CUT;
          break;

        case k_paste:
          __PASTE;
          break;

        case k_search:
          __SEARCH;
          break;

        case k_cmdl:
          __VI;
          break;

        case k_rk:
          line_clear(LINES - 1);
          mvwprintw(stdscr, LINES - 1, 0, "reset 'b'uffer | 't'erminal");
          ch = getch();
          if (ch == 'b') {
            cp_buff_count = 0;
            line_clear(LINES - 1);
            mvwprintw(stdscr, LINES - 1, 0, "reset copy buff");
            gtimeout(500);
          } else if (ch == 't') {
            reset_terminal(0);
            list(path, NULL, false, false);
            line_clear(LINES - 1);
            mvwprintw(stdscr, LINES - 1, 0, "terminal reset");
            gtimeout(500);
            break;
          }
        case k_showpath:
          line_clear(LINES - 1);
          mvwprintw(stdscr, LINES - 1, 0, path);
          gtimeout(2000);
          break;

        case k_Nsearch:
          searchup();
          break;
        case k_nsearch:
          searchdn();
          break;
      }
    }
  }

  endwin();
  free_files();
  return 0;
}
