CC = gcc
CFLAGS = -O2
LDFLAGS = -lncursesw -ltinfo -D_GNU_SOURCE
SRC = ifm.c
BIN = ifm
PREFIX = /usr/bin
DESKTOP_FILE = assets/ifm.desktop
ICON_SRC = assets/ifm.svg
ICON_DEST = /usr/share/icons/hicolor/128x128/apps/ifm.png

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) -o $(BIN) $(SRC) $(LDFLAGS)

clean:
	rm -f $(BIN)

install: $(BIN)
	@if [ -f "$(PREFIX)/$(BIN)" ]; then \
		echo "Удаляется старый бинарник $(PREFIX)/$(BIN)"; \
		sudo rm -f "$(PREFIX)/$(BIN)"; \
	fi
	@echo "Копируется новый бинарник в $(PREFIX)"
	sudo install -m 755 $(BIN) $(PREFIX)
	
	@echo "Копируется $(DESKTOP_FILE) в /usr/share/applications/"
	sudo install -m 644 $(DESKTOP_FILE) /usr/share/applications/

	@echo "Копируется иконка в $(ICON_DEST)"
	sudo install -m 644 $(ICON_SRC) $(ICON_DEST)

	@echo "Обновление кеша .desktop файлов"
	sudo update-desktop-database /usr/share/applications/

	@echo "Установка завершена"

uninstall:
	@echo "Удаление бинарника $(PREFIX)/$(BIN)"
	sudo rm -f $(PREFIX)/$(BIN)

	@echo "Удаление .desktop файла"
	sudo rm -f /usr/share/applications/$(DESKTOP_FILE)

	@echo "Удаление иконки"
	sudo rm -f $(ICON_DEST)

	@echo "Обновление кеша .desktop файлов"
	sudo update-desktop-database /usr/share/applications/

	@echo "Обновление кеша иконок"
	sudo gtk-update-icon-cache /usr/share/icons/hicolor/

	@echo "Удаление завершено"

help:
	@echo "Доступные команды:"
	@echo "  make        - Собрать $(BIN)"
	@echo "  make clean  - Удалить скомпилированный файл"
	@echo "  make install    - Установить в $(PREFIX) и скопировать файлы"
	@echo "  make uninstall  - Удалить из $(PREFIX) и удалить файлы"
	@echo "  make help   - Показать это сообщение"
