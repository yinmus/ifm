
ifm is a lightweight file manager focused on speed.
=================================================================
It has many hotkeys that can be customized (src/config.h). There are icons that can be disabled:
When compiling: *icons will not be available at all
make install NOICONS=1
During program execution:
*in command mode
:set sd 0
    *Specify the default value of the sd variable:
    *in the file /src/ifm.c, change the value of the sd variable 

===================================================================
It also has a built-in archiver that simplifies working with archives. You can view the list of files and unpack them.

About hotkeys: 
*docs/about.txt

*About console mode:
*docs/console.txt
===================================================================

About hotkeys: 
*docs/about.txt

*About console mode:
*docs/console.txt
===================================================================

installation:
$ git clone https://github.com/yinmus/ifm.git
$ cd ifm
# make install

LICENSE: GPLv3 (https://www.gnu.org/licenses/gpl-3.0.ru.html)
