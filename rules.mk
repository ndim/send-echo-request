outdir = .

TARGETS =

TARGETS += $(outdir)/$(BASE).exe $(outdir)/$(BASE).stripped
TARGETS += $(outdir)/$(BASE).lss $(outdir)/$(BASE).sym

.PHONY: all
all: $(TARGETS)

$(outdir)/$(BASE).exe: $(outdir)/send-echo-request.o
	$(CC) -o $@ $(LDFLAGS) $(LIBS) $^

%.lss: %.exe
	$(OBJDUMP) -h -S $< > $@

%.sym: %.exe
	$(NM) -n $< > $@

%.stripped: %.exe
	$(STRIP) -o $@ $<

CLEANFILES += $(outdir)/send-echo-request.o

$(outdir)/send-echo-request.o : CFLAGS += -std=gnu99
$(outdir)/send-echo-request.o : CFLAGS += -Wall
$(outdir)/send-echo-request.o : CFLAGS += -Wextra
$(outdir)/send-echo-request.o : CFLAGS += -Werror
$(outdir)/send-echo-request.o : CFLAGS += -pedantic

$(outdir)/send-echo-request.o : CFLAGS += -g

# -O optimizes strlen() calls on string literals into constant numbers
$(outdir)/send-echo-request.o : CFLAGS += -Os

$(outdir)/send-echo-request.o : send-echo-request.c git-version.h usage-msg.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ -c $<

# Force recompile when we change the compile flags in GNUmakefile or rules.mk
$(outdir)/send-echo-request.o : GNUmakefile
$(outdir)/send-echo-request.o : rules.mk

