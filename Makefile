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
