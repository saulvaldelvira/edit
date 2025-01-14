.PHONY: clean install uninstall

TARGET_PLATFORM := unix

ifeq ($(TARGET_PLATFORM),win32)
	CC := x86_64-w64-mingw32-gcc
	CLANGD_PLATFORM =  , --target=x86_64-w64-mingw32
	CFLAGS = -lws2_32 -lshlwapi
	EXECUTABLE = edit.exe
else
	CC := cc
	EXECUTABLE = edit
endif

GDS_HOME=src/lib/GDS
JSON_HOME=src/lib/json

INCLUDE_DIRS = -I./src -I./src/lib/GDS/include
CFLAGS += -Wall -Wextra -pedantic -Wstrict-prototypes -ggdb \
		  $(INCLUDE_DIRS) $(FLAGS) \
		  -L$(GDS_HOME)/bin -lGDS-static -L$(JSON_HOME)/bin -ljson-static

PROFILE := debug

ifeq ($(PROFILE),release)
	CFLAGS += -O3
endif

LIBFILES= ./src/lib/str/wstr.c
CFILES=  $(shell find src -name '*.c' -not -path "src/lib/*" -not -path "src/platform/*") \
		 $(LIBFILES) \
		 $(shell find src/platform/$(TARGET_PLATFORM) -name '*.c')

OFILES= $(patsubst %.c,%.o,$(CFILES))

edit: $(OFILES)
	@ make -C $(GDS_HOME)
	@ make -C $(JSON_HOME)
	@ echo " LD => edit"
	@ $(CC) -o $(EXECUTABLE) $(OFILES) $(CFLAGS)

.c.o:
	@ echo " CC $@"
	@ $(CC) -c $< -o $@ $(CFLAGS)

PREFIX ?= /usr/local

install: edit
	@ echo "edit => $(PREFIX)/bin"
	@ echo "edit.1 => $(PREFIX)/share/man/man1"
	@ install -d $(PREFIX)/bin
	@ install -m  755 ./edit $(PREFIX)/bin
	@ install -d $(PREFIX)/share/man/man1
	@ install -m  644 edit.1 $(PREFIX)/share/man/man1

uninstall:
	@ echo " RM $(PREFIX)/bin/edit"
	@ echo " RM $(PREFIX)/share/man/man1/edit.1"
	@ rm -f $(PREFIX)/bin/edit \
			$(PREFIX)/share/man/man1/edit.1

init:
	@ git submodule update --init
	@ echo -e \
		"CompileFlags: \n" \
		"Add: [ -I$(shell pwd)/src/, " \
		"-I$(shell pwd)/src/lib/GDS/include/ , " \
		" -xc $(CLANGD_PLATFORM), " \
		"-std=c23 ]" > .clangd

CLEAN_LIBS:="false"
clean:
	@ if [ "$(CLEAN_LIBS)" != "false" ] ; then \
		 make -s -C $(GDS_HOME) clean ; \
		 make -s -C $(JSON_HOME) clean ; \
	  fi
	@ rm -f $(EXECUTABLE) $(OFILES)
