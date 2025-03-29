/*
======================================
    IFM by yinmus (c) 2025-2025
======================================

Relative path : ifm/src/ui.c
Github url : https://github.com/yinmus/ifm.git
License : GPLv3

*/

#include <locale.h>
#include <ncurses.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <wchar.h>
#include <stdlib.h>
#include "ui.h"
#include "icons.h"
#include "ifm.h"
#include "fmh.h"    

void UI()
{
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
    if (selected < offset)
    {
        offset = selected;
    }
    else if (selected >= offset + height)
    {
        offset = selected - height + 1;
    }

    for (int i = 0; i < height && i + offset < file_count; i++)
    {
        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, files[i + offset]);

        int is_marked = 0;
        for (int j = 0; j < MAX_FILES; j++)
        {
            if (marked_files[j].marked && strcmp(marked_files[j].path, full_path) == 0)
            {
                is_marked = 1;

                break;
            }
        }

        char truncated_name[MAX_DISPLAY_NAME + 1];
        if (strlen(files[i + offset]) > MAX_DISPLAY_NAME)
        {
            snprintf(truncated_name, sizeof(truncated_name), "%.*s...", MAX_DISPLAY_NAME - 3, files[i + offset]);
        }
        else
        {
            strncpy(truncated_name, files[i + offset], MAX_DISPLAY_NAME);
            truncated_name[MAX_DISPLAY_NAME] = '\0';
        }

        struct stat st;
        const char *icon = "";
        int color_pair = 5;

        if (stat(full_path, &st) == 0)
        {
            if (S_ISDIR(st.st_mode))
            {
                // icon = "";
		        icon = "";

                color_pair = 1;
            }
            else if (S_ISREG(st.st_mode))
            {
                color_pair = 2;
                const char *ext = strrchr(files[i + offset], '.');
                if (ext)
                    icon = icon_ext(ext + 1);
            }
        }

        if (i + offset == selected)
            attron(A_REVERSE);

        if (is_marked)
        {
            attron(COLOR_PAIR(6));
            mvaddch(i + 2, 2, '*');
            attroff(COLOR_PAIR(6));
        }
        else
        {
            mvaddch(i + 2, 2, ' ');
        }

        if (color_pair)
            attron(COLOR_PAIR(color_pair));
        mvprintw(i + 2, 3, "%s %-*s", icon, MAX_DISPLAY_NAME, files[i + offset]);
        if (color_pair)
            attroff(COLOR_PAIR(color_pair));

        if (i + offset == selected)
            attroff(A_REVERSE);
    }

    if (file_count > 0 && selected >= 0 && selected < file_count)
    {
        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, files[selected]);

        struct stat st;
        if (stat(full_path, &st) == 0)
        {
            double size = st.st_size;
            const char *unit = "B";
            if (size > 1024)
            {
                size /= 1024;
                unit = "KB";
            }
            if (size > 1024)
            {
                size /= 1024;
                unit = "MB";
            }
            if (size > 1024)
            {
                size /= 1024;
                unit = "GB";
            }

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

            const char *file_type = S_ISDIR(st.st_mode) ? "Directory" : S_ISREG(st.st_mode) ? "File"
                                                                                            : "Special";

            attron(COLOR_PAIR(3) | A_BOLD);
            mvprintw(LINES - 2, 0, "%.2f %s | %s | %s ",
                     size, unit, date_str, perms);
            int current_len = strlen(files[selected]) + strlen(file_type) + strlen(date_str) +
                              strlen(perms) + 60;
            for (int i = current_len; i < COLS; i++)
            {
                addch(' ');
            }
            attroff(COLOR_PAIR(3) | A_BOLD);
        }
    }
    else
    {
        attron(COLOR_PAIR(3));
        for (int i = 0; i < COLS; i++)
        {
            mvaddch(LINES - 2, i, ' ');
        }
        attroff(COLOR_PAIR(3));
    }

    refresh();
}


void show_marked_files()
{
    int marked_count = 0;
    for (int i = 0; i < MAX_FILES; i++)
    {
        if (marked_files[i].marked)
            marked_count++;
    }

    if (marked_count == 0)
    {
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
    for (int i = 0; i < MAX_FILES && line < win_height - 1; i++)
    {
        if (marked_files[i].marked)
        {
            char *filename = basename(marked_files[i].path);

            const char *icon = "";
            struct stat st;
            if (stat(marked_files[i].path, &st) == 0)
            {
                if (S_ISDIR(st.st_mode))
                {
                    icon = "";
                }
                else if (S_ISREG(st.st_mode))
                {
                    const char *ext = strrchr(filename, '.');
                    if (ext)
                        icon = icon_ext(ext + 1);
                }
            }

            mvwprintw(win, line++, 2, "%s %s", icon, marked_files[i].path);
        }
    }

    wrefresh(win);
    wgetch(win);
    delwin(win);
}
