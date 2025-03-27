/*
======================================
    IFM by yinmus (c) 2025-2025
======================================

Relative path : ifm/src/fmh.c
Github url : https://github.com/yinmus/ifm.git
License : GPLv3 

*/

#include "fmh.h"
#include "ifm.h"

#include <ncurses.h>
#include <stdlib.h>
#include <stdio.h>

#define IFM_VERSION "0.0.4"



void goto_help() {
    int win_height = 16;
    int win_width = 40;
    int start_y = (LINES - win_height) / 1.3;
    int start_x = 0;

    WINDOW *win = newwin(win_height, win_width, start_y, start_x);
    keypad(win, TRUE);
    wattron(win, COLOR_PAIR(1));
    box(win, 0, 0);
    wattroff(win, COLOR_PAIR(1));

    mvwprintw(win, 1, 2, "Go to:");
    mvwprintw(win, 2, 2, "h - cd ~");
    mvwprintw(win, 3, 2, "/ - cd /");
    mvwprintw(win, 4, 2, "d - cd /dev"); 
    mvwprintw(win, 5, 2, "e - cd /etc"); 
    mvwprintw(win, 6, 2, "m - cd /media or /run/media");
    mvwprintw(win, 7, 2, "M - cd /mnt");
    mvwprintw(win, 8, 2, "o - cd /opt");
    mvwprintw(win, 9, 2, "t - cd /tmp");
    mvwprintw(win, 10, 2, "u - cd /usr");
    mvwprintw(win, 11, 2, "s - cd /srv");
    mvwprintw(win, 12, 2, "? - cd /usr/share/doc");
    mvwprintw(win, 13, 2, "g - goto first file"); 

    wrefresh(win);

    int ch = getch();
    delwin(win);
    touchwin(stdscr);
    refresh();

    if (ch != ERR) {
        goto_cmd(ch);
    }
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

void Version() {
    printf("ifm %s\n\n", IFM_VERSION);
    printf("Copyright (c) 2025 YINMUS-IFM\n");
    printf("Released under the MIT License.\n\n");
    printf("Author: Yinmus <https://github.com/yinmus/>\n");
    printf("Please report bugs: <https://github.com/yinmus/ifm/issues>\n");
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

