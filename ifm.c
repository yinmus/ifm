/*
    ifm.c
    MIT License

    Copyright (c) 2025 IFM-YINMUS

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
    _____________________________________________________________________________
    BY YINMUS
    <https://github.com/yinmus/ifm>
*/


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


// Default viewers

#define VIDEO_VIEWER "mpv"
#define AUDIO_VIEWER "mpv"
#define IMAGE_VIEWER "feh"
#define DEFAULT_VIEWER "micro"

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

void cls() {
    clear();
    refresh();
}

int esc(char *buffer, int max_len, const char *prompt) {
    echo();  
    curs_set(1);  

    int pos = 0; 
    buffer[0] = '\0';  

    while (1) {
        move(LINES - 2, 0);
        clrtoeol();  
        mvprintw(LINES - 2, 0, "%s: %s", prompt, buffer);  
        move(LINES - 2, strlen(prompt) + 2 + pos); 
        refresh();

        int ch = getch();  

        if (ch == '\n' || ch == KEY_ENTER) {  
            break;
        } else if (ch == 27) { 
            noecho();
            curs_set(0);
            mvprintw(LINES - 2, 0, "Cancelled. Exiting input...");
            refresh();
            usleep(1500000);  
            return 0;  
        } else if (ch == 127 || ch == KEY_BACKSPACE) {  
            if (pos > 0) {
                memmove(&buffer[pos - 1], &buffer[pos], strlen(buffer) - pos + 1);
                pos--;
            }
        } else if (ch == KEY_DC) {  
            if (pos < strlen(buffer)) {
                memmove(&buffer[pos], &buffer[pos + 1], strlen(buffer) - pos);
            }
        } else if (ch == KEY_LEFT) {  
            if (pos > 0) pos--;
        } else if (ch == KEY_RIGHT) {  
            if (pos < strlen(buffer)) pos++;
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
    fprintf(pager, "  [:]          Enter command mode\n");
    fprintf(pager, "  [G]          Go to the first file\n");
    fprintf(pager, "  [B]          Go to the last file\n");
    fprintf(pager, "  [\]          Open with custom viewer\n");
    fprintf(pager, "\n");
    fprintf(pager, "Command Mode:\n");
    fprintf(pager, "  [cd <dir>]   Change directory\n");
    fprintf(pager, "  [h]          Show hidden files\n");
    fprintf(pager, "  [uh]         Hide hidden files\n");
    fprintf(pager, "  [q]          Quit\n");
    fprintf(pager, "  [gg]         Go to the first file\n");
    fprintf(pager, "  [g]          Go to the last file\n");
    fprintf(pager, "\n");
    fprintf(pager, "Mouse Controls:\n");
    fprintf(pager, "  [Left Click] Select/Open file/dir\n");
    fprintf(pager, "  [Right Click] Go back\n");
    fprintf(pager, "  [Scroll Up] Move up\n");
    fprintf(pager, "  [Scroll Down] Move down\n");
    fprintf(pager, "\n");
    fprintf(pager, "\n");

    pclose(pager); 

    reset_prog_mode();
    refresh();
}


// Confrim delete file/directory

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

int compare(const void *a, const void *b) {
    return strcasecmp((const char *)a, (const char *)b);
}

void list(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return;
    }

    file_count = 0;

    if (s_hidden) {
        strncpy(files[file_count++], ".", MAX_PATH);
        strncpy(files[file_count++], "..", MAX_PATH);
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && file_count < MAX_FILES) {
        if (!s_hidden && entry->d_name[0] == '.') continue;  
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue; 
        strncpy(files[file_count++], entry->d_name, MAX_PATH);
    }
    closedir(dir);

    qsort(files, file_count, MAX_PATH, compare);
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

void UI() {
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

void rmfd(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        perror("stat");
        return;
    }

    if (S_ISDIR(st.st_mode)) {
        DIR *dir = opendir(path);
        if (!dir) {
            perror("opendir");
            return;
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

            char full_path[MAX_PATH];
            snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

            rmfd(full_path); 
        }
        closedir(dir);

        if (rmdir(path) != 0) {
            perror("rmdir");
        }
    } else {
        if (remove(path) != 0) {
            perror("remove");
        }
    }
}

void open_file(const char *filename) {
    char full_path[MAX_PATH];
    snprintf(full_path, sizeof(full_path), "%s/%s", path, filename);

    def_prog_mode(); 
    endwin();  

    const char *ext = strrchr(filename, '.');
    if (!ext) {
        char cmd[MAX_PATH + 50];
        snprintf(cmd, sizeof(cmd), "%s '%s'", DEFAULT_VIEWER, full_path);
        system(cmd);
    } else {
        ext++;
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
            snprintf(cmd, sizeof(cmd), "%s '%s' &> /dev/null &", strstr(ext, "mp3") ? AUDIO_VIEWER : VIDEO_VIEWER, full_path);
            system(cmd);
        } else {
            char cmd[MAX_PATH + 50];
            snprintf(cmd, sizeof(cmd), "%s '%s'", DEFAULT_VIEWER, full_path);
            system(cmd);
        }
    }

    reset_prog_mode(); 
    refresh();  
}

void cr_file() {
    char filename[MAX_NAME];
    if (esc(filename, MAX_NAME, "Enter filename")) {
        if (strlen(filename) > 0) {
            char full_path[MAX_PATH];
            snprintf(full_path, sizeof(full_path), "%s/%s", path, filename);
            FILE *file = fopen(full_path, "w");
            if (file) fclose(file);
        }
    }
}
void to_back() {
    if (chdir("..") == 0) {
        getcwd(path, sizeof(path));
        list(path);
        selected = 0;
        offset = 0;
    }
}

void to_home() {
    const char *home = getenv("HOME");
    if (home) {
        strncpy(path, home, sizeof(path));
        chdir(path);
        list(path);
        selected = 0;
        offset = 0;
    }
}

void crt_dir() {
    char dirname[MAX_NAME];
    if (esc(dirname, MAX_NAME, "Enter directory name")) {
        if (strlen(dirname) > 0) {
            char full_path[MAX_PATH];
            snprintf(full_path, sizeof(full_path), "%s/%s", path, dirname);
            mkdir(full_path, 0777);
        }
    }
}

void ren(const char *filename) {
    setlocale(LC_ALL, ""); 

    char new_name[MAX_NAME];
    strncpy(new_name, filename, MAX_NAME - 1);  
    new_name[MAX_NAME - 1] = '\0';  

    if (!esc(new_name, MAX_NAME, "New name")) {
        return;  
    }

    if (strlen(new_name) > 0) {
        char old_path[MAX_PATH];
        char new_path[MAX_PATH];
        snprintf(old_path, sizeof(old_path), "%s/%s", path, filename);
        snprintf(new_path, sizeof(new_path), "%s/%s", path, new_name);

        if (access(new_path, F_OK) == 0) {
            mvprintw(LINES - 1, 0, "Error: File with this name already exists.");
            return;
        }

        if (rename(old_path, new_path) != 0) {
            mvprintw(LINES - 1, 0, "Error: Could not rename file.");
        } else {
            list(path);  
        }
    }
}

int dirt(const char *path) {
    struct stat statbuf;
    return (stat(path, &statbuf) == 0 && S_ISDIR(statbuf.st_mode));
}

void UC_view(const char *filename) {
    char viewer[MAX_NAME];

    if (!esc(viewer, MAX_NAME, "Enter viewer")) {
        return;  
    }

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
            cls(); 
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


void Command() {
    echo();
    curs_set(1);
    wchar_t command[MAX_NAME] = {0};
    int pos = 0;
    int tab_index = -1;
    char matches[MAX_FILES][MAX_NAME];
    int match_count = 0;

    while (1) {
        move(LINES - 2, 0);  
        clrtoeol(); 
        mvprintw(LINES - 2, 0, ":%ls", command);
        move(LINES - 2, pos + 1); 
        refresh();

        wint_t ch;
        get_wch(&ch);

        if (ch == '\t') {
            if (wcsncmp(command, L"cd ", 3) == 0) {
                wchar_t prefix[MAX_NAME] = {0};
                wcsncpy(prefix, command + 3, pos - 3);

                if (tab_index == -1) {
                    match_count = 0;
                    DIR *dir = opendir(".");
                    if (dir) {
                        struct dirent *entry;
                        while ((entry = readdir(dir)) != NULL) {
                            struct stat st;
                            if (stat(entry->d_name, &st) == 0 && S_ISDIR(st.st_mode)) {
                                wchar_t wname[MAX_NAME];
                                mbstowcs(wname, entry->d_name, MAX_NAME);
                                if (wcsncmp(wname, prefix, wcslen(prefix)) == 0) {
                                    wcstombs(matches[match_count++], wname, MAX_NAME);
                                }
                            }
                        }
                        closedir(dir);
                    }

                    if (match_count == 0) continue;

                    if (match_count > 1) {
                        wchar_t common_prefix[MAX_NAME] = {0};
                        mbstowcs(common_prefix, matches[0], MAX_NAME);

                        for (int i = 1; i < match_count; i++) {
                            int j = 0;
                            wchar_t wmatch[MAX_NAME];
                            mbstowcs(wmatch, matches[i], MAX_NAME);
                            while (common_prefix[j] && wmatch[j] && common_prefix[j] == wmatch[j]) {
                                j++;
                            }
                            common_prefix[j] = '\0';
                        }

                        if (wcslen(common_prefix) > wcslen(prefix)) {
                            wcsncpy(command + 3, common_prefix, MAX_NAME - 3);
                            pos = 3 + wcslen(common_prefix);
                            continue;
                        }
                    }
                }

                if (match_count > 0) {
                    tab_index = (tab_index + 1) % match_count;
                    wchar_t wmatch[MAX_NAME];
                    mbstowcs(wmatch, matches[tab_index], MAX_NAME);
                    wcsncpy(command + 3, wmatch, MAX_NAME - 3);
                    pos = 3 + wcslen(wmatch);
                }
            } else if (wcscmp(command, L"cd") == 0) {
                if (tab_index == -1) {
                    match_count = 0;
                    DIR *dir = opendir(".");
                    if (dir) {
                        struct dirent *entry;
                        while ((entry = readdir(dir)) != NULL) {
                            struct stat st;
                            if (stat(entry->d_name, &st) == 0 && S_ISDIR(st.st_mode)) {
                                if (entry->d_name[0] == '.' && !s_hidden) continue;
                                wchar_t wname[MAX_NAME];
                                mbstowcs(wname, entry->d_name, MAX_NAME);
                                wcstombs(matches[match_count++], wname, MAX_NAME);
                            }
                        }
                        closedir(dir);
                    }
                }

                if (match_count > 0) {
                    tab_index = (tab_index + 1) % match_count;
                    wchar_t wmatch[MAX_NAME];
                    mbstowcs(wmatch, matches[tab_index], MAX_NAME);
                    wcsncpy(command + 3, wmatch, MAX_NAME - 3);
                    pos = 3 + wcslen(wmatch);
                }
            }
        } else if (ch == 127 || ch == KEY_BACKSPACE) {
            if (pos > 0) {
                memmove(command + pos - 1, command + pos, (wcslen(command) - pos + 1) * sizeof(wchar_t));
                pos--;
                tab_index = -1;
            }
        } else if (ch == KEY_DC) {
            if (pos < wcslen(command)) {
                memmove(command + pos, command + pos + 1, (wcslen(command) - pos) * sizeof(wchar_t));
                tab_index = -1;
            }
        } else if (ch == KEY_LEFT) {
            if (pos > 0) pos--;
        } else if (ch == KEY_RIGHT) {
            if (pos < wcslen(command)) pos++;
        } else if (ch == '\n') {
            break;
        } else if (ch >= 32 && ch <= 126) {
            if (pos < MAX_NAME - 1) {
                memmove(command + pos + 1, command + pos, (wcslen(command) - pos + 1) * sizeof(wchar_t));
                command[pos++] = ch;
                tab_index = -1;
            }
        }
    }

    noecho();
    curs_set(0);

    char command_str[MAX_NAME];
    wcstombs(command_str, command, MAX_NAME);

    if (strncmp(command_str, "cd ", 3) == 0) {
        char dir[MAX_NAME];
        strncpy(dir, command_str + 3, MAX_NAME);
        if (chdir(dir) == 0) {
            getcwd(path, sizeof(path));
            list(path);
            selected = 0;
            offset = 0;
        } else {
            mvprintw(LINES - 1, 0, "Error: Directory not found.");
        }
    } else if (strcmp(command_str, "h") == 0) {
        s_hidden = 1;
        list(path);
    } else if (strcmp(command_str, "uh") == 0) {
        s_hidden = 0;
        list(path);
    } else if (strcmp(command_str, "q") == 0) {
        clear();
        endwin();
        exit(0);
    } else if (strcmp(command_str, "gg") == 0) {
        selected = 0;
    } else if (strcmp(command_str, "g") == 0) {
        selected = file_count - 1;
    } else {
        mvprintw(LINES - 1, 0, "Unknown command.");
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

    list(path);
    strncpy(lpath, path, sizeof(lpath));

    while (1) {
        UI();
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
                            list(path);
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
                if (ch == 'h') {
                    to_home();
                }

        } else {
            switch (ch) {
                case 'q':
                    clear();
                    endwin();
                    return 0;
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
                    list(path);
                    break;
                case 10:
                case 'l': {
                    char full_path[MAX_PATH];
                    snprintf(full_path, sizeof(full_path), "%s/%s", path, files[selected]);

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
                case KEY_DC: {
                    char full_path[MAX_PATH];
                    snprintf(full_path, sizeof(full_path), "%s/%s", path, files[selected]);
                
                    if (conf_del(files[selected])) {  
                        rmfd(full_path); 
                        list(path);  
                        selected = 0;  
                    }
                    break;
                }
                case 'm':
                    crt_dir();
                    list(path);
                    break;
                case 't':
                    cr_file();
                    list(path);
                    break;
                case 'r':
                    ren(files[selected]);
                    break;
                case '\\':
                    UC_view(files[selected]);
                    break;
                case KEY_F(1):  // F1
                    help_view();
                    break;

                case ':':
                    Command();
                    break;
            }
        }
    }

    endwin();
    return 0;
}

