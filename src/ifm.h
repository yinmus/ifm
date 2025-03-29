/*
======================================
    IFM by yinmus (c) 2025-2025
======================================

Relative path : ifm/src/ifm.h
Github url : https://github.com/yinmus/ifm.git
License : GPLv3 

*/

#ifndef IFM_H
#define IFM_H

#include <time.h>
#include "fmh.h"

#define MAX_FILES 16384
#define MAX_PATH 16384
#define MAX_NAME 256
#define MIN_DISPLAY_NAME 10
#define MAX_DISPLAY_NAME (COLS > 55 ? (COLS - 45) : MIN_DISPLAY_NAME)

extern char files[MAX_FILES][MAX_PATH];
extern int file_count, selected, offset;
extern char path[MAX_PATH];
extern char lpath[MAX_PATH];
extern int s_hidden;

typedef struct {
    char path[MAX_PATH];
    int marked;
} MarkedFile;

extern MarkedFile marked_files[MAX_FILES];

typedef struct {
    char video_viewer[MAX_NAME];
    char audio_viewer[MAX_NAME];
    char image_viewer[MAX_NAME];
    char default_viewer[MAX_NAME];
} Config;

extern int last_clicked;
extern time_t last_click_time;

extern char **environ;


void create_default_config();
void load_config(Config *config);
void list(const char *path);    
void rm(const char *path);
void open_file(const char *filename);
void cr_file();
void cr_dir();
void ren(const char *filename); 
void open_with(const char *filename);
void to_back();
void to_home();
void UI();
void show_marked_files();
void mark_help();
void fcontent(const char *filename);
void doc_menu();
int cpe(char *buffer, int max_len, const char *prompt);
int confrim_delete(const char *filename);
void get_info(const char *filename, char *info, size_t info_size);
void goto_cmd(int next_char);
int dist_s(const char *str);
int dirt(const char *path);
void hs_path(const char *relative_path, char *absolute_path);




#endif
