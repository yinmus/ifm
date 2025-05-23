



<div align="center">
<H1>IFM - light, easy text file manager</H1>
<a href="https://github.com/yinmus/ifm/">
<img src="ifm-logo.png" width="750">
</a>
</div>

____

Examples of work, image of file manager



<p>
<H3> Beautiful interface with icons for many file extensions. The font must be set for the icons to display correctly</H3>
<img src="https://github.com/yinmus/a/blob/main/p1.png" width="78%" /> 
</p>

### Arch / Manjaro Linux

```
sudo pacman -S noto-fonts
```

###  Debian / Ubuntu / Linux Mint / Pop!_OS

```
sudo apt install fonts-noto
```

### Fedora

```
sudo dnf install google-noto-fonts
```



___

<p>
  <H3> Ability to label files for faster file management </H3>
 <img src="https://github.com/yinmus/a/blob/main/p2.png" width="50%" />
  
</p>

___




### Setting up the config:

Config location: **$HOME/.config/ifm/config**

When you start ifm, the config is automatically generated

About [setup](docs/CFG-GUIDE.txt)  


### Installation:

Download the latest [release](https://github.com/yinmus/ifm/releases/)
Execute the commands according to the release instructions




### Manual assembly:

(Clone repo)

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




###### [LICENSE](LICENSE) : [GPLv3](https://www.gnu.org/licenses/gpl-3.0.ru.html)
