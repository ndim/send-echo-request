prefix     = /usr
execprefix = $(prefix)
bindir     = $(execprefix)/bin

INSTALL = install

hostprefix = 

CC      = $(hostprefix)gcc
NM      = $(hostprefix)nm
OBJDUMP = $(hostprefix)objdump
STRIP   = $(hostprefix)strip

CLEANFILES =

DUMMY1 := $(shell ./update-git-version.sh)
CLEANFILES += git-version.h

DUMMY2 := $(shell ./update-usage-msg.sh)
CLEANFILES += usage-msg.h

BASE = send-echo-request

.PHONY: all
all: $(BASE).exe $(BASE).stripped $(BASE).lss $(BASE).sym

CLEANFILES += $(BASE).exe
$(BASE).exe: send-echo-request.o
	$(CC) $(LDFLAGS) $(LIBS) $^ -o $@

CLEANFILES += $(BASE).lss
$(BASE).lss: $(BASE).exe
	$(OBJDUMP) -h -S $< > $@

CLEANFILES += $(BASE).sym
$(BASE).sym: $(BASE).exe
	$(NM) -n $< > $@

CLEANFILES += $(BASE).stripped
$(BASE).stripped: $(BASE).exe
	$(STRIP) -o $@ $<

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
	$(INSTALL) -c  -m 0755 -d          $(DESTDIR)$(bindir)
	$(INSTALL) -cp -m 0755 $(BASE).exe $(DESTDIR)$(bindir)/$(BASE)

.PHONY: uninstall
uninstall:
	rm -f $(DESTDIR)$(bindir)/$(BASE)
