# ifm - simple file manager
# See LICENSE.txt for copyright and license details.

PREFIX = /usr/bin

CC = gcc
CFLAGS = -O2
LDFLAGS = -lncursesw -ltinfo -lmagic

SRC = src/fmh.c src/ifm.c src/main.c src/ui.c src/goto.c src/mark.c
OBJ = fmh.o ifm.o main.o ui.o goto.o mark.o
BIN = ifm

AR_SRC = src/archiver/ar.c
AR_OBJ = ar.o
AR_BIN = ifm-ar
AR_CFLAGS = -O2
AR_LDFLAGS = -larchive

all: ${BIN} ${AR_BIN}

fmh.o: src/fmh.c
	${CC} -c ${CFLAGS} $< -o $@

ifm.o: src/ifm.c
	${CC} -c ${CFLAGS} $< -o $@

main.o: src/main.c
	${CC} -c ${CFLAGS} $< -o $@

ui.o: src/ui.c
	${CC} -c ${CFLAGS} $< -o $@

goto.o: src/goto.c
	${CC} -c ${CFLAGS} $< -o $@

mark.o: src/mark.c
	${CC} -c ${CFLAGS} $< -o $@

ar.o: src/archiver/ar.c
	${CC} -c ${AR_CFLAGS} $< -o $@

${BIN}: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

${AR_BIN}: ${AR_OBJ}
	${CC} -o $@ ${AR_OBJ} ${AR_LDFLAGS}

clean:
	rm -f ${OBJ} ${AR_OBJ}

install: all
	mkdir -p ${DESTDIR}${PREFIX}
	cp -f ${BIN} ${AR_BIN} ${DESTDIR}${PREFIX}
	chmod 755 ${DESTDIR}${PREFIX}/${BIN} ${DESTDIR}${PREFIX}/${AR_BIN}

uninstall:
	rm -f ${DESTDIR}${PREFIX}/${BIN} \
	      ${DESTDIR}${PREFIX}/${AR_BIN}

.PHONY: all clean install uninstall
