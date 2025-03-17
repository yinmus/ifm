# IFM - light, easy text file manager

<img src="logo-ifm.png" alt="scr" width="750">

____

### Installation:

The `make install ins=0` command compiles the `ifm` binary and installs it by copying necessary files: documentation (`ABOUT`, `COMMANDS`, `LICENSE`) to `/usr/share/ifm`, the icon (`ifm.png`) to `/usr/share/icons/hicolor/128x128/apps/`, and the desktop entry (`ifm.desktop`) to `/usr/share/applications/`. It also updates the desktop database to ensure proper system integration.

```
make install ins=0
```

The `make install ins=1` command performs the same installation process as `make install ins=0`, but after successful installation, it removes the ifm source directory to keep the workspace clean.

```
make install ins=1
```

The `make uninstall` command removes the IFM binary, desktop entry, and icon, deletes documentation files, updates system caches, and cleans up empty directories.

```
make uninstall
```

____

### [F1] - Call for help
#### Weight - 60Kb
___




###### p.s.  Автодополнения команды :cd работает не очень хорошо
###### [LICENSE](LICENSE) : [MIT](https://en.wikipedia.org/wiki/MIT_License)
