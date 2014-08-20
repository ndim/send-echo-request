prefix = /usr
execprefix = $(prefix)
bindir = $(execprefix)/bin

INSTALL = install
NM = nm
OBJDUMP = objdump

CLEANFILES =

DUMMY1 := $(shell ./update-git-version.sh)
CLEANFILES += git-version.h

DUMMY2 := $(shell ./update-usage-msg.sh)
CLEANFILES += usage-msg.h

EXE = send-echo-request

.PHONY: all
all: $(EXE) $(EXE).lss $(EXE).sym

CLEANFILES += $(EXE)
$(EXE) : send-echo-request.o

$(EXE).lss: $(EXE)
	$(OBJDUMP) -h -S $< > $@

$(EXE).sym: $(EXE)
	$(NM) -n $(EXE) > $@

CLEANFILES += send-echo-request.o

send-echo-request.o : CFLAGS += -std=gnu99
send-echo-request.o : CFLAGS += -Wall
send-echo-request.o : CFLAGS += -Wextra
send-echo-request.o : CFLAGS += -Werror
send-echo-request.o : CFLAGS += -pedantic

send-echo-request.o : CFLAGS += -g

# -O optimizes strlen() calls on string literals into constant numbers
send-echo-request.o : CFLAGS += -Os

send-echo-request.o : send-echo-request.c git-version.h usage-msg.h

# Force recompile when we change the compile flags in GNUmakefile
send-echo-request.o : GNUmakefile

.PHONY: clean
clean:
	rm -f $(CLEANFILES)

.PHONY: install
install: all
	$(INSTALL) -c  -m 0755 -d     $(DESTDIR)$(bindir)
	$(INSTALL) -cp -m 0755 $(EXE) $(DESTDIR)$(bindir)/send-echo-request

.PHONY: uninstall
uninstall:
	rm -f $(DESTDIR)$(bindir)/send-echo-request
