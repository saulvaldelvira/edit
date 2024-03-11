.PHONY: clean install uninstall
CC=cc
CFLAGS+= -Wall -Wextra -pedantic -O3 -g -Wstrict-prototypes
CFILES= $(wildcard ./src/*.c) $(wildcard ./src/highlight/*.c) \
		./src/lib/GDS/src/Vector.c ./src/lib/GDS/src/util/compare.c ./src/lib/str/wstr.c
OFILES= $(patsubst %.c,%.o,$(CFILES))
DEPS= $(patsubst %.c,%.d,$(CFILES))

edit: $(OFILES)
	@ $(CC) -o edit $(OFILES) $(CFLAGS)

-include $(DEPS)
.c.o:
	@ echo " CC $@"
	@ $(CC) -c $< -o $@ -MMD -MP $(CFLAGS)

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

clean:
	@ rm -f edit $(OFILES) $(DEPS) $(DEPS)
