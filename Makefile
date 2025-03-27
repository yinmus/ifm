CC = gcc
CFLAGS = -O2
LDFLAGS = -lncursesw -ltinfo -lmagic -D_GNU_SOURCE
SRC = src/ifm.c src/fmh.c
BIN = ifm
PREFIX = /usr/bin
DESKTOP_FILE = assets/ifm.desktop
ICON_SRC = assets/ifm.png
ICON_DEST = /usr/share/icons/hicolor/128x128/apps/ifm.png
DOC_FILES = docs/ABOUT.txt docs/COMMANDS.txt docs/CFG-GUIDE.txt LICENSE docs/CFG-GUIDE.txt
DOC_DIR = /usr/share/doc/ifm

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) -o $(BIN) $(SRC) $(LDFLAGS)

clean:
	rm -f $(BIN)

install:
	@if [ "$(ins)" = "0" ] || [ "$(ins)" = "1" ]; then \
		$(MAKE) install_bas; \
		if [ "$(ins)" = "1" ]; then \
			echo "Removing source directory ifm"; \
			cd .. && rm -rf ifm; \
		fi \
	else \
		echo "Usage: make install ins=0 or make install ins=1"; \
	fi

install_bas:
	@echo "Compiling and installing $(BIN)"
	$(MAKE) all

	@echo "Installing binary to $(PREFIX)"
	sudo install -m 755 $(BIN) $(PREFIX)

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

	@echo "Removing .desktop file"
	sudo rm -f /usr/share/applications/$(DESKTOP_FILE)

	@echo "Removing icon"
	sudo rm -f $(ICON_DEST)

	@echo "Removing documentation files from $(DOC_DIR)"
	sudo rm $(DOC_DIR)/*


	@echo "Updating desktop and icon caches"
	sudo update-desktop-database /usr/share/applications/
	sudo gtk-update-icon-cache /usr/share/icons/hicolor/

	@echo "Uninstallation completed"

help:
	@echo "Available commands:"
	@echo "  make          - Compile $(BIN)"
	@echo "  make clean    - Remove compiled binary"
	@echo "  make install ins=0 - Compile and install"
	@echo "  make install ins=1 - Install and delete 'ifm' source directory"
	@echo "  make uninstall - Uninstall $(BIN)"
	@echo "  make help     - Show this help message"
