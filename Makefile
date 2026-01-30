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
		  $(INCLUDE_DIRS) $(FLAGS) -std=c23

LDFLAGS=-L$(GDS_HOME)/bin -lGDS-static -L$(JSON_HOME)/bin -ljson-static

ifdef PLUGIN_SUPPORT
	CFLAGS += -fPIC
endif

PROFILE := debug

ifeq ($(PROFILE),release)
	CFLAGS += -O3
endif

LIBFILES= ./src/lib/str/wstr.c
CFILES=  $(shell find src -name '*.c' -not -path "src/main.c" -not -path "src/lib/*" -not -path "src/platform/*") \
		 $(LIBFILES) \
		 $(shell find src/platform/$(TARGET_PLATFORM) -name '*.c')

ifndef PLUGIN_SUPPORT
	CFILES += src/main.c
endif

OFILES= $(patsubst %.c,%.o,$(CFILES))

edit: $(OFILES)
	@ make -C $(GDS_HOME)
	@ make -C $(JSON_HOME)
	@ echo " LD => edit"
ifdef PLUGIN_SUPPORT
	@ $(CC) -shared -o lib$(EXECUTABLE).so $(OFILES) $(CFLAGS) $(LDFLAGS)
	@ $(CC) -Wl,-rpath='$(PREFIX)/lib' -ledit -L./ src/main.c -o $(EXECUTABLE) $(CFLAGS) $(LDFLAGS)
else
	@ $(CC) -o $(EXECUTABLE) $(OFILES) $(CFLAGS) $(LDFLAGS)
endif

release:
	@ make -s clean CLEAN_LIBS=true
	@ make -s PROFILE=release

.c.o:
	@ echo " CC $@"
	@ $(CC) -c $< -o $@ $(CFLAGS)

PREFIX ?= /usr/local

install: edit
	@ echo "edit => $(PREFIX)/bin"
	@ echo "edit.1 => $(PREFIX)/share/man/man1"
	@ install -d $(PREFIX)/bin
	@ install -m  755 ./edit $(PREFIX)/bin
ifdef PLUGIN_SUPPORT
	@ install -m  755 ./libedit.so $(PREFIX)/lib
endif
	@ install -d $(PREFIX)/share/man/man1
	@ install -m  644 edit.1 $(PREFIX)/share/man/man1

uninstall:
	@ echo " RM $(PREFIX)/bin/edit"
	@ echo " RM $(PREFIX)/share/man/man1/edit.1"
	@ rm -f $(PREFIX)/bin/edit \
			$(PREFIX)/lib/libedit.so \
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
	@ rm -f $(EXECUTABLE) $(OFILES) lib$(EXECUTABLE).so
