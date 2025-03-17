# IFM - light, easy text file manager

<img src="logo-ifm.png" alt="scr" width="750">

____

### Installation:

The `make install` command installs IFM by first removing any existing binary to prevent conflicts, then copying the newly compiled binary to /usr/bin/. It also installs the desktop entry in /usr/share/applications/ and places the application icon in the system icons directory for proper integration. Additionally, it creates a documentation directory in /usr/share/ifm, copies relevant documentation files, and updates the desktop database to ensure IFM appears correctly in application menus.

```
make install
```

The `make uninstall` command removes the IFM binary, desktop entry, and icon, deletes documentation files, updates system caches, and cleans up empty directories.

```
make uninstall
```

____

### [F1] - Call for help
#### Weight - 60Kb
___

###### p.s.  Автодополнения команды :cd не идеально работает
###### [LICENSE](LICENSE) : [MIT](https://en.wikipedia.org/wiki/MIT_License)
