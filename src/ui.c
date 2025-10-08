
#include "ui.h"

#include <assert.h>
#include <grp.h>
#include <locale.h>
#include <ncurses.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <wchar.h>

#include "./icons/icons.h"
#include "ifm.h"

#define MAX_COLORS 256
static char* color_pairs[MAX_COLORS] = { 0 };
static int next_color_pair = 20;

int
get_hex(const char* hex_color)
{
  if (!hex_color)
    return 0;

  for (int i = 0; i < next_color_pair; i++) {
    if (color_pairs[i] && !strcmp(color_pairs[i], hex_color)) {
      return i + 1;
    }
  }

  if (next_color_pair >= MAX_COLORS)
    return 0;

  const char* hex = (hex_color[0] == '#') ? hex_color + 1 : hex_color;
  int r, g, b;
  if (sscanf(hex, "%02x%02x%02x", &r, &g, &b) != 3)
    return 0;

  init_color(
    next_color_pair + 8, r * 1000 / 255, g * 1000 / 255, b * 1000 / 255);
  init_pair(next_color_pair + 1, next_color_pair + 8, COLOR_BLACK);
  color_pairs[next_color_pair] = strdup(hex_color);

  return next_color_pair++ + 1;
}

void
shorten_path(char* dest, const char* src, int max_width)
{
  if (!dest || !src || max_width <= 0) {
    if (dest)
      *dest = '\0';
    return;
  }

  if (strlen(src) <= (size_t)max_width) {
    strcpy(dest, src);
    return;
  }

  if (strcmp(src, "/") == 0) {
    strcpy(dest, "/");
    return;
  }

  char clean_path[MAX_PATH * 2];
  const char* src_ptr = src;
  char* dst_ptr = clean_path;
  while (*src_ptr) {
    if (*src_ptr == '/' && *(src_ptr + 1) == '/') {
      src_ptr++;
      continue;
    }
    *dst_ptr++ = *src_ptr++;
  }
  *dst_ptr = '\0';

  char* components[MAX_PATH];
  int component_count = 0;
  char* path_copy = strdup(clean_path);
  char* token = strtok(path_copy, "/");
  while (token && component_count < MAX_PATH) {
    components[component_count++] = token;
    token = strtok(NULL, "/");
  }

  if (component_count == 0) {
    strncpy(dest, clean_path, max_width);
    dest[max_width] = '\0';
    free(path_copy);
    return;
  }

  char shortened[MAX_PATH * 2] = { 0 };
  size_t remaining = max_width;

  if (clean_path[0] == '/') {
    strcat(shortened, "/");
    remaining--;
  }

  for (int i = 0; i < component_count - 1; i++) {
    if (remaining < 3)
      break;
    strncat(shortened, components[i], 1);
    strcat(shortened, "/");
    remaining -= 2;
  }

  if (component_count > 0) {
    const char* last = components[component_count - 1];
    strncat(shortened, last, remaining);
  }

  strncpy(dest, shortened, max_width);
  dest[max_width] = '\0';
  free(path_copy);
}

void
display_file_info(const struct stat* st,
                  const char* perms,
                  const struct passwd* pw,
                  const struct group* gr,
                  const char* date,
                  double size,
                  const char* un,
                  const char* filename)
{
  if (!st || !perms || !date || !un) {
    return;
  }

  move(LINES - 1, 0);
  clrtoeol();

  attron(COLOR_PAIR(1));
  printw("%s ", perms);
  attroff(COLOR_PAIR(1));

  attron(COLOR_PAIR(4));
  char info_buff[256];
  snprintf(info_buff,
           sizeof(info_buff),
           "%ld %s %s %.0f%s %s",
           st->st_nlink,
           pw ? pw->pw_name : "?",
           gr ? gr->gr_name : "?",
           size,
           un,
           date);
  printw("%s", info_buff);
  attroff(COLOR_PAIR(4));

  int percentage = (file_count > 1) ? (selected * 100) / (file_count - 1) : 0;
  const char* position = (selected == 0)                ? "Top"
                         : (selected == file_count - 1) ? "Bot"
                                                        : "";

  char position_info[64];
  if (selected == 0) {
    snprintf(position_info,
             sizeof(position_info),
             "%s %d/%d",
             position,
             selected + 1,
             file_count);
  } else if (selected == file_count - 1) {
    snprintf(position_info,
             sizeof(position_info),
             "%s %d/%d",
             position,
             selected + 1,
             file_count);
  } else {
    snprintf(position_info,
             sizeof(position_info),
             "%d%% %d/%d",
             percentage,
             selected + 1,
             file_count);
  }

  if (S_ISLNK(st->st_mode)) {
    char link_target[MAX_PATH];
    char full_path[MAX_PATH];

    snprintf(full_path, sizeof(full_path), "%s/%s", path, filename);
    ssize_t len = readlink(full_path, link_target, sizeof(link_target) - 1);

    if (len != -1) {
      link_target[len] = '\0';

      int info_len = strlen(perms) + 1 + strlen(info_buff);
      int pos_info_len = strlen(position_info);
      int available_space = COLS - info_len - pos_info_len - 5;

      if (available_space > 0) {
        char display_target[MAX_PATH];
        if (strlen(link_target) > available_space) {
          snprintf(display_target,
                   available_space + 1,
                   "%.*s...",
                   available_space - 3,
                   link_target);
        } else {
          strncpy(display_target, link_target, available_space);
        }

        attron(COLOR_PAIR(5));
        printw(" -> %s", display_target);
        attroff(COLOR_PAIR(5));
      }
    }
  }

  char hidden_set[64];

  if (show_hidden == 0) {
    strcpy(hidden_set, "H");
  } else {
    strcpy(hidden_set, " ");
  }

  int right_offset =
    COLS - (int)strlen(position_info) - (int)strlen(hidden_set) - 2;
  if (right_offset > 0) {
    mvprintw(LINES - 1, right_offset, "%s %s", hidden_set, position_info);
  }
}

void
Display()
{
  curs_set(0);
  clear();
  setlocale(LC_ALL, "");

  init_pair(1, COLOR_CYAN, COLOR_BLACK);
  init_pair(2, COLOR_GREEN, COLOR_BLACK);
  init_pair(3, COLOR_YELLOW, COLOR_BLACK);
  init_pair(4, COLOR_WHITE, COLOR_BLACK);
  init_pair(5, COLOR_BLUE, COLOR_BLACK);
  init_pair(6, COLOR_RED, COLOR_BLACK);
  init_pair(7, COLOR_BLACK, COLOR_WHITE);
  init_pair(8, COLOR_MAGENTA, COLOR_BLACK);

  char full_path[MAX_PATH * 2] = { 0 };
  if (file_count > 0 && selected >= 0 && selected < file_count) {
    if (strcmp(path, "/") == 0) {
      snprintf(full_path, sizeof(full_path), "/%s", files[selected]);
    } else {
      snprintf(full_path, sizeof(full_path), "%s/%s", path, files[selected]);
    }
  } else {
    strncpy(full_path, path, sizeof(full_path));
  }

  char clean_path[MAX_PATH * 2];
  char* dst = clean_path;
  const char* src = full_path;
  while (*src) {
    if (*src == '/' && *(src + 1) == '/') {
      src++;
      continue;
    }
    *dst++ = *src++;
  }
  *dst = '\0';

  char display_path[MAX_PATH * 2];
  const char* home = getenv("HOME");
  if (home) {
    if (strcmp(clean_path, home) == 0) {
      strcpy(display_path, "~");
    } else if (strncmp(clean_path, home, strlen(home)) == 0 &&
               clean_path[strlen(home)] == '/') {
      snprintf(
        display_path, sizeof(display_path), "~%s", clean_path + strlen(home));
    } else {
      strncpy(display_path, clean_path, sizeof(display_path));
    }
  } else {
    strncpy(display_path, clean_path, sizeof(display_path));
  }

  char short_path[MAX_PATH * 2];
  shorten_path(short_path, display_path, COLS - 2);

  if (file_count > 0 && selected >= 0 && selected < file_count) {
    char* last_slash = strrchr(short_path, '/');
    if (last_slash) {
      attron(COLOR_PAIR(5) | A_BOLD);
      mvprintw(0, 0, "%.*s", (int)(last_slash - short_path + 1), short_path);
      attroff(COLOR_PAIR(5) | A_BOLD);

      attron(COLOR_PAIR(4) | A_BOLD);
      printw("%s", last_slash + 1);
      attroff(COLOR_PAIR(4) | A_BOLD);
    } else {
      attron(COLOR_PAIR(4) | A_BOLD);
      mvprintw(0, 0, "%s", short_path);
      attroff(COLOR_PAIR(4) | A_BOLD);
    }
  } else {
    attron(COLOR_PAIR(5) | A_BOLD);
    mvprintw(0, 0, "%s", short_path);
    attroff(COLOR_PAIR(5) | A_BOLD);
  }

  int height = LINES - 4;
  int scroll_margin = height / 10;
  if (scroll_margin < 2)
    scroll_margin = 2;

  if (selected < offset) {
    offset = selected;
  } else if (selected >= offset + height) {
    offset = selected - height + 1;
  } else if (selected < offset + scroll_margin) {
    offset = selected - scroll_margin;
    if (offset < 0)
      offset = 0;
  } else if (selected >= offset + height - scroll_margin) {
    offset = selected - height + scroll_margin + 1;
  }

  if (offset > file_count - height) {
    offset = file_count - height;
  }
  if (offset < 0) {
    offset = 0;
  }

  int max_name_width = COLS - 10;
  if (max_name_width < 10)
    max_name_width = 10;

  for (int i = 0; i < height && i + offset < file_count; i++) {
    char item_path[MAX_PATH];
    snprintf(item_path, sizeof(item_path), "%s/%s", path, files[i + offset]);

    int is_marked = 0;
    for (int j = 0; j < MAX_FILES; j++) {
      if (marked_files[j].marked && !strcmp(marked_files[j].path, item_path)) {
        is_marked = 1;
        break;
      }
    }

    IconResult icon = { .icon = "", .color = NULL };
    if (sd != 1) {
      icon = icon_filename(files[i + offset]);
    }

    int color = 4;
    struct stat st;
    if (lstat(item_path, &st) == 0) {
      if (S_ISDIR(st.st_mode)) {
        color = 5;
        if (!strcmp(icon.icon, "")) {
          icon.icon = "";
          icon.color = "#8aacf3";
        }
      } else if (S_ISLNK(st.st_mode)) {
        color = 1;
        struct stat target_st;
        if (stat(item_path, &target_st) == 0 && S_ISDIR(target_st.st_mode)) {
          if (!strcmp(icon.icon, "")) {
            icon.icon = "";
            icon.color = "#8aacf3";
          }
        }
      } else if (S_ISREG(st.st_mode)) {
        if (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) {
          color = 2;
        }
        if (!strcmp(icon.icon, "")) {
          const char* ext = strrchr(files[i + offset], '.');
          if (ext && strlen(ext) > 1) {
            icon = icon_ext(ext + 1);
          }
        }
      }
    }

    int icon_color = color;
    if (icon.color) {
      icon_color = get_hex(icon.color);
      if (!icon_color)
        icon_color = color;
    }

    if (is_marked) {
      attron(COLOR_PAIR(6));
      mvaddch(i + 2, 0, '*');
      attroff(COLOR_PAIR(6));
    } else {
      mvaddch(i + 2, 1, ' ');
    }

    if (sd != 1) {
      attron(COLOR_PAIR(icon_color));
      mvprintw(i + 2, 1, "%s", icon.icon);
      attroff(COLOR_PAIR(icon_color));
    }

    char* name = files[i + offset];
    int name_len = strlen(name);
    char display_name[max_name_width + 4];

    if (name_len > max_name_width) {
      snprintf(
        display_name, sizeof(display_name), "%.*s~", max_name_width - 1, name);
    } else {
      strncpy(display_name, name, max_name_width);
      display_name[max_name_width] = '\0';
    }

    if (i + offset == selected)
      attron(A_REVERSE);
    attron(COLOR_PAIR(color) | A_NORMAL);
    mvprintw(i + 2, 3, "%-*s", max_name_width, display_name);
    attroff(COLOR_PAIR(color) | A_NORMAL);
    if (i + offset == selected)
      attroff(A_REVERSE);
  }

  if (file_count > height) {
    int scrollbar_height = height;
    int scrollbar_pos = (height * offset) / file_count;
    int scrollbar_end = (height * (offset + height)) / file_count;
    if (scrollbar_end >= height)
      scrollbar_end = height - 1;
  }

  if (file_count > 0 && selected >= 0 && selected < file_count) {
    char item_path[MAX_PATH];
    snprintf(item_path, sizeof(item_path), "%s/%s", path, files[selected]);

    struct stat st;
    if (lstat(item_path, &st) == 0) {
      double size = st.st_size;
      const char* un = "B";
      if (size >= 1024) {
        size /= 1024;
        un = "K";
      }
      if (size >= 1024) {
        size /= 1024;
        un = "M";
      }
      if (size >= 1024) {
        size /= 1024;
        un = "G";
      }

      char perms[11];
      snprintf(perms,
               sizeof(perms),
               "%c%c%c%c%c%c%c%c%c%c",
               S_ISLNK(st.st_mode)    ? 'l'
               : S_ISDIR(st.st_mode)  ? 'd'
               : S_ISCHR(st.st_mode)  ? 'c'
               : S_ISBLK(st.st_mode)  ? 'b'
               : S_ISFIFO(st.st_mode) ? 'p'
               : S_ISSOCK(st.st_mode) ? 's'
                                      : '-',
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

      struct passwd* pw = getpwuid(st.st_uid);
      struct group* gr = getgrgid(st.st_gid);

      display_file_info(&st, perms, pw, gr, date, size, un, files[selected]);
    }
  } else {
    attron(COLOR_PAIR(3));
    move(LINES - 1, 0);
    clrtoeol();
    attroff(COLOR_PAIR(3));
  }

  refresh();
}
