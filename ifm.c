// ifm.c
// by yinmus
// MIT License

// Copyright (c) 2025 IFM-YINMUS

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// GITHUB: https://github.com/yinmus/ifm

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <wchar.h>
#include "icons.h"

#define MAX_FILES 1024
#define MAX_PATH 1024
#define MAX_NAME 256

#define VIDEO_VIEWER "mpv"
#define AUDIO_VIEWER "mpv"
#define IMAGE_VIEWER "feh"
#define DEFAULT_VIEWER "micro"

char files[MAX_FILES][MAX_PATH];
int file_count = 0, selected = 0, offset = 0;
char path[MAX_PATH];
char lpath[MAX_PATH];
int s_hidden = 0;

int dist_s(const char *str);
void cls_scr();
void help_view();
int conf_del(const char *filename);
void ls_files(const char *path);
void get_inf(const char *filename, char *info, size_t info_size);
void ui();
void rmfd(const char *filename);
void open_file(const char *filename);
void cr_file();
void to_back();
void to_home();
void cr_dir();
void rename_fd(const char *filename);
int dirt(const char *path);
int conf_ex();

int dist_s(const char *str) {
    wchar_t wstr[MAX_NAME];
    mbstowcs(wstr, str, MAX_NAME);
    return wcswidth(wstr, MAX_NAME);
}

void cls_scr() {
    clear();
    refresh();
}

void help_view() {
    def_prog_mode();
    endwin();

    FILE *pager = popen("less", "w"); 

    if (pager == NULL) {
        perror("popen");
        return;
    }

    fprintf(pager, "IFM - Lightweight Ncurses File Manager\n");
    fprintf(pager, "======================================\n");
    fprintf(pager, "Controls:\n");
    fprintf(pager, "  [K/J]        Move up/down\n");
    fprintf(pager, "  [Enter/L]    Open file/dir\n");
    fprintf(pager, "  [H]          Go back\n");
    fprintf(pager, "  [U]          Toggle hidden files\n");
    fprintf(pager, "  [Alt + H]    Go home\n");
    fprintf(pager, "  [T]          Create file\n");
    fprintf(pager, "  [D]          Create dir\n");
    fprintf(pager, "  [Delete]     Delete file/dir\n");
    fprintf(pager, "  [R]          Rename file/dir\n");
    fprintf(pager, "  [C]          Open with custom viewer\n");
    fprintf(pager, "  [Q]          Exit\n");
    fprintf(pager, "  [F1]         Help\n");
    fprintf(pager, "\n");
    fprintf(pager, "\n");
    fprintf(pager, "\n");
    fprintf(pager, "\n");
    fprintf(pager, "\n");
    fprintf(pager, "\n");


    pclose(pager); 

    reset_prog_mode();
    refresh();
}

int conf_ex() {
    curs_set(1);
    echo();
    char response[MAX_NAME];
    mvprintw(LINES - 2, 0, "Quit? (y/n): ");
    getnstr(response, MAX_NAME - 1);
    noecho();
    curs_set(0);
    return (response[0] == 'y' || response[0] == 'Y');
}

int conf_del(const char *filename) {
    curs_set(1);
    echo();
    char response[MAX_NAME];
    mvprintw(LINES - 2, 0, "Delete %s? (y/n): ", filename);
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

void ui() {
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
        const char *icon = "";

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
    refresh();
}

void rmfd(const char *filename) {
    if (!conf_del(filename)) return;

    char full_path[MAX_PATH];
    snprintf(full_path, sizeof(full_path), "%s/%s", path, filename);

    struct stat st;
    if (stat(full_path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            if (rmdir(full_path) != 0) {
                mvprintw(LINES - 1, 0, "Error: Could not delete directory.");
            }
        } else {
            if (remove(full_path) != 0) {
                mvprintw(LINES - 1, 0, "Error: Could not delete file.");
            }
        }
        ls_files(path);
    }
}

void open_file(const char *filename) {
    char full_path[MAX_PATH];
    snprintf(full_path, sizeof(full_path), "%s/%s", path, filename);

    const char *ext = strrchr(filename, '.');
    if (!ext) {
        char cmd[MAX_PATH + 50];
        snprintf(cmd, sizeof(cmd), "%s '%s'", DEFAULT_VIEWER, full_path);
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
            snprintf(cmd, sizeof(cmd), "%s '%s' &> /dev/null &", IMAGE_VIEWER, full_path);
            system(cmd);
            return;
        }
    }

    for (size_t i = 0; i < sizeof(video_audio_formats) / sizeof(video_audio_formats[0]); i++) {
        if (!strcasecmp(ext, video_audio_formats[i])) {
            char cmd[MAX_PATH + 50];
            snprintf(cmd, sizeof(cmd), "%s '%s' &> /dev/null &", strstr(ext, "mp3") ? AUDIO_VIEWER : VIDEO_VIEWER, full_path);
            system(cmd);
            return;
        }
    }

    char cmd[MAX_PATH + 50];
    snprintf(cmd, sizeof(cmd), "%s '%s'", DEFAULT_VIEWER, full_path);
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

void rename_fd(const char *filename) {
    echo();
    char new_name[MAX_NAME];
    mvprintw(LINES - 2, 0, "Enter new name: ");
    getnstr(new_name, MAX_NAME - 1);
    noecho();

    if (strlen(new_name) > 0) {
        char old_path[MAX_PATH];
        char new_path[MAX_PATH];
        snprintf(old_path, sizeof(old_path), "%s/%s", path, filename);
        snprintf(new_path, sizeof(new_path), "%s/%s", path, new_name);

        if (rename(old_path, new_path) != 0) {
            mvprintw(LINES - 1, 0, "Error: Could not rename file.");
        } else {
            ls_files(path);
        }
    }
}

int dirt(const char *path) {
    struct stat statbuf;
    return (stat(path, &statbuf) == 0 && S_ISDIR(statbuf.st_mode));
}

void custom_view(const char *filename) {
    echo();  
    char viewer[MAX_NAME];
    mvprintw(LINES - 2, 0, "Enter viewer: ");
    getnstr(viewer, MAX_NAME - 1);  
    noecho(); 

    if (strlen(viewer) > 0) {
        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, filename);

        const char *ext = strrchr(filename, '.');
        if (!ext) {
            ext = "";
        } else {
            ext++; 
        }

        const char *image_formats[] = {"png", "jpg", "jpeg", "webp", "svg", "bmp", "gif", "tiff"};
        const char *video_audio_formats[] = {
            "mp3", "mp4", "wav", "ogg", "mkv", "webm", "flac", "avi", "mov", "m4a", "aac"
        };

        int is_media = 0;
        for (size_t i = 0; i < sizeof(image_formats) / sizeof(image_formats[0]); i++) {
            if (!strcasecmp(ext, image_formats[i])) {
                is_media = 1;
                break;
            }
        }
        for (size_t i = 0; i < sizeof(video_audio_formats) / sizeof(video_audio_formats[0]); i++) {
            if (!strcasecmp(ext, video_audio_formats[i])) {
                is_media = 1;
                break;
            }
        }

        if (is_media) {
            char cmd[MAX_PATH + 50];
            snprintf(cmd, sizeof(cmd), "%s '%s' &> /dev/null &", viewer, full_path);
            system(cmd);
        } else {
            def_prog_mode(); 
            endwin(); 

            pid_t pid = fork();
            if (pid == 0) {  
                execlp(viewer, viewer, full_path, (char *)NULL);
                perror("execlp failed");
                exit(EXIT_FAILURE);
            } else if (pid > 0) {  
                int status;
                waitpid(pid, &status, 0);
            } else {
                perror("fork failed");
            }

            reset_prog_mode();  
            refresh();
        }
    }
}

int main(int argc, char *argv[]) {
    setlocale(LC_ALL, "");
    initscr();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    start_color();
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);

    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    mouseinterval(0);

    if (argc > 1) {
        strncpy(path, argv[1], sizeof(path));
    } else {
        getcwd(path, sizeof(path));
    }

    ls_files(path);
    strncpy(lpath, path, sizeof(lpath));

    while (1) {
        ui();
        int ch = getch();

        if (ch == KEY_MOUSE) {
            MEVENT event;
            if (getmouse(&event) == OK) {
                if (event.bstate & BUTTON1_PRESSED) {
                    int y = event.y - 3;
                    if (y >= 0 && y < file_count) {
                        selected = y + offset;
                        char full_path[MAX_PATH];
                        snprintf(full_path, sizeof(full_path), "%s/%s", path, files[selected]);

                        if (dirt(full_path)) {
                            strncpy(lpath, path, sizeof(lpath));
                            chdir(full_path);
                            getcwd(path, sizeof(path));
                            ls_files(path);
                            selected = 0;
                            offset = 0;
                        } else {
                            open_file(files[selected]);
                        }
                    }
                } else if (event.bstate & BUTTON3_PRESSED) {
                    to_back();
                } else if (event.bstate & BUTTON4_PRESSED) {
                    if (selected > 0) selected--;
                } else if (event.bstate & BUTTON5_PRESSED) {
                    if (selected < file_count - 1) selected++;
                }
            }
        } else if (ch == 27) {
            ch = getch();
            if (ch == 'a') {
                help_view();
            } else if (ch == 'h') {
                to_home();
            }

        } else {
            switch (ch) {
                case 'q':
                    if (conf_ex()) {
                        clear();
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
                case 'b':
                    selected = file_count - 1;
                    break;
                case 'g':
                    selected = 0;
                    break;
                case 'u':
                    s_hidden = !s_hidden;
                    ls_files(path);
                    break;
                case 10:
                case 'l': {
                    char full_path[MAX_PATH];
                    snprintf(full_path, sizeof(full_path), "%s/%s", path, files[selected]);

                    if (dirt(full_path)) {
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
                case KEY_DC:
                    rmfd(files[selected]);
                    ls_files(path);
                    selected = 0;
                    break;
                case 'd':
                    cr_dir();
                    ls_files(path);
                    break;
                case 't':
                    cr_file();
                    ls_files(path);
                    break;
                case 'r':
                    rename_fd(files[selected]);
                    break;
                case 'c':
                    custom_view(files[selected]);
                    break;
                case KEY_F(1):  // F1
                    help_view();
                    break;
            }
        }
    }

    endwin();
    return 0;
}
