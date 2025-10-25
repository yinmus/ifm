
#include <dirent.h>
#include <libgen.h>
#include <limits.h>
#include <magic.h>
#include <ncurses.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "ifm.h"

void
mark_help()
{
  __echo("e", COLS - 1);

#ifndef NOHINTS
  int win_height = LINES * 0.8;
  if (win_height < 10) {
    win_height = 10;
  } else if (win_height > LINES) {
    win_height = LINES;
  }

  int win_width = COLS;
  int start_y = LINES - win_height;
  if (start_y < 0) {
    start_y = 0;
  }
  int start_x = 0;

  WINDOW* win = newwin(win_height, win_width, start_y, start_x);
  keypad(win, TRUE);

  mvwprintw(win, 1, 0, "key          command");
  mvwhline(win, 2, 0, ACS_HLINE, win_width);
  mvwprintw(win, 3, 0, "G            mark from current to end");
  mvwprintw(win, 4, 0, "g            mark from current to start");
  mvwprintw(win, 5, 0, "j            mark next 5 files");
  mvwprintw(win, 6, 0, "k            mark previous 5 files");
  mvwprintw(win, 7, 0, "a            mark all files in directory");
  mvwprintw(win, 8, 0, "u            unmark");

  mvwprintw(win, 10, 0, "d            delete marked files");
  mvwprintw(win, 11, 0, "r            rename marked files");

  wrefresh(win);
  
#endif // endif NOHINTS

  int ch = getch();
  int next_ch = 0;

  if (ch == 'u') {
    __echo("eu", COLS - 2);
  #ifndef NOHINTS
    delwin(win);
    touchwin(stdscr);
    refresh();

    WINDOW* hint_win = newwin(win_height, win_width, start_y, start_x);
    mvwprintw(hint_win, 1, 0, "key          command");
    mvwhline(hint_win, 2, 0, ACS_HLINE, win_width);
    mvwprintw(hint_win, 3, 0, "G            unmark from current to end");
    mvwprintw(hint_win, 4, 0, "g            unmark from current to start");
    mvwprintw(hint_win, 5, 0, "j            unmark next 5 files");
    mvwprintw(hint_win, 6, 0, "k            unmark previous 5 files");
    mvwprintw(hint_win, 7, 0, "A            unmark all files");
    mvwprintw(hint_win, 8, 0, "a            unmark all files in directory");

    wrefresh(hint_win);
    next_ch = getch();
    delwin(hint_win);

    touchwin(stdscr);
    refresh();
  #else
    next_ch = getch();
  #endif
  }

  if (ch != ERR) {
    switch (ch) {
      case 'G':
        if (next_ch == 0) {
          for (int i = selected; i < file_count; i++) {
            char full_path[MAX_PATH];
            snprintf(full_path, sizeof(full_path), "%s/%s", path, files[i]);

            int already_marked = 0;
            for (int j = 0; j < MAX_FILES; j++) {
              if (marked_files[j].marked &&
                  strcmp(marked_files[j].path, full_path) == 0) {
                already_marked = 1;
                selected = file_count - 1;
                break;
              }
            }

            if (!already_marked) {
              for (int j = 0; j < MAX_FILES; j++) {
                if (!marked_files[j].marked) {
                  strncpy(marked_files[j].path, full_path, MAX_PATH);
                  marked_files[j].marked = 1;
                  selected = file_count - 1;
                  break;
                }
              }
            }
          }
        }
        break;

      case 'g': {
        for (int i = selected; i >= 0; i--) {
          char full_path[MAX_PATH];
          snprintf(full_path, sizeof(full_path), "%s/%s", path, files[i]);

          int already_marked = 0;
          for (int j = 0; j < MAX_FILES; j++) {
            if (marked_files[j].marked &&
                strcmp(marked_files[j].path, full_path) == 0) {
              already_marked = 1;
              break;
            }
          }

          if (!already_marked) {
            for (int j = 0; j < MAX_FILES; j++) {
              if (!marked_files[j].marked) {
                strncpy(marked_files[j].path, full_path, MAX_PATH);
                marked_files[j].marked = 1;
                selected = 0;
                break;
              }
            }
          }
        }
        break;
      }

      case 'a':
        if (next_ch == 0) {
          for (int i = 0; i < file_count; i++) {
            char full_path[MAX_PATH];
            snprintf(full_path, sizeof(full_path), "%s/%s", path, files[i]);

            int already_marked = 0;
            for (int j = 0; j < MAX_FILES; j++) {
              if (marked_files[j].marked &&
                  strcmp(marked_files[j].path, full_path) == 0) {
                already_marked = 1;
                break;
              }
            }

            if (!already_marked) {
              for (int j = 0; j < MAX_FILES; j++) {
                if (!marked_files[j].marked) {
                  strncpy(marked_files[j].path, full_path, MAX_PATH);
                  marked_files[j].marked = 1;
                  break;
                }
              }
            }
          }
        }
        break;

      case 'u': {
        if (next_ch == 'G') {
          for (int i = selected; i < file_count; i++) {
            char full_path[MAX_PATH];
            snprintf(full_path, sizeof(full_path), "%s/%s", path, files[i]);

            for (int j = 0; j < MAX_FILES; j++) {
              if (marked_files[j].marked &&
                  strcmp(marked_files[j].path, full_path) == 0) {
                marked_files[j].marked = 0;
                memset(marked_files[j].path, 0, MAX_PATH);
                selected = file_count - 1;
                break;
              }
            }
          }
        } else if (next_ch == 'g') {
          for (int i = selected; i >= 0; i--) {
            char full_path[MAX_PATH];
            snprintf(full_path, sizeof(full_path), "%s/%s", path, files[i]);

            for (int j = 0; j < MAX_FILES; j++) {
              if (marked_files[j].marked &&
                  strcmp(marked_files[j].path, full_path) == 0) {
                marked_files[j].marked = 0;
                memset(marked_files[j].path, 0, MAX_PATH);
                selected = 0;
                break;
              }
            }
          }
        } else if (next_ch == 'j') {
          int end = selected + 5;
          if (end >= file_count)
            end = file_count - 1;

          for (int i = selected; i <= end; i++) {
            char full_path[MAX_PATH];
            snprintf(full_path, sizeof(full_path), "%s/%s", path, files[i]);

            for (int j = 0; j < MAX_FILES; j++) {
              if (marked_files[j].marked &&
                  strcmp(marked_files[j].path, full_path) == 0) {
                marked_files[j].marked = 0;
                memset(marked_files[j].path, 0, MAX_PATH);
                break;
              }
            }
          }
          selected =
            (selected + 5 < file_count) ? selected + 5 : file_count - 1;
        } else if (next_ch == 'k') {
          int start = selected - 5;
          if (start < 0)
            start = 0;

          for (int i = selected; i >= start; i--) {
            char full_path[MAX_PATH];
            snprintf(full_path, sizeof(full_path), "%s/%s", path, files[i]);

            for (int j = 0; j < MAX_FILES; j++) {
              if (marked_files[j].marked &&
                  strcmp(marked_files[j].path, full_path) == 0) {
                marked_files[j].marked = 0;
                memset(marked_files[j].path, 0, MAX_PATH);
                break;
              }
            }
          }
          selected = (selected - 5 >= 0) ? selected - 5 : 0;
        }

        else if (next_ch == 'A') {
          for (int i = 0; i < MAX_FILES; i++) {
            marked_files[i].marked = 0;
            memset(marked_files[i].path, 0, MAX_PATH);
          }
        } else if (next_ch == 'a') {
          for (int i = 0; i < file_count; i++) {
            char full_path[MAX_PATH];
            snprintf(full_path, sizeof(full_path), "%s/%s", path, files[i]);

            for (int j = 0; j < MAX_FILES; j++) {
              if (marked_files[j].marked &&
                  strcmp(marked_files[j].path, full_path) == 0) {
                marked_files[j].marked = 0;
                memset(marked_files[j].path, 0, MAX_PATH);
                break;
              }
            }
          }
        }
        break;
      }

      case 'j': {
        int end = selected + 5;
        if (end >= file_count)
          end = file_count - 1;

        for (int i = selected; i <= end; i++) {
          char full_path[MAX_PATH];
          snprintf(full_path, sizeof(full_path), "%s/%s", path, files[i]);

          int already_marked = 0;
          for (int j = 0; j < MAX_FILES; j++) {
            if (marked_files[j].marked &&
                strcmp(marked_files[j].path, full_path) == 0) {
              already_marked = 1;
              break;
            }
          }

          if (!already_marked) {
            for (int j = 0; j < MAX_FILES; j++) {
              if (!marked_files[j].marked) {
                strncpy(marked_files[j].path, full_path, MAX_PATH);
                marked_files[j].marked = 1;
                break;
              }
            }
          }
        }
        selected = (selected + 5 < file_count) ? selected + 5 : file_count - 1;
        break;
      }

      case 'k': {
        int start = selected - 5;
        if (start < 0)
          start = 0;

        for (int i = selected; i >= start; i--) {
          char full_path[MAX_PATH];
          snprintf(full_path, sizeof(full_path), "%s/%s", path, files[i]);

          int already_marked = 0;
          for (int j = 0; j < MAX_FILES; j++) {
            if (marked_files[j].marked &&
                strcmp(marked_files[j].path, full_path) == 0) {
              already_marked = 1;
              break;
            }
          }

          if (!already_marked) {
            for (int j = 0; j < MAX_FILES; j++) {
              if (!marked_files[j].marked) {
                strncpy(marked_files[j].path, full_path, MAX_PATH);
                marked_files[j].marked = 1;
                break;
              }
            }
          }
        }
        selected = (selected - 5 >= 0) ? selected - 5 : 0;
        break;
      }

      case 'd': {
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
            list(path, NULL, false, false);
            ;
            selected = 0;
          }
        }
        memset(marked_files, 0, sizeof(marked_files));
        break;
      }

      case 'r': {
        for (int i = 0; i < MAX_FILES; i++) {
          if (marked_files[i].marked) {
            char* filename = basename(marked_files[i].path);
            char dir[MAX_PATH];
            strncpy(dir, marked_files[i].path, MAX_PATH);
            dirname(dir);

            char new_name[MAX_NAME];
            strncpy(new_name, filename, MAX_NAME);
            if (cpe(new_name, MAX_NAME, "Rename to: ")) {
              char new_path[MAX_PATH];
              snprintf(new_path, sizeof(new_path), "%s/%s", dir, new_name);
              selected = 0;

              if (rename(marked_files[i].path, new_path) == 0) {
                strncpy(marked_files[i].path, new_path, MAX_PATH);
              }
            }
          }
        }
        list(path, NULL, false, false);
        ;
        memset(marked_files, 0, sizeof(marked_files));
        break;
      }
    }
  }
}

void
show_marked_files()
{
  int count = 0;

  for (int i = 0; i < MAX_FILES; i++) {
    if (marked_files[i].marked) {
      count++;
    }
  }

  if (!count) {
    line_clear(LINES - 1);
    mvprintw(LINES - 1, 0, "No files marked");
    refresh();

    gtimeout(700);

    return;
  }

  char tmp_file[] = "/tmp/ifmXXXXXX";
  int fd = mkstemp(tmp_file);
  if (fd == -1) {
    mvprintw(LINES - 1, 0, "Failed to create temporary file");
    refresh();
    sleep(1);
    return;
  }

  FILE* tmp_fp = fdopen(fd, "w");
  if (!tmp_fp) {
    close(fd);
    unlink(tmp_file);
    mvprintw(LINES - 1, 0, "Failed to open temporary file");
    refresh();
    sleep(1);
    return;
  }

  for (int i = 0; i < MAX_FILES; i++) {
    if (marked_files[i].marked) {
      struct stat st;
      if (stat(marked_files[i].path, &st) == 0) {
        char perms[11];
        snprintf(perms,
                 sizeof(perms),
                 "%c%c%c%c%c%c%c%c%c%c",
                 S_ISDIR(st.st_mode) ? 'd' : '-',
                 st.st_mode & S_IRUSR ? 'r' : '-',
                 st.st_mode & S_IWUSR ? 'w' : '-',
                 st.st_mode & S_IXUSR ? 'x' : '-',
                 st.st_mode & S_IRGRP ? 'r' : '-',
                 st.st_mode & S_IWGRP ? 'w' : '-',
                 st.st_mode & S_IXGRP ? 'x' : '-',
                 st.st_mode & S_IROTH ? 'r' : '-',
                 st.st_mode & S_IWOTH ? 'w' : '-',
                 st.st_mode & S_IXOTH ? 'x' : '-');

        char date[20];
        strftime(date, sizeof(date), "%Y-%m-%d %H:%M", localtime(&st.st_mtime));

        fprintf(
          tmp_fp, "%-80s %-10s %-16s\n", marked_files[i].path, perms, date);
      }
    }
  }

  fclose(tmp_fp);

  char command[4096];
  snprintf(command, sizeof(command), "less %s", tmp_file);

  endwin();
  system(command);
  initscr();

  unlink(tmp_file);
  refresh();
}
