.PHONY: clean install uninstall
CC=cc
CFLAGS+= -Wall -Wextra -pedantic -O3 -g
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
	@ echo " => $(INSTALL_PATH)/bin/edit"
	@ install -d $(INSTALL_PATH)/bin
	@ install -m  755 ./edit $(INSTALL_PATH)/bin

uninstall:
	@ echo " RM $(INSTALL_PATH)/bin/edit"
	@ rm -f $(INSTALL_PATH)/bin/edit

clean:
	@ rm -f edit $(OFILES) $(DEPS) $(DEPS)
