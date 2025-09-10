# ifm - simple file manager
# See LICENSE.txt for copyright and license details.

PREFIX = /usr/bin

CC = gcc
CFLAGS = -O2
LDFLAGS = -lncursesw -ltinfo -lmagic

SRC = src/fmh.c src/ifm.c src/main.c src/ui.c src/goto.c src/mark.c
OBJ = $(SRC:src/%.c=src/%.o)  
BIN = ifm

AR_SRC = src/archiver/ar.c
AR_OBJ = src/archiver/ar.o  
AR_BIN = ifm-ar
AR_CFLAGS = -O2
AR_LDFLAGS = -larchive

all: ${BIN} ${AR_BIN}

src/%.o: src/%.c
	${CC} -c ${CFLAGS} $< -o $@

src/archiver/ar.o: src/archiver/ar.c
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
