
CC = gcc
CFLAGS = -O2
LDFLAGS = -lncursesw -ltinfo -lmagic -D_GNU_SOURCE
SRC = src/fmh.c src/ifm.c src/main.c src/ui.c src/goto.c src/cfg.c src/mark.c
BIN = ifm 

AR_CFLAGS = -O2
AR_LDFLAGS = -larchive
AR_SRC = src/oer/archiver/ar.c
AR_BIN = ifm-ar

PREFIX = /usr/bin
DESKTOP_FILE = ifm.desktop
ICON_SRC = ifm.png
ICON_DEST = /usr/share/icons/hicolor/128x128/apps/ifm.png
DOC_FILES = docs/commands.txt LICENSE.txt 
DOC_DIR = /usr/share/doc/ifm

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) -o $(BIN) $(SRC) $(LDFLAGS)
	$(CC) $(AR_CFLAGS) -o $(AR_BIN) $(AR_SRC) $(AR_LDFLAGS)

clean:
	rm -f $(BIN)
	rm -f $(AR_BIN)

install: all
	@echo "Installing binary to $(PREFIX)"
	sudo install -m 755 $(BIN) $(AR_BIN) $(PREFIX)

	@echo "Installing $(DESKTOP_FILE) to /usr/share/applications/"
	sudo install -m 644 $(DESKTOP_FILE) /usr/share/applications/

	@echo "Installing icon to $(ICON_DEST)"
	sudo install -m 644 $(ICON_SRC) $(ICON_DEST)

	@echo "Creating documentation directory $(DOC_DIR)"
	sudo mkdir -p $(DOC_DIR)

	@echo "Copying documentation files to $(DOC_DIR)"

	for file in $(DOC_FILES); do \
		sudo install -m 644 $$file $(DOC_DIR); \
	done

	@echo "Updating desktop database"
	sudo update-desktop-database /usr/share/applications/

	@echo "Installation completed"

uninstall:
	@echo "Removing binary $(PREFIX)/$(BIN)"
	sudo rm -f $(PREFIX)/$(BIN)
	sudo rm -f $(PREFIX)/$(AR_BIN)

	@echo "Removing .desktop file"
	sudo rm -f /usr/share/applications/$(DESKTOP_FILE)

	@echo "Removing icon"
	sudo rm -f $(ICON_DEST)

	@echo "Removing documentation files from $(DOC_DIR)"
	sudo rm -f $(DOC_DIR)/*

	@echo "Updating desktop and icon caches"
	sudo update-desktop-database /usr/share/applications/
	sudo gtk-update-icon-cache /usr/share/icons/hicolor/

	@echo "Uninstallation completed"

help:
	@echo " make           - Compile $(BIN)"
	@echo " make clean     - Remove compiled binaries"
	@echo " make install   - Compile and install the program"
	@echo " make uninstall - Uninstall the program"
	@echo " make help      - Show this help message"

.PHONY: all clean install uninstall help
