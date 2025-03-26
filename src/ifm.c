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

#include <magic.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <limits.h> 
#include <string.h>
#include <unistd.h>
#include <locale.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <wchar.h>
#include <libgen.h>
#include <pwd.h>
#include <errno.h>


#include "icons.h"


#define MAX_FILES 16384
#define MAX_PATH 16384
#define MAX_NAME 256
#define IFM_VERSION "0.0.4"



char files[MAX_FILES][MAX_PATH];
int file_count = 0, selected = 0, offset = 0;
char path[MAX_PATH];
char lpath[MAX_PATH];
int s_hidden = 0;




typedef struct {
    char path[MAX_PATH];
    int marked;
} MarkedFile;

MarkedFile marked_files[MAX_FILES] = {0}; 

typedef struct {
    char video_viewer[MAX_NAME];
    char audio_viewer[MAX_NAME];
    char image_viewer[MAX_NAME];
    char default_viewer[MAX_NAME];
} Config;



static int last_clicked = -1;
static time_t last_click_time = 0;


extern char **environ;

int dist_s(const char *str) {
    wchar_t wstr[MAX_NAME];
    mbstowcs(wstr, str, MAX_NAME);
    return wcswidth(wstr, MAX_NAME);
}

void cls() {
    clear();
    refresh();
}

int dirt(const char *path) {
    struct stat statbuf;
    return (stat(path, &statbuf) == 0 && S_ISDIR(statbuf.st_mode));
}

int cpe(char *buffer, int max_len, const char *prompt);

void hs_path(const char *relative_path, char *absolute_path) {
    if (relative_path[0] == '/') {
        strncpy(absolute_path, relative_path, PATH_MAX);
    } else {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            snprintf(absolute_path, PATH_MAX, "%s/%s", cwd, relative_path);
        } else {
            perror("getcwd");
            strncpy(absolute_path, relative_path, PATH_MAX);
        }
    }
}






int compare(const void *a, const void *b) {
    return strcasecmp((const char *)a, (const char *)b);
}


void create_default_config() {
    const char *home_dir = getenv("HOME");
    if (!home_dir) {
        struct passwd *pw = getpwuid(getuid());
        home_dir = pw->pw_dir;
    }

    char config_dir[MAX_PATH];
    snprintf(config_dir, sizeof(config_dir), "%s/.config/ifm", home_dir);
    mkdir(config_dir, 0755);

    char config_path[MAX_PATH];
    snprintf(config_path, sizeof(config_path), "%s/config", config_dir);
    
    if (access(config_path, F_OK) != 0) {
        FILE *file = fopen(config_path, "w");
        if (file) {
            fprintf(file, "# IFM configuration file\n");
            fprintf(file, "# Default applications for different file types\n\n");
            fprintf(file, "video=vlc\n");
            fprintf(file, "audio=vlc\n");
            fprintf(file, "image=eog\n");
            fprintf(file, "other=nano\n");
            fclose(file);
        }
    }
}

void load_config(Config *config) {
    strcpy(config->video_viewer, "vlc");
    strcpy(config->audio_viewer, "vlc");
    strcpy(config->image_viewer, "eog");
    strcpy(config->default_viewer, "nano");

    const char *home_dir = getenv("HOME");
    if (!home_dir) {
        struct passwd *pw = getpwuid(getuid());
        home_dir = pw->pw_dir;
    }

    char config_path[MAX_PATH];
    snprintf(config_path, sizeof(config_path), "%s/.config/ifm/config", home_dir);

    FILE *file = fopen(config_path, "r");
    if (!file) {
        return;
    }

    char line[MAX_PATH];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0';

        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }

        char *key = strtok(line, "=");
        char *value = strtok(NULL, "=");

        if (key && value) {
            key = strtok(key, " \t");
            value = strtok(value, " \t");

            if (strcmp(key, "video") == 0) {
                strncpy(config->video_viewer, value, MAX_NAME);
            } else if (strcmp(key, "audio") == 0) {
                strncpy(config->audio_viewer, value, MAX_NAME);
            } else if (strcmp(key, "image") == 0) {
                strncpy(config->image_viewer, value, MAX_NAME);
            } else if (strcmp(key, "other") == 0) {
                strncpy(config->default_viewer, value, MAX_NAME);
            }
        }
    }

    fclose(file);
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
    Config config;
    load_config(&config);

    char full_path[MAX_PATH];
    snprintf(full_path, sizeof(full_path), "%s/%s", path, filename);

    def_prog_mode();  
    endwin();         

    magic_t magic_cookie = magic_open(MAGIC_MIME_TYPE);
    if (magic_cookie == NULL) {
        perror("magic_open failed");
        goto fallback;
    }
    
    if (magic_load(magic_cookie, NULL) != 0) {
        perror("magic_load failed");
        magic_close(magic_cookie);
        goto fallback;
    }

    const char *mime_type = magic_file(magic_cookie, full_path);
    if (mime_type == NULL) {
        perror("magic_file failed");
        magic_close(magic_cookie);
        goto fallback;
    }

    pid_t pid = fork();
    if (pid == 0) { 
        if (strncmp(mime_type, "text/", 5) == 0 || 
            strstr(mime_type, "json") || 
            strstr(mime_type, "xml") ||
            strstr(mime_type, "script")) {
            execlp(config.default_viewer, config.default_viewer, full_path, (char *)NULL);
        } 
        else {
            setsid();
            if (strncmp(mime_type, "image/", 6) == 0) {
                execlp(config.image_viewer, config.image_viewer, full_path, (char *)NULL);
            } else {
                int is_wayland = getenv("WAYLAND_DISPLAY") != NULL;
                const char *opener = is_wayland ? "wl-open" : "xdg-open";
                execlp(opener, opener, full_path, (char *)NULL);
            }
        }
        perror("execlp failed");
        exit(EXIT_FAILURE);
    } 
    else if (pid > 0) { 
        int status;
        waitpid(pid, &status, 0); 
        magic_close(magic_cookie);
        
        reset_prog_mode();
        clear();
        refresh();
    } 
    else {
        perror("fork failed");
        goto fallback;
    }
    return;

fallback:
    if (magic_cookie != NULL) {
        magic_close(magic_cookie);
    }
    
    pid_t fallback_pid = fork();
    if (fallback_pid == 0) {
        execlp(config.default_viewer, config.default_viewer, full_path, (char *)NULL);
        perror("fallback viewer failed");
        exit(EXIT_FAILURE);
    } 
    else if (fallback_pid > 0) {
        int status;
        waitpid(fallback_pid, &status, 0);
        reset_prog_mode();
        clear();
        refresh();
    } 
    else {
        perror("fork failed");
        reset_prog_mode();
        refresh();
    }
}


void cr_file() {
    char filename[MAX_NAME];
    if (cpe(filename, MAX_NAME, "Filename:")) {
        if (strlen(filename) > 0) {
            char full_path[MAX_PATH];
            snprintf(full_path, sizeof(full_path), "%s/%s", path, filename);
            FILE *file = fopen(full_path, "w");
            if (file) fclose(file);

            list(path);

            for (int i = 0; i < file_count; i++) {
                if (strcmp(files[i], filename) == 0) {
                    selected = i;
                    break;
                }
            }
        }
    }
}

void crt_dir() {
    char dirname[MAX_NAME];
    if (cpe(dirname, MAX_NAME, "Dir name")) {
        if (strlen(dirname) > 0) {
            char full_path[MAX_PATH];
            snprintf(full_path, sizeof(full_path), "%s/%s", path, dirname);
            mkdir(full_path, 0777);

            list(path);

            for (int i = 0; i < file_count; i++) {
                if (strcmp(files[i], dirname) == 0) {
                    selected = i;
                    break;
                }
            }
        }
    }
}

void ren(const char *filename) {
    setlocale(LC_ALL, ""); 

    char new_name[MAX_NAME];
    strncpy(new_name, filename, MAX_NAME - 1);  
    new_name[MAX_NAME - 1] = '\0';  

    move(LINES - 1, 0);
    clrtoeol();
    refresh();

    if (!cpe(new_name, MAX_NAME, "Rename to")) {  
        return;  
    }

    if (strlen(new_name) > 0 && strcmp(new_name, filename) != 0) {
        char old_path[MAX_PATH];
        char new_path[MAX_PATH];
        snprintf(old_path, sizeof(old_path), "%s/%s", path, filename);
        snprintf(new_path, sizeof(new_path), "%s/%s", path, new_name);

        if (access(new_path, F_OK) == 0) {
            mvprintw(LINES - 1, 0, "Error: File already exists");
            getch();
            return;
        }

        if (rename(old_path, new_path) != 0) {
            mvprintw(LINES - 1, 0, "Error: Rename failed");
            getch();
        } else {
            list(path);
            for (int i = 0; i < file_count; i++) {
                if (strcmp(files[i], new_name) == 0) {
                    selected = i;
                    break;
                }
            }
        }
    }
}

void open_with(const char *filename) {
    char viewer[MAX_NAME];

    if (!cpe(viewer, MAX_NAME, "Viewer")) {
        return;  
    }

    if (strlen(viewer) > 0) {
        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, filename);

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
            
            reset_prog_mode();
            clear();
            refresh();
        } else {
            perror("fork failed");
            reset_prog_mode();
            refresh();
        }
    }
}



void to_back() {
    char current_path[PATH_MAX]; 
    getcwd(current_path, sizeof(current_path));

    const char *home_dir = getenv("HOME");
    if (!home_dir) {
        mvprintw(LINES - 1, 0, "Error: Home directory not found.");
        return;
    }

    if (strcmp(current_path, home_dir) == 0) {
        if (chdir("/home") == 0) {
            getcwd(path, sizeof(path));
            list(path);
            selected = 0; 
            offset = 0;
        } else {
            mvprintw(LINES - 1, 0, "Error: Could not go to /home.");
        }
        return;
    }

    char current_dir[MAX_NAME];
    strncpy(current_dir, basename(current_path), MAX_NAME);

    if (chdir("..") == 0) {
        getcwd(path, sizeof(path));
        list(path);

        for (int i = 0; i < file_count; i++) {
            if (strcmp(files[i], current_dir) == 0) {
                selected = i;
                break;
            }
        }
    } else {
        mvprintw(LINES, 0, "Error: Could not go back.");
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

void UI() {
    clear();

    init_pair(3, COLOR_CYAN, COLOR_BLACK); 
    init_pair(4, COLOR_YELLOW, COLOR_BLACK);
    init_pair(5, COLOR_WHITE, COLOR_BLACK);  
    init_pair(6, COLOR_RED, COLOR_BLACK); 

    attron(COLOR_PAIR(4) | A_BOLD);
    mvprintw(0, 0, "[ IFileManager ] - ");
    wchar_t wpath[MAX_PATH];
    mbstowcs(wpath, path, MAX_PATH);
    waddwstr(stdscr, wpath);
    attroff(COLOR_PAIR(4) | A_BOLD);

    attron(COLOR_PAIR(4) | A_BOLD);
    mvprintw(0, COLS - 15, "Files: %d", file_count);
    attroff(COLOR_PAIR(4) | A_BOLD);

    int height = LINES - 3; 
    if (selected < offset) {
        offset = selected;
    } else if (selected >= offset + height) {
        offset = selected - height + 1;
    }

    for (int i = 0; i < height && i + offset < file_count; i++) {
        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, files[i + offset]);

        int is_marked = 0;
        for (int j = 0; j < MAX_FILES; j++) {
            if (strcmp(marked_files[j].path, full_path) == 0 && marked_files[j].marked) {
                is_marked = 1;
                break;
            }
        }

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
        snprintf(line, sizeof(line), "%s%s %-*s %10s %20s", 
                is_marked ? "*" : " ",
                icon, 
                30 - (name_width - strlen(files[i + offset])), 
                files[i + offset], 
                size_str, 
                date_str);

        if (i + offset == selected) attron(A_REVERSE);
        if (is_marked) attron(COLOR_PAIR(6)); 
        mvprintw(i + 2, 2, "%s", line);
        if (is_marked) attroff(COLOR_PAIR(6));
        if (i + offset == selected) attroff(A_REVERSE);

        attroff(COLOR_PAIR(1));
        attroff(COLOR_PAIR(2));
    }

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
    fprintf(pager, "  [K/J]         Move up/down\n");
    fprintf(pager, "  [Shift+J]     Move down a portion of files\n");
    fprintf(pager, "  [Shift+K]     Move up a portion of files\n");
    fprintf(pager, "  [Enter/L]     Open file/dir\n");
    fprintf(pager, "  [H]           Go back\n");
    fprintf(pager, "  [CTRL + H]    Toggle hidden files\n");
    fprintf(pager, "  [Alt + H]     Go home\n");
    fprintf(pager, "  [T]           Create file\n");
    fprintf(pager, "  [M]           Create dir\n");
    fprintf(pager, "  [DEL]         Delete file/dir\n");
    fprintf(pager, "  [R]           Rename file/dir\n");
    fprintf(pager, "  [C]           Mark/unmark file/dir\n");
    fprintf(pager, "  [Shift+R]     Rename marked files/dirs \n");
    fprintf(pager, "  [O]     Open with custom viewer\n");
    fprintf(pager, "  [Q]           Exit\n");
    fprintf(pager, "  [F1]          Help\n");
    fprintf(pager, "  [I]           Open ABOUT, LICENSE, and COMMANDS help.\n");
    fprintf(pager, "  [gg]          Go to the first file\n");
    fprintf(pager, "  [G]           Go to the last file\n");
    fprintf(pager, "\n");
    fprintf(pager, "Mouse Controls:\n");
    fprintf(pager, "  [Left Click]  Select/Open file/dir\n");
    fprintf(pager, "  [Right Click] Go back\n");
    fprintf(pager, "  [Scroll Up]   Move up\n");
    fprintf(pager, "  [Scroll Down] Move down\n");
    fprintf(pager, "\n");
    fprintf(pager, "\n");

    pclose(pager); 

    reset_prog_mode();
    refresh();
}

void fcontent(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        mvprintw(LINES - 1, 0, "Ошибка: файл %s не найден.", filename);
        refresh();
        getch();
        return;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = malloc(file_size + 1);
    if (!buffer) {
        fclose(file);
        mvprintw(LINES - 1, 0, "Ошибка: не удалось выделить память.");
        refresh();
        getch();
        return;
    }

    fread(buffer, 1, file_size, file);
    buffer[file_size] = '\0';
    fclose(file);

    int win_height = LINES / 2;
    int win_width = COLS / 2;
    int start_y = (LINES - win_height) / 2;
    int start_x = (COLS - win_width) / 2;

    WINDOW *win = newwin(win_height, win_width, start_y, start_x);
    keypad(win, TRUE);
    wattron(win, COLOR_PAIR(1));
    box(win, 0, 0);
    wattroff(win, COLOR_PAIR(1));

    int max_lines = win_height - 2;
    int max_cols = win_width - 2;  

    char *lines[1024];
    int line_count = 0;
    char *line = strtok(buffer, "\n");
    while (line != NULL && line_count < 1023) {
        int line_length = strlen(line);
        int start_pos = 0;

        while (start_pos < line_length) {
            int chunk_length = max_cols;
            if (start_pos + chunk_length > line_length) {
                chunk_length = line_length - start_pos;
            }

            lines[line_count] = malloc(chunk_length + 1);
            if (!lines[line_count]) {
                for (int i = 0; i < line_count; i++) {
                    free(lines[i]);
                }
                free(buffer);
                fclose(file);
                return;
            }

            strncpy(lines[line_count], line + start_pos, chunk_length);
            lines[line_count][chunk_length] = '\0';
            start_pos += chunk_length;
            line_count++;
        }

        line = strtok(NULL, "\n");
    }

    int offset = 0;

    while (1) {
        werase(win);
        wattron(win, COLOR_PAIR(1));
        box(win, 0, 0);
        wattroff(win, COLOR_PAIR(1));

        for (int i = 0; i < max_lines && i + offset < line_count; i++) {
            mvwprintw(win, i + 1, 1, "%s", lines[i + offset]);
        }

        wrefresh(win);

        int ch = wgetch(win);
        switch (ch) {
            case KEY_UP:
                if (offset > 0) offset--;
                break;
            case KEY_DOWN:
                if (offset < line_count - max_lines) offset++;
                break;
            case KEY_PPAGE:
                offset = (offset > max_lines) ? offset - max_lines : 0;
                break;
            case KEY_NPAGE:
                offset += max_lines;
                if (offset > line_count - max_lines) {
                    offset = line_count - max_lines;
                }
                break;
            case 27:  
            case 'q':
            case KEY_LEFT:
                for (int i = 0; i < line_count; i++) {
                    free(lines[i]);
                }
                free(buffer);
                delwin(win);
                touchwin(stdscr);
                refresh();
                return;
        }
    }
}


void abmenu() {
    int menu_selected = 0;
    const char *menu_items[] = {"LICENSE", "COMMANDS", "ABOUT"};
    int menu_count = sizeof(menu_items) / sizeof(menu_items[0]);

    int win_height = 15;
    int win_width = 35;
    int start_y = (LINES - win_height) / 2;
    int start_x = (COLS - win_width) / 2;

    WINDOW *win = newwin(win_height, win_width, start_y, start_x);
    keypad(win, TRUE);
    wattron(win, COLOR_PAIR(1)); 
    box(win, 0, 0);
    wattroff(win, COLOR_PAIR(1));

    while (1) {
        werase(win);
        wattron(win, COLOR_PAIR(1));
        box(win, 0, 0);
        wattroff(win, COLOR_PAIR(1));
        mvwprintw(win, 1, 1, "IFM");
        mvwprintw(win, 2, 1, "Lightweight file text manager.");
        mvwprintw(win, 3, 1, "Launch commands");
        mvwprintw(win, 4, 1, "ifm -? - To open this menu"); 
        mvwprintw(win, 5, 1, "ifm -h - To open help text");
        mvwprintw(win, 6, 1, "ifm PATH - To open 'PATH' dir");

        for (int i = 0; i < menu_count; i++) {
            if (i == menu_selected) {
                wattron(win, A_REVERSE);
            }
            mvwprintw(win, i + 9, 1, "%s", menu_items[i]);
            if (i == menu_selected) {
                wattroff(win, A_REVERSE);
            }
        }

        wrefresh(win);

        int ch = wgetch(win);
        switch (ch) {
            case KEY_UP:
                if (menu_selected > 0) menu_selected--;
                break;
            case KEY_DOWN:
                if (menu_selected < menu_count - 1) menu_selected++;
                break;
            case '\n':
            case KEY_RIGHT:
                if (strcmp(menu_items[menu_selected], "LICENSE") == 0) {
                    fcontent("/usr/share/ifm/LICENSE");
                } else if (strcmp(menu_items[menu_selected], "COMMANDS") == 0) {
                    fcontent("/usr/share/ifm/COMMANDS");
                } else if (strcmp(menu_items[menu_selected], "ABOUT") == 0) {
                    fcontent("/usr/share/ifm/ABOUT");
                }
                touchwin(stdscr);  
                refresh();
                break;
            case KEY_F(2):  
            case 27:
            case 'q':
                delwin(win);
                touchwin(stdscr); 
                refresh();
                return;
            case KEY_LEFT:  
                delwin(win);
                touchwin(stdscr); 
                refresh();
                return;
        }
    }
}

int cpe(char *buffer, int max_len, const char *prompt) {
    echo();  
    curs_set(1);  

    int pos = strlen(buffer);  

    while (1) {
        move(LINES - 2, 0);
        clrtoeol();  
        mvprintw(LINES - 2, 0, "%s: %s", prompt, buffer);  

        int cursor_pos = 0;
        for (int i = 0; i < pos; i++) {  
            if ((buffer[i] & 0xC0) != 0x80) {  
                cursor_pos++;
            }
        }
        move(LINES - 2, strlen(prompt) + 2 + cursor_pos);
        refresh();

        int ch = getch();  

        if (ch == '\n' || ch == KEY_ENTER) {  
            break;
        } else if (ch == 27) {  // ESC
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
                memmove(&buffer[pos], &buffer[pos + char_len], strlen(buffer) - pos - char_len + 1);
            }
        } else if (ch == KEY_DC) {  // Delete
            if (pos < strlen(buffer)) {
                int char_len = 1;
                while ((buffer[pos + char_len] & 0xC0) == 0x80) {
                    char_len++;  
                }
                memmove(&buffer[pos], &buffer[pos + char_len], strlen(buffer) - pos - char_len + 1);
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
        } else if (ch == 23) { // Ctrl + W
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

int confrim_delete(const char *filename) {
    curs_set(1);
    echo();
    char response[MAX_NAME];
    mvprintw(LINES - 2, 0, "Delete %s? (Y/n): ", filename);
    getnstr(response, MAX_NAME - 1);
    noecho();
    curs_set(0);
    return (response[0] == 'y' || response[0] == 'Y' || response[0] == '\0');
}


void reference() {
    printf("IFM - Lightweight Ncurses File Manager\n");
    printf("Version: %s\n\n", IFM_VERSION);
    printf("Usage: ifm [OPTION] [PATH]\n\n");
    printf("Options:\n");
    printf("  -h, -?    Show this help message\n");
    printf("  -V        Show version information\n");
    printf("  PATH      Open the specified directory (default: current directory)\n\n");
    printf("Examples:\n");
    printf("  ifm                     Open current directory\n");
    printf("  ifm Documents           Open 'Documents' directory\n");
    printf("  ifm -h                  Show this help message\n");
    printf("  ifm -?                  Open the menu\n");
    printf("  ifm -V                  Show version information\n");
}

void show_version() {
    printf("ifm %s\n\n", IFM_VERSION);
    printf("Copyright (c) 2025 YINMUS-IFM\n");
    printf("Released under the MIT License.\n\n");
    printf("Author: Yinmus <https://github.com/yinmus/>\n");
    printf("Please report bugs: <https://github.com/yinmus/ifm/issues>\n");
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







int main(int argc, char *argv[]) {
    setlocale(LC_ALL, "ru_RU.UTF-8");

    create_default_config();


    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0) {
            reference();
            return 0;
        } else if (strcmp(argv[1], "-?") == 0) {
            initscr();
            noecho();
            curs_set(0);
            keypad(stdscr, TRUE);
            start_color();
            init_pair(1, COLOR_CYAN, COLOR_BLACK);
            init_pair(2, COLOR_GREEN, COLOR_BLACK);

            abmenu();  

            endwin();  
            return 0;
        } else if (strcmp(argv[1], "-V") == 0) {
            show_version();
            return 0;   
            
        } else {
            hs_path(argv[1], path);
        }
    } else {
        getcwd(path, sizeof(path));
    }

    initscr();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    start_color();
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);

    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    mouseinterval(0);
    
    list(path);
    strncpy(lpath, path, sizeof(lpath));
    
    while (1) {
        UI();
        int ch = getch();

        if (ch == KEY_MOUSE) {
            MEVENT event;
            if (getmouse(&event) == OK) {
                if (event.bstate & BUTTON1_PRESSED) {
                    int y = event.y - 2;
                    if (y >= 0 && y < file_count) {
                        int clicked_item = y + offset;
                        
                        if (clicked_item == last_clicked && (time(NULL) - last_click_time) * 1000 < 500) {
                            char full_path[MAX_PATH];
                            snprintf(full_path, sizeof(full_path), "%s/%s", path, files[clicked_item]);
                            
                            if (dirt(full_path)) {
                                strncpy(lpath, path, sizeof(lpath));
                                chdir(full_path);
                                getcwd(path, sizeof(path));
                                list(path);
                                selected = 0;
                                offset = 0;
                            } else {
                                open_file(files[clicked_item]);
                            }
                            last_clicked = -1; 
                        } else {
                            selected = clicked_item;
                            last_clicked = clicked_item;
                            last_click_time = time(NULL);
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
                case KEY_LEFT:
                    to_back();
                    break;
                case 'k': 
                case KEY_UP:
                    if (selected > 0) selected--;
                    break;
            
                case 'K':
                    if (selected > 0) {
                        selected -= (file_count > 100) ? file_count / 5 : file_count / 1.5;
                    }
                    if (selected < 0) selected = 0; 
                
                
                
                    break;

                case 'j':
                case KEY_DOWN:
                    if (selected < file_count - 1) selected++;
                    break;
                case 'J':
                    if (selected < file_count - 1) {
                        selected += (file_count > 100) ? file_count / 5 : file_count / 1.5;
                    }
                    if (selected >= file_count) selected = file_count - 1;
                
                
                    break;
            
                    
                case 'G': 
                    selected = file_count - 1;
                    break;
                case 'g': { 
                    int next = getch();
                    if (next == 'g') {
                        selected = 0;
                    } else {
                        ungetch(next);
                    }
                    break;
                }
                case 8: 
                    s_hidden = !s_hidden;
                    list(path); 
                    selected = 0; 
                    offset = 0; 
                    clear();
                    UI(); 
                    break;
                case 10: 
                case 'l': 
                case KEY_RIGHT: {
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

                case 'c': {
                    char full_path[MAX_PATH];
                    snprintf(full_path, sizeof(full_path), "%s/%s", path, files[selected]);
                    
                    int found = -1;
                    for (int i = 0; i < MAX_FILES; i++) {
                        if (strcmp(marked_files[i].path, full_path) == 0) {
                            found = i;
                            break;
                        }
                    }
                    
                    if (found >= 0) {
                        marked_files[found].marked = !marked_files[found].marked;
                        if (!marked_files[found].marked) {
                            memset(marked_files[found].path, 0, MAX_PATH);
                        }
                    } else {
                        for (int i = 0; i < MAX_FILES; i++) {
                            if (marked_files[i].path[0] == '\0') {
                                strncpy(marked_files[i].path, full_path, MAX_PATH);
                                marked_files[i].marked = 1;
                                break;
                            }
                        }
                    }
                    
                    if (selected < file_count - 1) {
                        selected++;
                    }
                    break;
                }

                case KEY_DC: {
                        int any_marked = 0;
                        for (int i = 0; i < MAX_FILES; i++) {
                            if (marked_files[i].marked) any_marked = 1;
                        }
                        
                        if (any_marked) {
                            if (confrim_delete("marked files")) {
                                for (int i = 0; i < MAX_FILES; i++) {
                                    if (marked_files[i].marked) {
                                        rmfd(marked_files[i].path);
                                        memset(marked_files[i].path, 0, MAX_PATH);
                                        marked_files[i].marked = 0;
                                    }
                                }
                                list(path);
                                selected = 0;
                            }
                        } else {
                            char full_path[MAX_PATH];
                            snprintf(full_path, sizeof(full_path), "%s/%s", path, files[selected]);
                            if (confrim_delete(files[selected])) {
                                rmfd(full_path);
                                list(path);
                                selected = 0;
                            }
                        }
                        break;
                    }

                case 'R': {
                    for (int i = 0; i < MAX_FILES; i++) {
                        if (marked_files[i].marked) {
                            char *filename = basename(marked_files[i].path);
                            char dir[MAX_PATH];
                            strncpy(dir, marked_files[i].path, MAX_PATH);
                            dirname(dir);
                            
                            char new_name[MAX_NAME];
                            strncpy(new_name, filename, MAX_NAME);
                            if (cpe(new_name, MAX_NAME, "Rename to")) {
                                char new_path[MAX_PATH];
                                snprintf(new_path, sizeof(new_path), "%s/%s", dir, new_name);
                                
                                if (rename(marked_files[i].path, new_path) == 0) {
                                    strncpy(marked_files[i].path, new_path, MAX_PATH);
                                }
                            }
                        }
                    }
                    
                    list(path);
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
                case 'o':
                    open_with(files[selected]);
                    break;
                case KEY_F(1):  // F1
                    help_view();
                    break;
                case 'i':  
                    abmenu(); 
                    break;
            }
        }
    }

    endwin();
    return 0;
}
