#include "ifm.h"

#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <locale.h>
#include <magic.h>
#include <ncurses.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <wchar.h>

#include "config.h"
#include "fmh.h"
#include "goto.h"
#include "ui.h"

static bool cp_buff_ct[MAX_COPY_FILES];
char cp_buff[MAX_COPY_FILES][MAX_PATH];
int cp_buff_count = 0;

static int search_count = 0;
static int search_index = 0;
static char search_results[MAX_FILES][MAX_PATH];

char** files = NULL;
int file_count = 0;
int selected = 0;
int offset = 0;
char path[MAX_PATH];
char lpath[MAX_PATH];

int s_hidden = 0;
int sd = 1;
MarkedFile marked_files[MAX_FILES] = { 0 };
void
init_files()
{
  if (files == NULL) {
    files = malloc(MAX_FILES * sizeof(char*));
    if (files == NULL) {
      perror("Failed to allocate files array");
      exit(EXIT_FAILURE);
    }

    for (int i = 0; i < MAX_FILES; i++) {
      files[i] = malloc(MAX_PATH * sizeof(char));
      if (files[i] == NULL) {
        perror("Failed to allocate file path");
        for (int j = 0; j < i; j++) {
          free(files[j]);
        }
        free(files);
        files = NULL;
        exit(EXIT_FAILURE);
      }
      files[i][0] = '\0';
    }
  }
}

void
free_files()
{
  if (files != NULL) {
    for (int i = 0; i < MAX_FILES; i++) {
      free(files[i]);
    }
    free(files);
    files = NULL;
  }
}

void
resize_files(int new_size)
{
  char** new_files = realloc(files, new_size * sizeof(char*));
  if (new_files != NULL) {
    files = new_files;
    for (int i = file_count; i < new_size; i++) {
      files[i] = malloc(MAX_PATH * sizeof(char));
      files[i][0] = '\0';
    }
  }
}
void
line_clear(int y)
{
  move(y, 0);
  clrtoeol();
  refresh();
}

void
list(const char* dir_path, const char* filter, bool show_dirs, bool show_files)
{
  assert(dir_path != NULL && "dir_path cannot be NULL");

  if (files == NULL) {
    init_files();
  }

  DIR* dir = opendir(dir_path);
  if (!dir) {
    return;
  }

  file_count = 0;

  if (access(dir_path, R_OK | X_OK) != 0) {
    closedir(dir);
    return;
  }

  bool show_all = !show_dirs && !show_files;

  struct dirent* entry;
  while ((entry = readdir(dir)) != NULL && file_count < MAX_FILES) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    if (!s_hidden && entry->d_name[0] == '.')
      continue;

    char full_path[MAX_PATH];
    snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

    struct stat st;
    if (stat(full_path, &st) != 0)
      continue;

    bool is_dir = S_ISDIR(st.st_mode);

    if (!show_all) {
      if (show_dirs && !is_dir)
        continue;
      if (show_files && is_dir)
        continue;
    }

    if (filter != NULL && !CASE_INSENSITIVE_STRSTR(entry->d_name, filter)) {
      continue;
    }

    strncpy(files[file_count], entry->d_name, MAX_PATH);
    file_count++;
  }
  closedir(dir);

  qsort(files, file_count, sizeof(char*), compare);
}

void
rm(const char* path)
{
  struct stat st;

  if (lstat(path, &st) != 0) {
    mvprintw(LINES - 1, 0, "E: Cannot access '%s' - %s", path, strerror(errno));
    refresh();
    getch();
    return;
  }

  if (S_ISLNK(st.st_mode)) {
    if (unlink(path) != 0) {
      mvprintw(LINES - 1,
               0,
               "E: Cannot remove symlink '%s' - %s",
               path,
               strerror(errno));
      refresh();
      getch();
    }
    return;
  }

  if (S_ISDIR(st.st_mode)) {
    DIR* dir = opendir(path);
    if (!dir) {
      mvprintw(LINES - 1,
               0,
               "E: Cannot open directory '%s' - %s",
               path,
               strerror(errno));
      refresh();
      getch();
      return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
      if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
        continue;
      }

      char full_path[MAX_PATH];
      snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

      rm(full_path);
    }
    closedir(dir);

    if (rmdir(path) != 0) {
      if (errno == EACCES) {
        if (chmod(path, 0700) == 0) {
          if (rmdir(path) != 0) {
            mvprintw(LINES - 1,
                     0,
                     "E: Cannot remove directory '%s' - %s",
                     path,
                     strerror(errno));
            refresh();
            getch();
          }
        } else {
          mvprintw(LINES - 1,
                   0,
                   "E: Cannot change permissions for '%s' - %s",
                   path,
                   strerror(errno));
          refresh();
          getch();
        }
      } else {
        mvprintw(LINES - 1,
                 0,
                 "E: Cannot remove directory '%s' - %s",
                 path,
                 strerror(errno));
        refresh();
        getch();
      }
    }
  } else {
    if (remove(path) != 0) {
      if (errno == EACCES) {
        if (chmod(path, 0600) == 0) {
          if (remove(path) != 0) {
            mvprintw(LINES - 1,
                     0,
                     "E: Cannot remove file '%s' - %s",
                     path,
                     strerror(errno));
            refresh();
            getch();
          }
        } else {
          mvprintw(LINES - 1,
                   0,
                   "E: Cannot change permissions for '%s' - %s",
                   path,
                   strerror(errno));
          refresh();
          getch();
        }
      } else {
        mvprintw(LINES - 1,
                 0,
                 "E: Cannot remove file '%s' - %s",
                 path,
                 strerror(errno));
        refresh();
        getch();
      }
    }
  }
}

void
open_file(const char* filename)
{
  assert(filename != NULL && "filename cannot be NULL");
  assert(strlen(filename) > 0 && "filename cannot be empty");

  char full_path[MAX_PATH];
  snprintf(full_path, sizeof(full_path), "%s/%s", path, filename);

  if (access(full_path, R_OK) != 0) {
    line_clear(LINES - 1);
    mvprintw(LINES - 1, 0, "E: no read permission for '%s'", filename);
    refresh();
    gtimeout(1000);
    return;
  }

  struct stat st;
  if (lstat(full_path, &st) == 0 && S_ISLNK(st.st_mode)) {
    char target_path[MAX_PATH];
    ssize_t len = readlink(full_path, target_path, sizeof(target_path) - 1);
    if (len != -1) {
      target_path[len] = '\0';
      if (stat(target_path, &st) == 0 && S_ISDIR(st.st_mode)) {
        strncpy(path, full_path, sizeof(path));
        list(path, NULL, false, false);
        ;
        selected = 0;
        offset = 0;
        return;
      }
    }
  }

  if (stat(full_path, &st) == 0 && st.st_size == 0) {
    const char* editor = getenv("EDITOR");
    if (editor) {
      def_prog_mode();
      endwin();
      char command[MAX_PATH + 50];
      snprintf(command, sizeof(command), "%s \"%s\"", editor, full_path);
      system(command);
      reset_prog_mode();
      refresh();
    } else {
      char cmd[MAX_PATH + 20];
      snprintf(cmd, sizeof(cmd), "xdg-open \"%s\" &", full_path);
      system(cmd);
    }
    list(path, NULL, false, false);
    ;
    reset_terminal(0);
    return;
  }

  magic_t magic = magic_open(MAGIC_MIME_TYPE);
  if (!magic) {
    char cmd[MAX_PATH + 20];
    snprintf(cmd, sizeof(cmd), "xdg-open \"%s\" &", full_path);
    system(cmd);
    return;
  }

  if (magic_load(magic, NULL) != 0) {
    magic_close(magic);
    char cmd[MAX_PATH + 20];
    snprintf(cmd, sizeof(cmd), "xdg-open \"%s\" &", full_path);
    system(cmd);
    return;
  }

  const char* mime_type = magic_file(magic, full_path);
  const char* editor = getenv("EDITOR");
  char command[MAX_PATH + 50];

  if (mime_type && strstr(mime_type, "text/") && editor) {
    def_prog_mode();
    endwin();
    snprintf(command, sizeof(command), "%s \"%s\"", editor, full_path);
    system(command);
    reset_prog_mode();
    refresh();
  } else {
    snprintf(command, sizeof(command), "xdg-open \"%s\" &", full_path);
    system(command);
  }

  magic_close(magic);
  list(path, NULL, false, false);
  reset_terminal(0);
}

void
cr_file()
{
  char filename[MAX_NAME] = { 0 };
  if (cpe(filename, MAX_NAME, "[path]/file name: ")) {
    assert(strlen(filename) > 0 && "filename cannot be empty");

    char full_path[MAX_PATH];
    snprintf(full_path, sizeof(full_path), "%s/%s", path, filename);
    assert(strlen(full_path) < MAX_PATH && "path too long");

    struct stat st;
    if (stat(full_path, &st) == 0) {
      attron(COLOR_PAIR(6) | A_BOLD);

      mvprintw(LINES - 1, 0, "E: File or directory already exists");
      attroff(COLOR_PAIR(6) | A_BOLD);

      getch();
      return;
    }

    FILE* file = fopen(full_path, "w");
    if (file == NULL) {
      mvprintw(LINES - 1,
               0,
               "E: Cannot change permissions for '%s' - %s",
               path,
               strerror(errno));
      refresh();
      gtimeout(500);
      return;
    }
    if (file)
      fclose(file);

    list(path, NULL, false, false);
    ;

    for (int i = 0; i < file_count; i++) {
      if (strcmp(files[i], filename) == 0) {
        selected = i;
        break;
      }
    }
  }
}

void
gtimeout(int ms)
{
  timeout(ms);
  int ch = getch();
  timeout(-1);
}

void
cr_dir()
{
  char dirname[MAX_NAME] = { 0 };
  if (cpe(dirname, MAX_NAME, "[path]/dir name: ")) {
    if (strlen(dirname) == 0) {
      mvprintw(LINES - 1, 0, "E: Directory name cannot be empty");
      return;
    }

    char full_path[MAX_PATH];
    snprintf(full_path, sizeof(full_path), "%s/%s", path, dirname);
    if (strlen(full_path) >= MAX_PATH) {
      mvprintw(LINES - 1, 0, "E: Path too long");
      gtimeout(500);
      return;
    }

    struct stat st;
    if (stat(full_path, &st) == 0) {
      attron(COLOR_PAIR(6) | A_BOLD);

      mvprintw(LINES - 1, 0, "E: File or directory already exists");
      attroff(COLOR_PAIR(6) | A_BOLD);
      gtimeout(500);
      return;
    }

    int result = mkdir(full_path, 0777);
    if (result != 0) {
      if (errno == EACCES) {
        mvprintw(LINES - 1,
                 0,
                 "E: Cannot change permissions for '%s' - %s",
                 path,
                 strerror(errno));
        refresh();
        gtimeout(500);
      } else {
        mvprintw(
          LINES - 1, 0, "E: Cannot create directory E:%s", strerror(errno));
      }
      gtimeout(500);
      return;
    }

    list(path, NULL, false, false);
    ;

    for (int i = 0; i < file_count; i++) {
      if (strcmp(files[i], dirname) == 0) {
        selected = i;
        break;
      }
    }
  }
}

void
ren(const char* filename)
{
  assert(filename != NULL && "filename cannot be NULL");
  assert(strlen(filename) > 0 && "filename cannot be empty");

  setlocale(LC_ALL, "");

  char new_name[MAX_NAME];
  strncpy(new_name, filename, MAX_NAME - 1);
  new_name[MAX_NAME - 1] = '\0';

  move(LINES - 1, 0);
  clrtoeol();
  refresh();

  if (!cpe(new_name, MAX_NAME, "rename: ")) {
    return;
  }

  if (strlen(new_name) > 0 && strcmp(new_name, filename) != 0) {
    char old_path[MAX_PATH];
    char new_path[MAX_PATH];
    snprintf(old_path, sizeof(old_path), "%s/%s", path, filename);
    snprintf(new_path, sizeof(new_path), "%s/%s", path, new_name);
    assert(strlen(old_path) < MAX_PATH && "old path too long");
    assert(strlen(new_path) < MAX_PATH && "new path too long");

    if (access(new_path, F_OK) == 0) {
      mvprintw(LINES - 1, 0, "E: File already exists");
      gtimeout(500);

      return;
    }

    int result = rename(old_path, new_path);
    assert(result == 0 && "rename failed");
    if (result != 0) {
      mvprintw(LINES - 1, 0, "E: Rename failed");
      gtimeout(500);
    } else {
      list(path, NULL, false, false);
      ;
      for (int i = 0; i < file_count; i++) {
        if (strcmp(files[i], new_name) == 0) {
          selected = i;
          break;
        }
      }
    }
  }
}

void
console(const char* filename)
{
  char full_path[MAX_PATH];
  snprintf(full_path, sizeof(full_path), "%s/%s", path, filename);

  if (access(full_path, R_OK) != 0) {
    mvprintw(LINES - 1, 0, "E: Cannot access file");
    gtimeout(1000);
    return;
  }

  char command[MAX_PATH] = { 0 };
  if (!cpe(command, MAX_PATH, ":")) {
    line_clear(LINES - 1);
    refresh();
    return;
  }

  line_clear(LINES - 1);
  refresh();

  if (strlen(command) == 0) {
    list(path, NULL, false, false);
    return;
  }

  char full_command[MAX_PATH * 3];
  char* dollar_pos = strchr(command, '$');
  char* nosel;

  nosel = strstr(command, "%n");

  if (dollar_pos != NULL) {
    int prefix_len = dollar_pos - command;
    char prefix[MAX_PATH];
    char suffix[MAX_PATH];

    strncpy(prefix, command, prefix_len);
    prefix[prefix_len] = '\0';

    strcpy(suffix, dollar_pos + 1);

    snprintf(full_command,
             sizeof(full_command),
             "%s\"%s\"%s",
             prefix,
             full_path,
             suffix);
  } else if (nosel != NULL) {
    size_t len = strlen("%n");
    memmove(nosel, nosel + len, strlen(nosel + len) + 1);

    snprintf(full_command, sizeof(full_command), "%s", command);
  } else {
    snprintf(
      full_command, sizeof(full_command), "%s \"%s\"", command, full_path);
  }
  def_prog_mode();
  endwin();

  int pr = system(full_command);

  reset_prog_mode();
  refresh();
  list(path, NULL, false, false);
}

void
to_back()
{
  strncpy(lpath, path, sizeof(lpath));

  if (strcmp(path, "/") == 0) {
    line_clear(LINES - 1);
    mvprintw(LINES - 1, 0, "E: already at root directory");
    refresh();
    gtimeout(500);
    return;
  }

  char* last_slash = strrchr(path, '/');
  if (!last_slash) {
    line_clear(LINES - 1);
    mvprintw(LINES - 1, 0, "E: invalid path format");
    refresh();
    gtimeout(500);
    return;
  }

  if (last_slash == path) {
    path[1] = '\0';
  } else {
    *last_slash = '\0';
  }

  if (chdir(path) != 0) {
    strncpy(path, lpath, sizeof(path));
    mvprintw(LINES - 1, 0, "E: Cannot access parent directory");
    refresh();
    gtimeout(500);
    return;
  }

  list(path, NULL, false, false);
  ;
  selected = 0;

  char* old_dir_name = basename(lpath);
  for (int i = 0; i < file_count; i++) {
    if (strcmp(files[i], old_dir_name) == 0) {
      selected = i;
      break;
    }
  }

  int height = LINES - 4;
  offset = (selected > height / 2) ? selected - height / 2 : 0;
  if (offset > file_count - height) {
    offset = file_count - height;
  }
  if (offset < 0)
    offset = 0;
}

void
to_home()
{
  if (strcmp(path, "/") != 0) {
    strncpy(lpath, path, sizeof(lpath));
  }

  const char* home = getenv("HOME");
  if (home) {
    strncpy(path, home, sizeof(path));
    if (chdir(path) != 0) {
      mvprintw(LINES - 1, 0, "E: Cannot access home directory");
      refresh();
      gtimeout(500);
      return;
    }
    list(path, NULL, false, false);
    ;
    selected = 0;
    offset = 0;
  } else {
    mvprintw(LINES - 1, 0, "E: HOME environment variable not set");
    refresh();
    gtimeout(500);
  }
}

int
cpe(char* buffer, int max_len, const char* prompt)
{
  echo();
  curs_set(1);

  int pos = strlen(buffer);

  while (1) {
    line_clear(LINES - 1);
    clrtoeol();
    mvprintw(LINES - 1, 0, "%s%s", prompt, buffer);

    int cursor_pos = 0;
    for (int i = 0; i < pos; i++) {
      if ((buffer[i] & 0xC0) != 0x80) {
        cursor_pos++;
      }
    }
    move(LINES - 1, strlen(prompt) + cursor_pos);
    refresh();

    int ch = getch();

    if (ch == '\n' || ch == KEY_ENTER) {
      break;
    } else if (ch == ESC) {
      noecho();
      curs_set(0);
      return 0;
    } else if (ch == 127 || ch == KEY_BACKSPACE) {
      if (pos > 0) {
        int char_len = 1;
        while (pos - char_len >= 0 && (buffer[pos - char_len] & 0xC0) == 0x80) {
          char_len++;
        }
        pos -= char_len;
        memmove(&buffer[pos],
                &buffer[pos + char_len],
                strlen(buffer) - pos - char_len + 1);
      }
    } else if (ch == KEY_DC) {
      if (pos < strlen(buffer)) {
        int char_len = 1;
        while ((buffer[pos + char_len] & 0xC0) == 0x80) {
          char_len++;
        }
        memmove(&buffer[pos],
                &buffer[pos + char_len],
                strlen(buffer) - pos - char_len + 1);
      }
    } else if (ch == KEY_LEFT) {
      if (pos > 0) {
        do {
          pos--;
        } while (pos > 0 && (buffer[pos] & 0xC0) == 0x80);
      }
    } else if (ch == KEY_RIGHT) {
      if (pos < strlen(buffer)) {
        do {
          pos++;
        } while (pos < strlen(buffer) && (buffer[pos] & 0xC0) == 0x80);
      }
    } else if (ch == 23) {
      if (pos > 0) {
        int word_start = pos;
        while (word_start > 0 && buffer[word_start - 1] == ' ') {
          word_start--;
        }
        while (word_start > 0 && buffer[word_start - 1] != ' ') {
          word_start--;
        }
        memmove(&buffer[word_start], &buffer[pos], strlen(buffer) - pos + 1);
        pos = word_start;
      }
    } else if (ch >= 32 && ch <= 126 || ch >= 128) {
      if (strlen(buffer) < max_len - 1) {
        memmove(&buffer[pos + 1], &buffer[pos], strlen(buffer) - pos + 1);
        buffer[pos++] = ch;
      }
    }
  }

  noecho();
  curs_set(0);
  return 1;
}

int
confrim_delete(const char* filename)
{
  curs_set(1);
  noecho();

  move(LINES - 1, 0);
  clrtoeol();
  refresh();

  mvprintw(LINES - 1, 0, "Confirm deletion of: %s (y/N): ", filename);
  refresh();

  int confirmed = 0;
  while (1) {
    int ch = getch();

    if (ch == 'y' || ch == 'Y') {
      confirmed = 1;
      break;
    } else if (ch == 'n' || ch == 'N' || ch == ESC || ch == '\n') {
      confirmed = 0;
      break;
    }
  }

  curs_set(0);

  line_clear(LINES - 1);
  clrtoeol();
  refresh();

  return confirmed;
}

void
resolve_path(const char* input, char* output, size_t size)
{
  if (!input || !output || size == 0)
    return;

  if (input[0] == '/') {
    strncpy(output, input, size - 1);
    output[size - 1] = '\0';
  } else {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
      snprintf(output, size, "%s/%s", cwd, input);
    } else {
      fprintf(stderr, "resolve_path: getcwd failed: %s\n", strerror(errno));
      strncpy(output, input, size - 1);
      output[size - 1] = '\0';
    }
  }
}

void
search()
{
  assert(path != NULL && "path cannot be NULL");
  assert(strlen(path) > 0 && "path cannot be empty");

  char filter[MAX_NAME] = { 0 };
  search_count = 0;

  if (cpe(filter, MAX_NAME, "/")) {
    if (strlen(filter) > 0) {
      DIR* dir = opendir(path);
      if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL && search_count < MAX_FILES) {
          if (strcmp(entry->d_name, ".") == 0 ||
              strcmp(entry->d_name, "..") == 0)
            continue;

          if (!s_hidden && entry->d_name[0] == '.')
            continue;

          if (CASE_INSENSITIVE_STRSTR(entry->d_name, filter)) {
            strncpy(search_results[search_count++], entry->d_name, MAX_PATH);
          }
        }
        closedir(dir);
      }

      if (search_count > 0) {
        search_index = 0;
        for (int i = 0; i < file_count; i++) {
          if (strcmp(files[i], search_results[search_index]) == 0) {
            selected = i;
            break;
          }
        }
      } else {
        line_clear(LINES - 1);
        mvprintw(LINES - 1, 0, "No matches found");
        refresh();
        gtimeout(500);
      }
    } else {
      list(path, NULL, false, false);
    }
  }
}

void
searchdn()
{
  if (search_count == 0)
    return;

  int start_pos = selected + 1;
  for (int i = start_pos; i < file_count; i++) {
    for (int j = 0; j < search_count; j++) {
      if (strcmp(files[i], search_results[j]) == 0) {
        selected = i;
        search_index = j;
        goto ad_view;
      }
    }
  }

  for (int i = 0; i <= selected; i++) {
    for (int j = 0; j < search_count; j++) {
      if (strcmp(files[i], search_results[j]) == 0) {
        selected = i;
        search_index = j;
        goto ad_view;
      }
    }
  }

ad_view:
  int height = LINES - 4;
  if (selected >= offset + height) {
    offset = selected - height + 1;
  }
  if (selected < offset) {
    offset = selected;
  }
}

void
searchup()
{
  if (search_count == 0)
    return;

  int start_pos = selected - 1;
  for (int i = start_pos; i >= 0; i--) {
    for (int j = 0; j < search_count; j++) {
      if (strcmp(files[i], search_results[j]) == 0) {
        selected = i;
        search_index = j;
        goto ad_view;
      }
    }
  }

  for (int i = file_count - 1; i >= selected; i--) {
    for (int j = 0; j < search_count; j++) {
      if (strcmp(files[i], search_results[j]) == 0) {
        selected = i;
        search_index = j;
        goto ad_view;
      }
    }
  }

ad_view:
  int height = LINES - 4;
  if (selected >= offset + height) {
    offset = selected - height + 1;
  }
  if (selected < offset) {
    offset = selected;
  }
}

void
handle_archive(const char* filename)
{
  char full_path[MAX_PATH];
  snprintf(full_path, sizeof(full_path), "%s/%s", path, filename);

  char tmp_path[] = "/tmp/.ifmXXXXXX";
  int tmp_fd = mkstemp(tmp_path);
  if (tmp_fd == -1) {
    mvprintw(LINES - 1, 0, "E creating temp file");
    refresh();
    gtimeout(500);
    return;
  }
  close(tmp_fd);

  move(LINES - 1, 0);
  clrtoeol();
  refresh();

  mvprintw(LINES - 1, 0, "'l'ist | e'x'tract | 'c'ancel: ");
  refresh();

  int ch = getch();
  switch (tolower(ch)) {
    case 'l': {
      line_clear(LINES - 1);
      mvprintw(LINES - 1, 0, "please wait");
      refresh();
      char cmd[MAX_PATH * 3];
      snprintf(cmd,
               sizeof(cmd),
               "ifm-ar -t \"%s\" > \"%s\" 2>&1",
               full_path,
               tmp_path);

      system(cmd);

      def_prog_mode();
      endwin();
      char less_cmd[MAX_PATH + 10];
      snprintf(less_cmd, sizeof(less_cmd), "less -r \"%s\"", tmp_path);
      system(less_cmd);

      unlink(tmp_path);
      reset_prog_mode();
      refresh();
      break;
    }
    case 'x': {
      char extract_path[MAX_PATH];
      strcpy(extract_path, path);

      if (cpe(extract_path, sizeof(extract_path), "path: ")) {
        if (strlen(extract_path) > 0) {
          char* p = extract_path;
          while (*p != '\0') {
            if (*p == '/') {
              *p = '\0';
              mkdir(extract_path, 0755);
              *p = '/';
            }
            p++;
          }
          mkdir(extract_path, 0755);
        } else {
          strcpy(extract_path, path);
        }
      } else {
        unlink(tmp_path);
        return;
      }
      line_clear(LINES - 1);
      mvprintw(LINES - 1, 0, "please wait");
      refresh();
      char cmd[MAX_PATH * 3];
      snprintf(cmd,
               sizeof(cmd),
               "ifm-ar -x \"%s\" -o \"%s\"",
               full_path,
               extract_path);
      list(path, NULL, false, false);
      refresh();

      int result = system(cmd);

      move(LINES - 1, 0);
      clrtoeol();
      if (result == 0) {
        mvprintw(LINES - 1, 0, "Extracted to: %s", extract_path);
        list(path, NULL, false, false);
        refresh();
        gtimeout(300);
      } else {
        mvprintw(LINES - 1, 0, "E extracting to: %s", extract_path);
        refresh();
        gtimeout(500);
        list(path, NULL, false, false);
      }
      refresh();
      break;
    }
    case 'c':
    default:
      break;
  }

  move(LINES - 1, 0);
  clrtoeol();
  refresh();

  unlink(tmp_path);
}

void
vi()
{
  char cmd[MAX_NAME] = { 0 };

  while (true) {
    if (!cpe(cmd, MAX_NAME, ":")) {
      break;
    }

    if (cmd[0] == '\0') {
      continue;
    }

    if (strcmp(cmd, "q") == 0 || strcmp(cmd, "quit") == 0) {
      __exit();
      return;
    }

    if (strcmp(cmd, "clear") == 0 || strcmp(cmd, "cls") == 0) {
      reset_terminal(0);
      list(path, NULL, false, false);
      return;
    }

    if (strcmp(cmd, "noh") == 0 || strcmp(cmd, "nohlsearch") == 0) {
      search_count = 0;
      search_index = 0;
      return;
    }

    if (strncmp(cmd, "set ", 4) == 0) {
      char* setting = cmd + 4;
      char* value = strchr(setting, ' ');

      if (value) {
        *value = '\0';
        value++;

        if (strcmp(setting, "sd") == 0) {
          sd = atoi(value);
          if (sd != 0 && sd != 1) {
            sd = 0;
          }

          reset_terminal(0);
          list(path, NULL, false, false);
          return;
        } else {
          mvprintw(LINES - 1, 0, "Unknown setting: %s", setting);
          refresh();
          gtimeout(500);
        }
      } else {
        mvprintw(LINES - 1, 0, "Usage: set <option> <value>");
        refresh();
        gtimeout(500);
      }
      continue;
    }

    if (cmd[0] == '+' || cmd[0] == '-') {
      int step = 1;

      if (strlen(cmd) > 1) {
        step = atoi(cmd + 1);
        if (step <= 0)
          step = 1;
      }

      if (cmd[0] == '+') {
        selected += step;
        if (selected >= file_count) {
          selected = file_count - 1;
        }
      } else {
        selected -= step;
        if (selected < 0) {
          selected = 0;
        }
      }

      int height = LINES - 4;
      offset = (selected > height / 2) ? selected - height / 2 : 0;
      if (offset > file_count - height) {
        offset = file_count - height;
      }
      if (offset < 0)
        offset = 0;

      return;
    }

    if (isdigit(cmd[0])) {
      int num = atoi(cmd);
      if (num >= 0) {
        if (num < file_count) {
          selected = num;
        } else {
          selected = file_count - 1;
        }
        int height = LINES - 4;
        offset = (selected > height / 2) ? selected - height / 2 : 0;
        if (offset > file_count - height) {
          offset = file_count - height;
        }
        if (offset < 0)
          offset = 0;
        return;
      }
    }

    attron(COLOR_PAIR(6) | A_BOLD);
    mvprintw(LINES - 1, 0, "unknown command: '%s'", cmd);
    attroff(COLOR_PAIR(6) | A_BOLD);
    refresh();
    gtimeout(500);
    return;
  }
}

void
__exit()
{
  clear();
  refresh();
  cp_buff_count = 0;
  endwin();
  exit(0);
  return;
}

void
__scrl_up()
{
  if (selected > 0) {
    selected--;
  } else {
    selected = file_count - 1;
  }
}

void
__scrl_down()
{
  if (selected < file_count - 1) {
    selected++;
  } else {
    selected = 0;
  }
}

void
__goto_last_file()
{
  selected = file_count - 1;
}

void
__goto()
{
  goto_help();
}

void
__hidden_files()
{
  int prev_selected = selected;
  char prev_file[MAX_PATH] = { 0 };
  if (prev_selected >= 0 && prev_selected < file_count) {
    strncpy(prev_file, files[prev_selected], MAX_PATH);
  }

  s_hidden = !s_hidden;
  list(path, NULL, false, false);
  ;

  if (strlen(prev_file)) {
    for (int i = 0; i < file_count; i++) {
      if (strcmp(files[i], prev_file) == 0) {
        selected = i;
        break;
      }
    }
  }

  offset = 0;
  Display();
}

void
__to_prev()
{
  if (strlen(lpath) == 0) {
    return;
  }

  if (access(lpath, R_OK | X_OK) != 0) {
    return;
  }

  char temp[MAX_PATH];
  strncpy(temp, path, sizeof(temp));

  strncpy(path, lpath, sizeof(path));
  strncpy(lpath, temp, sizeof(lpath));

  if (chdir(path) != 0) {
    return;
  }

  list(path, NULL, false, false);
  ;
  selected = 0;
  offset = 0;
}

void
__to_frwd()
{
  char full_path[MAX_PATH];

  snprintf(full_path, sizeof(full_path), "%s/%s", path, files[selected]);

  struct stat st;
  if (lstat(full_path, &st) == 0) {
    if (S_ISDIR(st.st_mode) ||
        (S_ISLNK(st.st_mode) && stat(full_path, &st) == 0 &&
         S_ISDIR(st.st_mode))) {
      strncpy(lpath, path, sizeof(lpath));

      if (chdir(full_path) != 0) {
        line_clear(LINES - 1);
        mvprintw(LINES - 1, 0, "E: Cannot access directory");
        refresh();
        gtimeout(500);
        return;
      }

      getcwd(path, sizeof(path));
      list(path, NULL, false, false);
      selected = 0;
      offset = 0;
      return;
    }
  }

  const char* ext = strrchr(files[selected], '.');
  if (ext &&
      (strcasecmp(ext, ".zip") == 0 || strcasecmp(ext, ".tar") == 0 ||
       strcasecmp(ext, ".gz") == 0 || strcasecmp(ext, ".bz2") == 0 ||
       strcasecmp(ext, ".xz") == 0 || strcasecmp(ext, ".rar") == 0 ||
       strcasecmp(ext, ".7z") == 0 || strcasecmp(ext, ".ar") == 0 ||
       strcasecmp(ext, ".lz") == 0 || strcasecmp(ext, ".bz") == 0 ||
       strcasecmp(ext, ".pkg.tar.zst") == 0 || strcasecmp(ext, ".tgz") == 0 ||
       strcasecmp(ext, ".tar.gz") == 0)) {
    handle_archive(files[selected]);
  } else {
    open_file(files[selected]);
  }
}

void
__mark_file()
{
  assert(selected >= 0 && selected < file_count && "Invalid selected index");
  assert(strlen(path) > 0 && "Empty current path");
  assert(strlen(files[selected]) > 0 && "Empty filename");
  assert(strlen(files[selected]) < MAX_NAME && "filename too long");
  assert(strlen(path) < MAX_PATH && "path too long");

  char full_path[MAX_PATH];
  snprintf(full_path, sizeof(full_path), "%s/%s", path, files[selected]);

  struct stat st;
  if (stat(full_path, &st) != 0) {
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
}

void
__delete()
{
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
  } else {
    char full_path[MAX_PATH];
    snprintf(full_path, sizeof(full_path), "%s/%s", path, files[selected]);
    if (confrim_delete(files[selected])) {
      rm(full_path);
      list(path, NULL, false, false);
      ;
      selected = 0;
    }
  }
}

void
__make_dir()
{
  cr_dir();
  list(path, NULL, false, false);
  ;
}

void
__make_file()
{
  cr_file();
  list(path, NULL, false, false);
  ;
}

void
__rename()
{
  ren(files[selected]);
}

void
__PG_scrl_up()
{
  selected -= (LINES - 4) / 2;
  if (selected < 0)
    selected = 0;
}

void
__PG_scrl_dn()
{
  selected += (LINES - 4) / 2;
  if (selected >= file_count)
    selected = file_count - 1;
}

void
__tab_handle()
{
  if (selected < file_count - 1) {
    selected++;
  } else {
    selected = 0;
  }
}

void
__echo(const char* t, int y)
{
  attron(A_BOLD);
  mvprintw(0, y, "%s", t);
  attroff(A_BOLD);
}

void
__cut()
{
  assert(selected >= 0 && selected < file_count && "Invalid selected index");
  assert(strlen(path) > 0 && "Empty current path");
  assert(strlen(files[selected]) > 0 && "Empty filename");

  char full_path[MAX_PATH];
  snprintf(full_path, sizeof(full_path), "%s/%s", path, files[selected]);

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

  __echo("c", COLS - 1);

  mvwprintw(win, 1, 0, "key          command");
  mvwhline(win, 2, 0, ACS_HLINE, win_width);
  mvwprintw(win, 3, 0, "x            cut");

  wrefresh(win);

  int ch = getch();
  delwin(win);
  touchwin(stdscr);
  refresh();

  if (ch != ERR) {
    switch (ch) {
      case 'x':
        if (access(full_path, F_OK) != 0) {
          line_clear(LINES - 1);
          mvprintw(LINES - 1, 0, "[E:01] File not found");
          refresh();
          gtimeout(800);
          return;
        }

        for (int i = 0; i < cp_buff_count; i++) {
          if (strcmp(cp_buff[i], full_path) == 0) {
            line_clear(LINES - 1);
            mvprintw(LINES - 1, 0, "[E:02] Already in buffer");
            refresh();
            gtimeout(800);
            return;
          }
        }

        if (cp_buff_count >= MAX_COPY_FILES) {
          line_clear(LINES - 1);
          mvprintw(LINES - 1, 0, "[E:03] Buffer full (%d)", MAX_COPY_FILES);
          refresh();
          gtimeout(500);
          return;
        }

        strncpy(cp_buff[cp_buff_count], full_path, MAX_PATH);

        cp_buff_ct[cp_buff_count] = true;
        cp_buff_count++;

        line_clear(LINES - 1);
        mvprintw(LINES - 1, 0, "'%s' cut", files[selected]);
        refresh();
        gtimeout(500);
        break;

      default:
        break;
    }
  }
}

void
__copy()
{
  assert(selected >= 0 && selected < file_count && "Invalid selected index");
  assert(strlen(path) > 0 && "Empty current path");
  assert(strlen(files[selected]) > 0 && "Empty filename");

  char full_path[MAX_PATH];
  snprintf(full_path, sizeof(full_path), "%s/%s", path, files[selected]);

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

  __echo("y", COLS - 1);

  mvwprintw(win, 1, 0, "key          command");
  mvwhline(win, 2, 0, ACS_HLINE, win_width);
  mvwprintw(win, 3, 0, "y            copy");

  wrefresh(win);

  int ch = getch();
  delwin(win);
  touchwin(stdscr);
  refresh();

  if (ch != ERR) {
    switch (ch) {
      case 'y':
        if (access(full_path, F_OK) != 0) {
          line_clear(LINES - 1);
          mvprintw(LINES - 1, 0, "[E:01] File not found");
          refresh();
          gtimeout(800);
          return;
        }

        for (int i = 0; i < cp_buff_count; i++) {
          if (strcmp(cp_buff[i], full_path) == 0) {
            line_clear(LINES - 1);
            mvprintw(LINES - 1, 0, "[E:02] Already in buffer");
            refresh();
            gtimeout(800);
            return;
          }
        }

        if (cp_buff_count >= MAX_COPY_FILES) {
          line_clear(LINES - 1);
          mvprintw(LINES - 1, 0, "[E:03] Buffer full (%d)", MAX_COPY_FILES);
          refresh();
          gtimeout(500);
          return;
        }

        strncpy(cp_buff[cp_buff_count], full_path, MAX_PATH);
        cp_buff_count++;

        line_clear(LINES - 1);
        mvprintw(LINES - 1, 0, "'%s' copied", files[selected]);
        refresh();
        gtimeout(500);
        break;

      default:
        break;
    }
    {
    }
  }
}

void
__paste()
{
  if (cp_buff_count == 0) {
    line_clear(LINES - 1);
    mvprintw(LINES - 1, 0, "[E:04] Buffer empty");
    refresh();
    gtimeout(500);
    return;
  }

  if (access(path, W_OK) != 0) {
    line_clear(LINES - 1);
    mvprintw(LINES - 1, 0, "[E:05] Write denied");
    refresh();
    gtimeout(500);
    return;
  }

  int ok = 0, fail = 0;
  bool cut_items_present = false;

  for (int i = 0; i < cp_buff_count; i++) {
    char* name = basename(cp_buff[i]);
    char dest[MAX_PATH];
    snprintf(dest, sizeof(dest), "%s/%s", path, name);

    if (access(dest, F_OK) == 0) {
      fail++;
      continue;
    }

    char cmd[MAX_PATH * 2 + 10];
    snprintf(
      cmd, sizeof(cmd), "cp -r \"%s\" \"%s\" 2>/dev/null", cp_buff[i], dest);

    if (system(cmd) == 0) {
      ok++;
      if (cp_buff_ct[i]) {
        cut_items_present = true;
      }
    } else {
      fail++;
    }
  }

  if (cut_items_present) {
    for (int i = 0; i < cp_buff_count; i++) {
      if (cp_buff_ct[i]) {
        char cmd[MAX_PATH + 10];
        snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", cp_buff[i]);
        system(cmd);
      }
    }
    cp_buff_count = 0;
    memset(cp_buff_ct, 0, sizeof(cp_buff_ct));
  }

  line_clear(LINES - 1);
  if (fail == 0) {
    mvprintw(LINES - 1,
             0,
             "pasted %d items%s",
             ok,
             cut_items_present ? " (cut items removed)" : "");
    list(path, NULL, false, false);
    refresh();
  } else {
    mvprintw(LINES - 1, 0, "Done (%d ok, %d fail)", ok, fail);
  }
  refresh();
  gtimeout(500);
  list(path, NULL, false, false);
}
