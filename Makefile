#GNUDIR        = /usr/lib/x86_64-linux-gnu
#DBUSDIRS      = dbus-1.0 glib-2.0
#IDIRS         = $(addprefix /usr/include/,$(DBUSDIRS)) $(addprefix $(GNUDIR)/,$(addsuffix /include,$(DBUSDIRS)))
IDIRS         = /usr/include/dbus-1.0 /usr/lib/x86_64-linux-gnu/dbus-1.0/include
LIBS          = dbus-1 pthread # dbus-glib-1 glib-2.0 gobject-2.0 rt

AR            = ar
CC            = clang
CFLAGS        = -D NDEBUG $(addprefix -I,$(IDIRS)) -O3 -Wall -Werror -fPIC -march=native
LD            = clang
LDFLAGS       = $(addprefix -l,$(LIBS)) -Wl,-s
RM            = rm -fr
TAR           = tar
TARFLAGS      = --group root --owner root -ch

CONTROLFILES = control md5sums
DEBPKG = dbus-redirector_1.0_amd64.deb
TARGET_BINARY = usr/bin/dbus-redirector
TARGET_SOURCES = core.c entries.c id.c log.c main.c mem.c pipe.c service.c thread.c
TARGET_OBJECTS = $(TARGET_SOURCES:%.c=%.o)

all: $(DEBPKG)

clean:
	$(RM) $(DEBPKG) $(TARGET_BINARY) $(TARGET_OBJECTS)

.PHONY: clean disasm nm

.c.o:
	$(CC) $(CFLAGS) -c $<

control.tar.xz: $(CONTROLFILES)
	$(TAR) $(TARFLAGS) -f - $(CONTROLFILES) | xz -cez9 - > $@ && $(RM) md5sums

data.tar.xz: $(TARGET_BINARY)
	$(TAR) $(TARFLAGS) -f - $(shell for name in `ls`; do test -d $$name && echo $$name; done | xargs) | xz -cez9 - > $@

debian-binary:
	@echo 2.0 > $@

disasm: $(TARGET_BINARY)
	@llvm-objdump --disassemble-all $(TARGET_BINARY) | less

md5sums: $(TARGET_BINARY)
	find . -type f | grep -v -e '/\.' | while read name; do echo -n $$name | cut -d'/' -f2- | grep '/' > /dev/null && echo $$name; done | xargs md5sum > $@

nm: $(TARGET_BINARY)
	@nm -gP --all $^ | less

$(DEBPKG): control.tar.xz data.tar.xz debian-binary
	$(AR) cr $@ $^ && $(RM) $^

$(TARGET_BINARY): $(TARGET_OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $^ && $(RM) $^
