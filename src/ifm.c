/*
======================================
    IFM by yinmus (c) 2025-2025
======================================

Relative path : ifm/src/ifm.c
Github url : https://github.com/yinmus/ifm.git
License : GPLv3 

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


#include "fmh.h"
#include "icons.h"
#include "ifm.h"




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


int cpe(char *buffer, int max_len, const char *prompt);
void goto_cmd(int next_char);
int dirt(const char *path);
int dist_s(const char *str);
void cls();
void hs_path(const char *relative_path, char *absolute_path);
int compare(const void *a, const void *b);
void hs_path(const char *relative_path, char *absolute_path);



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

void rm(const char *path) {
    struct stat st;
    
    if (lstat(path, &st) != 0) {
        mvprintw(LINES - 1, 0, "Error: Cannot access '%s' - %s", path, strerror(errno));
        refresh();
        getch();
        return;
    }

    if (S_ISLNK(st.st_mode)) {
        if (unlink(path) != 0) {
            mvprintw(LINES - 1, 0, "Error: Cannot remove symlink '%s' - %s", path, strerror(errno));
            refresh();
            getch();
        }
        return;
    }

    if (S_ISDIR(st.st_mode)) {
        DIR *dir = opendir(path);
        if (!dir) {
            mvprintw(LINES - 1, 0, "Error: Cannot open directory '%s' - %s", path, strerror(errno));
            refresh();
            getch();
            return;
        }

        struct dirent *entry;
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
                        mvprintw(LINES - 1, 0, "Error: Cannot remove directory '%s' - %s", path, strerror(errno));
                        refresh();
                        getch();
                    }
                } else {
                    mvprintw(LINES - 1, 0, "Error: Cannot change permissions for '%s' - %s", path, strerror(errno));
                    refresh();
                    getch();
                }
            } else {
                mvprintw(LINES - 1, 0, "Error: Cannot remove directory '%s' - %s", path, strerror(errno));
                refresh();
                getch();
            }
        }
    } 
    else {
        if (remove(path) != 0) {
            if (errno == EACCES) {
                if (chmod(path, 0600) == 0) {
                    if (remove(path) != 0) {
                        mvprintw(LINES - 1, 0, "Error: Cannot remove file '%s' - %s", path, strerror(errno));
                        refresh();
                        getch();
                    }
                } else {
                    mvprintw(LINES - 1, 0, "Error: Cannot change permissions for '%s' - %s", path, strerror(errno));
                    refresh();
                    getch();
                }
            } else {
                mvprintw(LINES - 1, 0, "Error: Cannot remove file '%s' - %s", path, strerror(errno));
                refresh();
                getch();
            }
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
    if (cpe(filename, MAX_NAME, "Filename")) {
        if (strlen(filename) > 0) {
            char full_path[MAX_PATH];
            snprintf(full_path, sizeof(full_path), "%s/%s", path, filename);
            
            struct stat st;
            if (stat(full_path, &st) == 0) {
                mvprintw(LINES - 1, 0, "Error: File or directory already exists");
                getch();
                return;
            }
            
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
            
            struct stat st;
            if (stat(full_path, &st) == 0) {
                mvprintw(LINES - 1, 0, "Error: File or directory already exists");
                getch();
                return;
            }
            
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
    setlocale(LC_ALL, "");

    init_pair(1, COLOR_CYAN, COLOR_BLACK);   
    init_pair(2, COLOR_GREEN, COLOR_BLACK);  
    init_pair(3, COLOR_CYAN, COLOR_BLACK);   
    init_pair(4, COLOR_YELLOW, COLOR_BLACK); 
    init_pair(5, COLOR_WHITE, COLOR_BLACK);  
    init_pair(6, COLOR_RED, COLOR_BLACK);    
    init_pair(7, COLOR_BLACK, COLOR_WHITE);

    attron(COLOR_PAIR(4) | A_BOLD);
    mvprintw(0, 0, "[ IFM ] - ");
    wchar_t wpath[MAX_PATH];
    mbstowcs(wpath, path, MAX_PATH);
    waddwstr(stdscr, wpath);
    attroff(COLOR_PAIR(4) | A_BOLD);
    mvprintw(0, COLS - 15, "Files: %d", file_count);

    int height = LINES - 4; 
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
            if (marked_files[j].marked && strcmp(marked_files[j].path, full_path) == 0) {
                is_marked = 1;

                break;
            }
        }

        char truncated_name[MAX_DISPLAY_NAME + 1];
        if (strlen(files[i + offset]) > MAX_DISPLAY_NAME) {
            snprintf(truncated_name, sizeof(truncated_name), "%.*s...", MAX_DISPLAY_NAME - 3, files[i + offset]);
        } else {
            strncpy(truncated_name, files[i + offset], MAX_DISPLAY_NAME);
            truncated_name[MAX_DISPLAY_NAME] = '\0';
        }

        struct stat st;
        const char *icon = "";
        int color_pair = 5; 

        if (stat(full_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                icon = "";
                color_pair = 1;
            } else if (S_ISREG(st.st_mode)) {
                color_pair = 2;
                const char *ext = strrchr(files[i + offset], '.');
                if (ext) icon = icon_ext(ext + 1);
            }
        }

        if (i + offset == selected) attron(A_REVERSE);
        
        if (is_marked) {
            attron(COLOR_PAIR(6));
            mvaddch(i + 2, 2, '*');
            attroff(COLOR_PAIR(6));
        } else {
            mvaddch(i + 2, 2, ' ');
        }
        
        if (color_pair) attron(COLOR_PAIR(color_pair));
        mvprintw(i + 2, 3, "%s %-*s", icon, MAX_DISPLAY_NAME, files[i + offset]);
        if (color_pair) attroff(COLOR_PAIR(color_pair));
        
        if (i + offset == selected) attroff(A_REVERSE);
    }

    if (file_count > 0 && selected >= 0 && selected < file_count) {
        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, files[selected]);
        
        struct stat st;
        if (stat(full_path, &st) == 0) {
            double size = st.st_size;
            const char *unit = "B";
            if (size > 1024) { size /= 1024; unit = "KB"; }
            if (size > 1024) { size /= 1024; unit = "MB"; }
            if (size > 1024) { size /= 1024; unit = "GB"; }
            
            char date_str[20];
            strftime(date_str, sizeof(date_str), "%Y-%m-%d %H:%M", localtime(&st.st_mtime));
            
            char perms[10];
            snprintf(perms, sizeof(perms), "%c%c%c%c%c%c%c%c%c",
                (S_ISDIR(st.st_mode)) ? 'd' : '-',
                (st.st_mode & S_IRUSR) ? 'r' : '-',
                (st.st_mode & S_IWUSR) ? 'w' : '-',
                (st.st_mode & S_IXUSR) ? 'x' : '-',
                (st.st_mode & S_IRGRP) ? 'r' : '-',
                (st.st_mode & S_IWGRP) ? 'w' : '-',
                (st.st_mode & S_IXGRP) ? 'x' : '-',
                (st.st_mode & S_IROTH) ? 'r' : '-',
                (st.st_mode & S_IWOTH) ? 'w' : '-',
                (st.st_mode & S_IXOTH) ? 'x' : '-');
            
            const char *file_type = S_ISDIR(st.st_mode) ? "Directory" : 
                                  S_ISREG(st.st_mode) ? "File" : "Special";
            
            attron(COLOR_PAIR(3) | A_BOLD); 
            mvprintw(LINES - 2, 0, "%.2f %s | %s | %s ", 
                 size, unit, date_str, perms);
            int current_len = strlen(files[selected]) + strlen(file_type) + strlen(date_str) + 
                            strlen(perms) + 60;
            for (int i = current_len; i < COLS; i++) {
                addch(' ');
            }
            attroff(COLOR_PAIR(3) | A_BOLD);
        }
    } else {
        attron(COLOR_PAIR(3));
        for (int i = 0; i < COLS; i++) {
            mvaddch(LINES - 2, i, ' ');
        }
        attroff(COLOR_PAIR(3));
    }

    refresh();
}



void show_marked_files() {
    int marked_count = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        if (marked_files[i].marked) marked_count++;
    }

    if (marked_count == 0) {
        mvprintw(LINES - 1, 0, "No files marked");
        refresh();
        getch();
        return;
    }

    int win_height = (marked_count + 4 < LINES - 4) ? marked_count + 4 : LINES - 4;
    int win_width = COLS - 10;
    int start_y = (LINES - win_height) / 2;
    int start_x = (COLS - win_width) / 2;

    

    WINDOW *win = newwin(win_height, win_width, start_y, start_x);
    keypad(win, TRUE);
    wattron(win, COLOR_PAIR(1));
    box(win, 0, 0);
    wattroff(win, COLOR_PAIR(1));
    mvwprintw(win, 0, 2, " Marked Files (%d) ", marked_count);

    int line = 1;
    for (int i = 0; i < MAX_FILES && line < win_height - 1; i++) {
        if (marked_files[i].marked) {
            char *filename = basename(marked_files[i].path);
            
            const char *icon = "";
            struct stat st;
            if (stat(marked_files[i].path, &st) == 0) {
                if (S_ISDIR(st.st_mode)) {
                    icon = ""; 
                } else if (S_ISREG(st.st_mode)) {
                    const char *ext = strrchr(filename, '.');
                    if (ext) icon = icon_ext(ext + 1);
                }
            }
            
            mvwprintw(win, line++, 2, "%s %s", icon, marked_files[i].path);
        }
    }

    wrefresh(win);
    wgetch(win);
    delwin(win);
}


void mark_help() {
    int win_height = 12;  
    int win_width = 50;   
    int start_y = (LINES - win_height) / 1.3;
    int start_x = 0;

    WINDOW *win = newwin(win_height, win_width, start_y, start_x);
    keypad(win, TRUE);
    wattron(win, COLOR_PAIR(1));
    box(win, 0, 0);
    wattroff(win, COLOR_PAIR(1));

    mvwprintw(win, 1, 2, "Macros for managing files with tags:");
    mvwprintw(win, 2, 2, "G - Mark from current to end");
    mvwprintw(win, 3, 2, "g - Mark from current to start");
    mvwprintw(win, 4, 2, "uG - Unmark from current to end");
    mvwprintw(win, 5, 2, "ug - Unmark from current to start");
    mvwprintw(win, 7, 2, "R - Rename marked files");
    mvwprintw(win, 8, 2, "c - Toggle mark on current file");

    wrefresh(win);

    int ch = getch();
    int next_ch = 0;
    
    if (ch == 'u') {
        next_ch = getch();
    }

    delwin(win);
    touchwin(stdscr);
    refresh();

    if (ch != ERR) {
        switch(ch) {
            case 'G':
                if (next_ch == 0) {
                    for (int i = selected; i < file_count; i++) {
                        char full_path[MAX_PATH];
                        snprintf(full_path, sizeof(full_path), "%s/%s", path, files[i]);
                        
                        int already_marked = 0;
                        for (int j = 0; j < MAX_FILES; j++) {
                            if (marked_files[j].marked && strcmp(marked_files[j].path, full_path) == 0) {
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
                
            case 'g': {
                
                for (int i = selected; i >= 0; i--) {
                    char full_path[MAX_PATH];
                    snprintf(full_path, sizeof(full_path), "%s/%s", path, files[i]);
                        
                    int already_marked = 0;
                    for (int j = 0; j < MAX_FILES; j++) {
                        if (marked_files[j].marked && strcmp(marked_files[j].path, full_path) == 0) {
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
                    break;
                }
            
            case 'u': {
                if (next_ch == 'G') {
                    for (int i = selected; i < file_count; i++) {
                        char full_path[MAX_PATH];
                        snprintf(full_path, sizeof(full_path), "%s/%s", path, files[i]);
                        
                        for (int j = 0; j < MAX_FILES; j++) {
                            if (marked_files[j].marked && strcmp(marked_files[j].path, full_path) == 0) {
                                marked_files[j].marked = 0;
                                memset(marked_files[j].path, 0, MAX_PATH);
                                break;
                            }
                        }
                    }
                } 
                else if (next_ch == 'g') {
                        for (int i = selected; i >= 0; i--) {
                            char full_path[MAX_PATH];
                            snprintf(full_path, sizeof(full_path), "%s/%s", path, files[i]);
                            
                            for (int j = 0; j < MAX_FILES; j++) {
                                if (marked_files[j].marked && strcmp(marked_files[j].path, full_path) == 0) {
                                    marked_files[j].marked = 0;
                                    memset(marked_files[j].path, 0, MAX_PATH);
                                    break;
                                }
                            }
                        }
                    }
                }
                break;
            }
        }
    }



void fcontent(const char *filename) {
    char tmp_path[MAX_PATH];
    snprintf(tmp_path, sizeof(tmp_path), "/tmp/ifm_man_%d", getpid());
    
    char cmd[MAX_PATH * 2];
    snprintf(cmd, sizeof(cmd), "man -l \"%s\" > \"%s\" 2>&1", filename, tmp_path);
    
    system(cmd);
    
    FILE *man_output = fopen(tmp_path, "r");
    if (!man_output) {
        mvprintw(LINES - 1, 0, "Error: Failed to get man output");
        refresh();
        getch();
        return;
    }

    int win_height = LINES - 4;
    int win_width = COLS - 4;
    int start_y = 2;
    int start_x = 2;
    
    WINDOW *win = newwin(win_height, win_width, start_y, start_x);
    keypad(win, TRUE);
    wattron(win, COLOR_PAIR(1));
    box(win, 0, 0);
    wattroff(win, COLOR_PAIR(1));
    
    char line[win_width];
    int line_num = 0;
    int offset = 0;
    
    char *lines[1000]; 
    int total_lines = 0;
    
    while (fgets(line, sizeof(line), man_output) && total_lines < 1000) {
        line[strcspn(line, "\n")] = '\0'; 
        lines[total_lines] = strdup(line);
        total_lines++;
    }
    fclose(man_output);
    unlink(tmp_path); 

    while (1) {
        werase(win);
        wattron(win, COLOR_PAIR(1));
        box(win, 0, 0);
        mvwprintw(win, 0, 2, " MAN: %s ", filename);
        wattroff(win, COLOR_PAIR(1));
        
        for (int i = 0; i < win_height - 2 && i + offset < total_lines; i++) {
            mvwprintw(win, i + 1, 1, "%.*s", win_width - 2, lines[i + offset]);
        }
        
        if (total_lines > win_height - 2) {
            int scroll_pos = (offset * (win_height - 4)) / total_lines;
            mvwaddch(win, 1 + scroll_pos, win_width - 1, ACS_CKBOARD);
        }
        
        wrefresh(win);
        
        int ch = wgetch(win);
        switch (ch) {
            case KEY_UP:
                if (offset > 0) offset--;
                break;
            case KEY_DOWN:
                if (offset < total_lines - (win_height - 2)) offset++;
                break;
            case KEY_PPAGE:
                offset = (offset > (win_height - 2)) ? offset - (win_height - 2) : 0;
                break;
            case KEY_NPAGE:
                offset += (win_height - 2);
                if (offset > total_lines - (win_height - 2)) {
                    offset = total_lines - (win_height - 2);
                }
                break;
            case 'g':
                offset = 0;
                break;
            case 'G':
                offset = total_lines - (win_height - 2);
                if (offset < 0) offset = 0;
                break;
            case KEY_HOME:
                offset = 0;
                break;
            case KEY_END:
                offset = total_lines - (win_height - 2);
                if (offset < 0) offset = 0;
                break;
            case 27: 
            case 'q':
            case 'i':
            case KEY_LEFT:
                for (int i = 0; i < total_lines; i++) {
                    free(lines[i]);
                }
                delwin(win);
                touchwin(stdscr);
                refresh();
                return;
        }
    }
}

void doc_menu() {
    int menu_selected = 0;
    const char *menu_items[] = {"LICENSE", "COMMANDS", "ABOUT", "CONFIG"};
    int menu_count = sizeof(menu_items) / sizeof(menu_items[0]);

    int win_height = 20;
    int win_width = 80;
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
                    fcontent("/usr/share/doc/ifm/LICENSE.txt");
                } else if (strcmp(menu_items[menu_selected], "COMMANDS") == 0) {
                    fcontent("/usr/share/doc/ifm/COMMANDS.txt");
                } else if (strcmp(menu_items[menu_selected], "ABOUT") == 0) {
                    fcontent("/usr/share/doc/ifm/ABOUT.txt");
                } else if (strcmp(menu_items[menu_selected], "CONFIG") == 0) {
                    fcontent("/usr/share/doc/ifm/CFG-GUIDE.txt");
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
            case 'i':  
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



void get_info(const char *filename, char *info, size_t info_size) {
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



void goto_cmd(int next_char) {
    char target_path[MAX_PATH];
    
    switch(next_char) {
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
	case 'g':
	    selected = 0;
	    offset = 0;
	    return;
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
        list(path);
        selected = 0;
        offset = 0;
    }
}


int dist_s(const char *str) {
    setlocale(LC_ALL, "");
    int width = 0;
    wchar_t wc;
    const char *ptr = str;
    size_t len = strlen(str);
    
    while (*ptr != '\0' && len > 0) {
        int consumed = mbtowc(&wc, ptr, len);
        if (consumed <= 0) break;
        width += wcwidth(wc);
        ptr += consumed;
        len -= consumed;
    }
    return width;
}

void cls() {
    clear();
    refresh();
}

int dirt(const char *path) {
    struct stat statbuf;
    return (stat(path, &statbuf) == 0 && S_ISDIR(statbuf.st_mode));
}

int compare(const void *a, const void *b) {
    return strcasecmp((const char *)a, (const char *)b);
}


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

            doc_menu();  

            endwin();  
            return 0;
        } else if (strcmp(argv[1], "-V") == 0) {
            Version();
            return 0;
        } else {
            hs_path(argv[1], path);
            struct stat st;
            if (stat(path, &st) != 0 || !S_ISDIR(st.st_mode)) {
                getcwd(path, sizeof(path));
            }
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
                        
                        if (clicked_item == last_clicked && (time(NULL) - last_click_time) * 1000 < 2000) {
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
                        selected -= 10;
                    }
                    if (selected < 0) selected = 0; 
                
                
                
                    break;

                case 'j':
                case KEY_DOWN:
                    if (selected < file_count - 1) selected++;
                    break;
                case 'J':
                    if (selected < file_count - 1) {
                        selected += 10;
                    }
                    if (selected >= file_count) selected = file_count - 1;
                
                
                    break;
            
                    
                case 'G': 
                    selected = file_count - 1;
                    break;
                case 'g': { 
                    goto_help();
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
                    
                    struct stat st;
                    if (stat(full_path, &st) != 0) {
                        break; 
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
                                        rm(marked_files[i].path);
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
                                rm(full_path);
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

                case 'i':  
                    doc_menu(); 
                    break;

                case 'M':
                    show_marked_files();
                    break;
                
                case 'e':
                    mark_help();
                    break;
                
                case KEY_PPAGE:  // Page Up
                    if (selected > 34) {
                        selected -= 35;
                    } else {
                        selected = 0;
                    }
                    break;
                
                case KEY_NPAGE:  // Page Down
                    if (selected < file_count - 35) {
                        selected += 35;
                    } else {
                        selected = file_count - 1;
                    }
                    break;
                    
                                      
            }
        }
    }

    endwin();
    return 0;
}
