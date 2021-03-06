# Usage: make [var=value]...
# Compile host binaries and (if crossprefix set) cross-compile as well.
#
# Some variables with example values:
#   crossprefix=i486-openwrt-linux-uclibc-
#   crossbindir=/bin
#   crosshost=wrt
#   bindir=/usr/sbin
#   DESTDIR=$PWD/_installroot
#
# Some interesting targets:
#   all (default)
#   clean
#   install
#   cross-install

prefix     = /usr
execprefix = $(prefix)
bindir     = $(execprefix)/bin

INSTALL = install

crossbindir = /bin
crosshost = need-to-set-crosshost-first
crossprefix =

crossCC      = $(crossprefix)gcc
crossNM      = $(crossprefix)nm
crossOBJDUMP = $(crossprefix)objdump
crossSTRIP   = $(crossprefix)strip

CC      = gcc
NM      = nm
OBJDUMP = objdump
STRIP   = strip

CLEANFILES =

DUMMY1 := $(shell ./update-git-version.sh)
CLEANFILES += git-version.h

DUMMY2 := $(shell ./update-usage-msg.sh)
CLEANFILES += usage-msg.h

BASE = send-echo-request
export BASE

crossbuild := cross-$(notdir $(crossprefix))build
hostbuild  := host-build

.PHONY: all
all:
	mkdir -p $(hostbuild)
	$(MAKE) -f rules.mk outdir=$(hostbuild)  CC=$(CC)      NM=$(NM)      OBJDUMP=$(OBJDUMP)      STRIP=$(STRIP)
ifneq ($(crossprefix),)
	mkdir -p $(crossbuild)
	$(MAKE) -f rules.mk outdir=$(crossbuild) CC=$(crossCC) NM=$(crossNM) OBJDUMP=$(crossOBJDUMP) STRIP=$(crossSTRIP)
endif

.PHONY: clean
clean:
	rm -f $(CLEANFILES)
	rm -rf $(crossbuild) $(hostbuild)

.PHONY: install
install: all
	$(INSTALL) -c  -m 0755 -d                       $(DESTDIR)$(bindir)
	$(INSTALL) -cp -m 0755 $(hostbuild)/$(BASE).exe $(DESTDIR)$(bindir)/$(BASE)

.PHONY: uninstall
uninstall:
	rm -f $(DESTDIR)$(bindir)/$(BASE)

# Remove the target file first to work around OpenWRT "text file busy" error.
.PHONY: cross-install
cross-install: all
	ssh $(crosshost) rm -f $(crossbindir)/$(BASE)
	scp $(crossbuild)/$(BASE).stripped $(crosshost):$(crossbindir)/$(BASE)

.PHONY: cross-uninstall
cross-uninstall:
	ssh $(crosshost) rm -f $(crossbindir)/$(BASE)
