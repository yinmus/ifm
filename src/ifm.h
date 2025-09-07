
#ifndef IFM_H
#define IFM_H

#include <stdbool.h>
#include <time.h>

#include "fmh.h"

#define MAX_FILES 16384
#define MAX_PATH 16384
#define MAX_NAME 256
#define MAX_COPY_FILES 100
#define MIN_DISPLAY_NAME 10
#define MAX_DISPLAY_NAME (COLS > 55 ? (COLS - 45) : MIN_DISPLAY_NAME)
#define CONTROL(c) ((c) & 0x1f)

extern char files[MAX_FILES][MAX_PATH];
extern int file_count, selected, offset;
extern char path[MAX_PATH];
extern char lpath[MAX_PATH];
extern int s_hidden;

extern char cp_buff[MAX_COPY_FILES][MAX_PATH];
extern int cp_buff_count;

typedef struct {
  char path[MAX_PATH];
  int marked;
} MarkedFile;

extern MarkedFile marked_files[MAX_FILES];

extern int last_clicked;

extern char **environ;

extern int sd;

#ifndef MOUSE_BTNS

#define LMB BUTTON1_PRESSED  // LEFT MOUSE BUTTON
#define RMB BUTTON3_PRESSED  // RIGHT MOUSE BUTTON
#define MWU BUTTON4_PRESSED  // MOUSE WHEEL UP
#define MWD BUTTON5_PRESSED  // MOUSE WHEEL DOWN

#endif

#ifndef PAGE_DN_UP
#define PAGE_UP KEY_PPAGE
#define PAGE_DOWN KEY_NPAGE
#endif

void list(const char *dir_path, const char *filter, bool show_dirs,
          bool show_files);
void rm(const char *path);
void open_file(const char *filename);
void cr_file();
void cr_dir();
void ren(const char *filename);
void open_with(const char *filename);
void to_back();
void to_home();
void show_marked_files();
void mark_help();
int cpe(char *buffer, int max_len, const char *prompt);
int confrim_delete(const char *filename);
void goto_cmd(int next_char);
void resolve_path(const char *input, char *output, size_t size);
void handle_archive(const char *filename);
void line_clear(int y);
void gtimeout(int ms);
void search();
void vi();
void search_UP();
void search_DN();

#define DIRT(path)                                                    \
  ({                                                                  \
    struct stat _statbuf;                                             \
    int _result = 0;                                                  \
    if (lstat((path), &_statbuf) == 0 && S_ISDIR(_statbuf.st_mode)) { \
      _result = (access((path), R_OK | X_OK) == 0);                   \
    }                                                                 \
    _result;                                                          \
  })

#define CASE_INSENSITIVE_STRSTR(haystack, needle) \
  ({                                              \
    int _found = 0;                               \
    if ((haystack) != NULL && (needle) != NULL) { \
      const char *_h = (haystack);                \
      while (*_h && !_found) {                    \
        const char *_h_ptr = _h;                  \
        const char *_n_ptr = (needle);            \
        while (tolower((unsigned char)*_h_ptr) == \
               tolower((unsigned char)*_n_ptr)) { \
          _h_ptr++;                               \
          _n_ptr++;                               \
          if (*_n_ptr == '\0') {                  \
            _found = 1;                           \
            break;                                \
          }                                       \
          if (*_h_ptr == '\0') {                  \
            break;                                \
          }                                       \
        }                                         \
        _h++;                                     \
      }                                           \
    }                                             \
    _found;                                       \
  })

#ifndef HANDLES
#define __BACK to_back()
#define __SCROLL_UP __scrl_up()
#define __SCROLL_DOWN __scrl_down()
#define __LAST_FILE __goto_last_file()
#define __GOTO __goto()
#define __EXIT __exit()
#define __HIDDEN_FILES __hidden_files()
#define __TO_FRWD __to_frwd()
#define __MARK_FILE __mark_file()
#define __DELETE __delete()
#define __MAKE_DIR __make_dir()
#define __MAKE_FILE __make_file()
#define __RENAME __rename()
#define __OPEN_WITH open_with(files[selected])
#define __MARK_FILES_MENU show_marked_files()
#define __HANDLE_MARKED_FILES mark_help()
#define __PGUP_HANDLE __PG_scrl_up()
#define __PGDN_HANDLE __PG_scrl_dn()
#define __UPDATE__ list(path, NULL, false, false);
#define __TAB_HANDLE __tab_handle()
#define __COPY __copy()
#define __PASTE __paste()
#define __VI vi()
#define __TO_PREV __to_prev()
#define __SEARCH search()
#define __CUT __cut()
#endif

void __exit();
void __scrl_up();
void __scrl_down();
void __goto_last_file();
void __goto();
void __hidden_files();
void __to_frwd();
void __mark_file();
void __delete();
void __make_dir();
void __make_file();
void __rename();
void __PG_scrl_up();
void __PG_scrl_dn();
void __tab_handle();
void __copy();
void __cut();
void __paste();
void __echo(const char *t, int y);
void __to_prev();

#endif
