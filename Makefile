.PHONY: clean install uninstall

TARGET_PLATFORM := unix

CC := cc
CFLAGS+= -Wall -Wextra -pedantic -O3 -g -Wstrict-prototypes -I./src
CFILES=  $(shell find src -name '*.c' -not -path "src/lib/*" -not -path "src/platform/*") \
		./src/lib/GDS/src/Vector.c ./src/lib/GDS/src/LinkedList.c \
		./src/lib/GDS/src/util/compare.c ./src/lib/str/wstr.c \
		$(wildcard src/lib/json/src/*.c) \
		$(shell find src/platform/$(TARGET_PLATFORM) -name '*.c')
OFILES= $(patsubst %.c,%.o,$(CFILES))

edit: $(OFILES)
	@ $(CC) -o edit $(OFILES) $(CFLAGS)

.c.o:
	@ echo " CC $@"
	@ $(CC) -c $< -o $@ $(CFLAGS)

INSTALL_PATH ?= /usr/local

install: edit
	@ echo "edit => $(INSTALL_PATH)/bin"
	@ echo "edit.1 => $(INSTALL_PATH)/share/man/man1"
	@ install -d $(INSTALL_PATH)/bin
	@ install -m  755 ./edit $(INSTALL_PATH)/bin
	@ install -d $(INSTALL_PATH)/share/man/man1
	@ install -m  644 edit.1 $(INSTALL_PATH)/share/man/man1

uninstall:
	@ echo " RM $(INSTALL_PATH)/bin/edit"
	@ echo " RM $(INSTALL_PATH)/share/man/man1/edit.1"
	@ rm -f $(INSTALL_PATH)/bin/edit \
			$(INSTALL_PATH)/share/man/man1/edit.1

init:
	@ git submodule update --init
	@ echo -e \
	"CompileFlags: \n" \
	"Add: -I$(shell pwd)/src/" > .clangd

clean:
	@ rm -f edit $(OFILES)
