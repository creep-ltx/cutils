# CUtils - Unix-style commands for AmigaOS 3.2, in C. Cross-built with bebbo's
# gcc (~/opt/amiga), -fbbb=- as the whole terminal family
# (Knowledge/amiga/toolchain-and-testing.md). -resident32: pure, so each can
# be made Resident (the p bit is set by make).
# make test: tests/run.sh in the private FS-UAE (CTerm/tests/files/uae.sh).

AMIGA   = $(HOME)/opt/amiga
CC      = $(AMIGA)/bin/m68k-amigaos-gcc
CFLAGS  = -Os -m68020 -noixemul -resident32 -fomit-frame-pointer -fno-common -fbbb=- -Wall -Wno-pointer-sign -Ilib
LDFLAGS = -m68020 -noixemul -resident32 -s
CMDS    = cat head wc
LIB     = build/libcu.a
LIBOBJ  = build/cu.o build/cu_each.o build/cu_in.o build/regex.o

all: $(addprefix bin/,$(CMDS))

build/%.o: lib/%.c lib/cu.h
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

build/cmd_%.o: src/%.c lib/cu.h
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

# an archive, so each command links only the parts it uses
# (link with -m68020: without it gcc takes the 68000 libnix, not the a4 one
# -resident32 needs, and the program crashes at start)
$(LIB): $(LIBOBJ)
	rm -f $@
	$(AMIGA)/bin/m68k-amigaos-ar rcs $@ $^

bin/%: build/cmd_%.o $(LIB)
	@mkdir -p bin
	$(CC) $(LDFLAGS) $< $(LIB) -o $@
	chmod +x $@

test: all
	tests/run.sh

clean:
	rm -rf build bin

.PHONY: all test clean

.SECONDARY:
