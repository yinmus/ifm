// github : https://github.com/yinmus/ifm
// License: MIT
// by yinmus

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>
#include <sys/stat.h>
#include <time.h>
#include <wchar.h> 
#include "icons.h"

#define MAX_FILES 1024
#define MAX_PATH 1024
#define MAX_NAME 256

char files[MAX_FILES][MAX_PATH];
int file_count = 0, selected = 0, offset = 0;
char path[MAX_PATH];
char lpath[MAX_PATH];
int s_hidden = 0;

int dist_s(const char *str) {
    wchar_t wstr[MAX_NAME];
    mbstowcs(wstr, str, MAX_NAME);
    return wcswidth(wstr, MAX_NAME);
}

void cls_scr() {
    system("clear");
}

void help_view() {
    def_prog_mode();
    endwin();

    char cmd[MAX_PATH];
    snprintf(cmd, sizeof(cmd), "man %s/Документы/.h/help.1", getenv("HOME"));
    system(cmd);

    reset_prog_mode();
    refresh();
}

int conf_ex() {
    curs_set(1); 
    echo();      
    char response[MAX_NAME];
    mvprintw(LINES - 2, 0, "Are you sure you want to quit? (y/n): ");
    getnstr(response, MAX_NAME - 1);
    noecho();    
    curs_set(0); 
    return (response[0] == 'y' || response[0] == 'Y');
}

void ls_files(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return;
    }
    
    file_count = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && file_count < MAX_FILES) {
        if (!s_hidden && entry->d_name[0] == '.') continue;
        strncpy(files[file_count++], entry->d_name, MAX_PATH);
    }
    closedir(dir);
}

void get_inf(const char *filename, char *info, size_t info_size) {
    char full_path[MAX_PATH];
    snprintf(full_path, sizeof(full_path), "%s/%s", path, filename);

    struct stat st;
    if (stat(full_path, &st) != 0) {
        snprintf(info, info_size, "Unknown");
        return;
    }

    double size = (double)st.st_size;
    const char *unit = "B";
    if (size > 1024) {
        size /= 1024;
        unit = "KB";
    }
    if (size > 1024) {
        size /= 1024;
        unit = "MB";
    }
    if (size > 1024) {
        size /= 1024;
        unit = "GB";
    }

    char date[20];
    strftime(date, sizeof(date), "%Y-%m-%d %H:%M", localtime(&st.st_mtime));

    snprintf(info, info_size, "Size: %.2f %s | Modified: %s", size, unit, date);
}

void dUI() {
    clear();
    attron(A_BOLD);
    mvprintw(0, 0, "[ IFileManager ] - %s", path);
    attroff(A_BOLD);
    mvprintw(1, 0, "──────────────────────────────────────────────────────────────");
    
    int height = LINES - 6; 
    if (selected >= offset + height) offset = selected - height + 1;
    if (selected < offset) offset = selected;
    
    int start_y = 2;
    int start_x = 0;
    int end_y = start_y + height;
    int end_x = COLS - 1;


    for (int y = start_y; y <= end_y; y++) {
        if (y == start_y || y == end_y) {
            mvaddch(y, start_x, ACS_ULCORNER); 
            mvaddch(y, end_x, ACS_URCORNER);  
            for (int x = start_x + 1; x < end_x; x++) {
                mvaddch(y, x, ACS_HLINE);      
            }
        } else {
            mvaddch(y, start_x, ACS_VLINE);     
            mvaddch(y, end_x, ACS_VLINE);       
        }
    }

    for (int i = 0; i < height && i + offset < file_count; i++) {
        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, files[i + offset]);

        struct stat st;
        char *icon = ""; 

        if (stat(full_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                icon = "";
                attron(COLOR_PAIR(1)); 
            } else if (S_ISREG(st.st_mode)) {
                attron(COLOR_PAIR(2)); 
                const char *ext = strrchr(files[i + offset], '.');
                if (ext) {
                    ext++;
                    icon = icon_ext(ext);
                }
            }
        }

        char size_str[20];
        char date_str[20];
        if (stat(full_path, &st) == 0) {
            double size = (double)st.st_size;
            const char *unit = "B";
            if (size > 1024) {
                size /= 1024;
                unit = "KB";
            }
            if (size > 1024) {
                size /= 1024;
                unit = "MB";
            }
            if (size > 1024) {
                size /= 1024;
                unit = "GB";
            }
            snprintf(size_str, sizeof(size_str), "%.2f %s", size, unit);

            strftime(date_str, sizeof(date_str), "%Y-%m-%d %H:%M", localtime(&st.st_mtime));
        } else {
            snprintf(size_str, sizeof(size_str), "Unknown");
            snprintf(date_str, sizeof(date_str), "Unknown");
        }

        int name_width = dist_s(files[i + offset]);

        char line[MAX_PATH];
        snprintf(line, sizeof(line), "%s %-*s %10s %20s", icon, 30 - (name_width - strlen(files[i + offset])), files[i + offset], size_str, date_str);

        if (i + offset == selected) attron(A_REVERSE);
        mvprintw(i + start_y + 1, 2, "%s", line);
        if (i + offset == selected) attroff(A_REVERSE);

        attroff(COLOR_PAIR(1));
        attroff(COLOR_PAIR(2));
    }

    mvprintw(LINES - 4, 0, "──────────────────────────────────────────────────────────────");
    mvprintw(LINES - 3, 2, "Files: %d | Hidden: %s", file_count, s_hidden ? "On" : "Off");
    mvprintw(LINES - 2, 0, "──────────────────────────────────────────────────────────────");
    // mvprintw(LINES - 1, 0, "[K/J] Navigate | [Enter/L] Open | [H] Back | [Alt+U] Toggle Hidden | [T] New File | [D] Delete | [Shift+D] New Dir | [Q] Quit | [Alt+A] Open Help");
    refresh();
}

void rmfd(const char *filename) {
    char full_path[MAX_PATH];
    snprintf(full_path, sizeof(full_path), "%s/%s", path, filename);
    
    struct stat st;
    if (stat(full_path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            rmdir(full_path);
        } else {
            remove(full_path);
        }
    }
}

void open_file(const char *filename) {
    char full_path[MAX_PATH];
    snprintf(full_path, sizeof(full_path), "%s/%s", path, filename);
    
    const char *ext = strrchr(filename, '.');
    if (!ext) {
        char cmd[MAX_PATH + 50];
        snprintf(cmd, sizeof(cmd), "micro '%s'", full_path);
        system(cmd);
        return;
    }

    ext++;
    const char *image_formats[] = {"png", "jpg", "jpeg", "webp", "svg", "bmp", "gif", "tiff"};
    const char *video_audio_formats[] = {
        "mp3", "mp4", "wav", "ogg", "mkv", "webm", "flac", "avi", "mov", "m4a", "aac"
    };

    for (size_t i = 0; i < sizeof(image_formats) / sizeof(image_formats[0]); i++) {
        if (!strcasecmp(ext, image_formats[i])) {
            char cmd[MAX_PATH + 50];
            snprintf(cmd, sizeof(cmd), "feh '%s' &> /dev/null &", full_path);
            system(cmd);
            return;
        }
    }

    for (size_t i = 0; i < sizeof(video_audio_formats) / sizeof(video_audio_formats[0]); i++) {
        if (!strcasecmp(ext, video_audio_formats[i])) {
            char cmd[MAX_PATH + 50];
            snprintf(cmd, sizeof(cmd), "vlc --quiet '%s' &> /dev/null &", full_path);
            system(cmd);
            return;
        }
    }

    char cmd[MAX_PATH + 50];
    snprintf(cmd, sizeof(cmd), "micro '%s'", full_path); 
    system(cmd);
}

void cr_file() {
    echo();
    char filename[MAX_NAME];
    mvprintw(LINES - 2, 0, "Enter filename: ");
    getnstr(filename, MAX_NAME - 1);
    noecho();
    
    if (strlen(filename) > 0) {
        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, filename);
        FILE *file = fopen(full_path, "w");
        if (file) fclose(file);
    }
}

void to_back() {
    if (chdir("..") == 0) {
        getcwd(path, sizeof(path));
        ls_files(path);
        selected = 0;
        offset = 0;
    }
}

void to_home() {
    const char *home = getenv("HOME");
    if (home) {
        strncpy(path, home, sizeof(path));
        chdir(path);
        ls_files(path);
        selected = 0;
        offset = 0;
    }
}

void cr_dir() {
    echo();
    char dirname[MAX_NAME];
    mvprintw(LINES - 2, 0, "Enter directory name: ");
    getnstr(dirname, MAX_NAME - 1);
    noecho();
    
    if (strlen(dirname) > 0) {
        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, dirname);
        mkdir(full_path, 0777);
    }
}

int is_dirY(const char *path) {
    struct stat statbuf;
    return (stat(path, &statbuf) == 0 && S_ISDIR(statbuf.st_mode));
}

int main(int argc, char *argv[]) {
    setlocale(LC_ALL, "");
    initscr();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    start_color();
    init_pair(1, COLOR_BLUE, COLOR_BLACK); 
    init_pair(2, COLOR_GREEN, COLOR_BLACK); 

    if (argc > 1) {
        strncpy(path, argv[1], sizeof(path));
    } else {
        getcwd(path, sizeof(path));  
    }

    ls_files(path);
    strncpy(lpath, path, sizeof(lpath));

    while (1) {
        dUI();
        int ch = getch();
        
        if (ch == 27) { // Alt
            ch = getch();
            if (ch == 'u') {
                s_hidden = !s_hidden;
                ls_files(path);
            } else if (ch == 'a') { 
                help_view(); 
            } else if (ch == 'h') {
                to_home();
            }
        } else {
            switch (ch) {
                case 'q':
                    if (conf_ex()) { 
                        cls_scr();
                        endwin();
                        return 0;
                    }
                    break;
                case 'h':
                    to_back();
                    break;
                case 'k':
                    if (selected > 0) selected--;
                    break;
                case 'j':
                    if (selected < file_count - 1) selected++;
                    break;
                case 10:
                case 'l': {
                    char full_path[MAX_PATH];
                    snprintf(full_path, sizeof(full_path), "%s/%s", path, files[selected]);

                    if (is_dirY(full_path)) {
                        strncpy(lpath, path, sizeof(lpath));
                        chdir(full_path);
                        getcwd(path, sizeof(path));
                        ls_files(path);
                        selected = 0;
                        offset = 0;
                    } else {
                        open_file(files[selected]);
                    }
                    break;
                }
                case 'd':
                    rmfd(files[selected]);
                    ls_files(path);
                    selected = 0;
                    break;
                case 't':
                    cr_file();
                    ls_files(path);
                    break;
                case 'D':
                    cr_dir();
                    ls_files(path);
                    break;
            }
        }
    }

    endwin();
    return 0;
}