IFM - light, easy file manager
=======================================================================================
Текстовый файловый менеджер, использующий ncurses. 
Не спорю, проект еще сырой, но все впереди, планируются улучшения. Может, что-то да выйдет
=======================================================================================

Установка:

1. Клонируйте репо 

git clone https://github.com/yinmus/ifm.git

=======================================================================================

2. Перейдите в директорию

cd ifm

=======================================================================================

3. Запустите установку

make install

=======================================================================================

Управление:

    IFM - Lightweight Ncurses File Manager
    ======================================
    Controls:
      [K/J]        Move selection up/down through the file list.
      [Enter/L]    Open the selected file or navigate into a directory
      [H]          Go back.
      [Alt + U]    Toggle visibility of hidden files (files starting with '.').
      [Alt + H]    Jump to the home directory.
      [T]          Create a new file in the current directory.
      [Shift + D]  Create a new directory in the current directory.
      [D]          Delete the selected file or directory (confirmation required).
      [Q]          Exit IFM (confirmation required).
      [Alt + A]    Open this help manual.
    
    Mouse:
      Left-click   Select/open files.
      Right-click  Go back.
      Scroll       Use the mouse wheel or [K/J] keys.

=======================================================================================
    
LICENSE : MIT
