.PHONY: clean install uninstall
CC=gcc
CFLAGS= -Wall -Wextra -pedantic -O3 -g $(FLAGS)
CFILES= $(wildcard ./src/*.c) $(wildcard ./src/highlight/*.c) \
		./src/lib/GDS/src/Vector.c ./src/lib/GDS/src/util/compare.c ./src/lib/str/wstr.c
HFILES= $(wildcard ./src/*.h)

edit: $(CFILES) $(HFILES)
	$(CC) $(CFLAGS) -o edit $(CFILES)

INSTALL_PATH ?= /usr/local

install: edit
	$(info Installing in $(INSTALL_PATH)/bin)
	install -d $(INSTALL_PATH)/bin
	install -m  755 ./edit $(INSTALL_PATH)/bin

uninstall:
	rm -f $(INSTALL_PATH)/bin/edit

clean:
	rm -f edit
