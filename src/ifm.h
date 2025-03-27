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

void goto_cmd(int next_char);
#define MAX_FILES 16384
#define MAX_PATH 16384
#define MAX_NAME 256
#define MIN_DISPLAY_NAME 10
#define MAX_DISPLAY_NAME (COLS > 55 ? (COLS - 45) : MIN_DISPLAY_NAME)

#endif