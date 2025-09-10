
#include <ncurses.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ifm.h"

void goto_cmd(int next_char) {
  char target_path[MAX_PATH];

  switch (next_char) {
  case '/':
    strcpy(target_path, "/");
    break;
  case 'e':
    strcpy(target_path, "/etc/");
    break;
  case 'm': {
    if (access("/media", F_OK) == 0) {
      strcpy(target_path, "/media/");
    } else if (access("/run/media", F_OK) == 0) {
      strcpy(target_path, "/run/media/");
    } else {
      strcpy(target_path, "/");
    }
    break;
  }
  case 'd':
    strcpy(target_path, "/dev/");
    break;
  case 'M':
    strcpy(target_path, "/mnt/");
    break;
  case 't':
    strcpy(target_path, "/tmp/");
    break;
  case 'v':
    strcpy(target_path, "/var/");
    break;
  case 's':
    strcpy(target_path, "/srv/");
    break;
  case '?':

    strcpy(target_path, "/usr/share/doc/ifm/");
    break;
  case 'o':

    strcpy(target_path, "/opt/");
    break;

  case 'r':
    strcpy(target_path, "/run/");
    break;

  case 'c':
    strcpy(target_path, "/sys/");
    break;

  case 'G':
    selected = file_count - 1;
    break;
  case 'g':
    selected = 0;
    break;
  case 'u':

    strcpy(target_path, "/usr/");
    break;
  case 'h':

    to_home();
    return;
  default:
    return;
  }

  if (chdir(target_path) == 0) {
    getcwd(path, sizeof(path));
    list(path, NULL, false, false);
    ;
    selected = 0;
    offset = 0;
  }
}

void goto_help() {
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

  __echo("g", COLS - 1);
  WINDOW *win = newwin(win_height, win_width, start_y, start_x);
  keypad(win, TRUE);

  mvwprintw(win, 1, 0, "key          command");
  mvwhline(win, 2, 0, ACS_HLINE, win_width);
  mvwprintw(win, 3, 0, "?            cd /usr/share/doc/ifm");
  mvwprintw(win, 4, 0, "/            cd /");
  mvwprintw(win, 5, 0, "c            cd /sys");
  mvwprintw(win, 6, 0, "d            cd /dev");
  mvwprintw(win, 7, 0, "e            cd /etc");
  mvwprintw(win, 8, 0, "h            cd ~");
  mvwprintw(win, 9, 0, "m            cd /media or /run/media");
  mvwprintw(win, 10, 0, "g            goto first file");
  mvwprintw(win, 11, 0, "G            goto last file");
  mvwprintw(win, 12, 0, "o            cd /opt");
  mvwprintw(win, 13, 0, "r            cd /run");
  mvwprintw(win, 14, 0, "s            cd /srv");
  mvwprintw(win, 15, 0, "t            cd /tmp");
  mvwprintw(win, 16, 0, "v            cd /var");
  mvwprintw(win, 17, 0, "u            cd /usr");
  mvwprintw(win, 18, 0, "M            cd /mnt");

  wrefresh(win);

  int ch = getch();
  delwin(win);
  touchwin(stdscr);
  refresh();

  if (ch != ERR) {
    goto_cmd(ch);
  }
}
