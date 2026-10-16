# tui.library - the toolkit for programs inside the terminal (README.md).
# Cross-built with Bebbo's m68k-amigaos-gcc, -fbbb=- as the whole
# terminal family (Knowledge/amiga/toolchain-and-testing.md).
#
#   make            the library three times (below), libtui.a (the toolkit
#                   built into a program, TUI_STATIC), libtuiglue.a (the old
#                   tui_* names), tuitest and tuibench
#   make headers    clib/ proto/ inline/ again from tui.sfd (after adding a call)
#
# The library, one per processor; install one as LIBS:tui.library:
#   build/000/tui.library   every Amiga (68000 and up)
#   build/020/tui.library   68020, 68030, 68040 (and Emu68)
#   build/060/tui.library   68060: the 020 code without the 64-bit multiply
#                           and divide the 060 has only in software
# A 68030 runs the 020 code as it is: gcc makes the same code for both.
#
# The library links nothing but its own code (-nostartfiles -nostdlib); the
# 68000 one also takes libnix 1.3's plain multiply and divide (no
# utility.library). The build fails if any other C library code gets in.

# Bebbo's m68k-amigaos-gcc: set AMIGA to where it is installed, otherwise it is
# looked for in the usual places (/opt/amiga, its own default, then ~/opt/amiga).
AMIGA   ?= $(firstword $(wildcard /opt/amiga $(HOME)/opt/amiga))
CC      = $(AMIGA)/bin/m68k-amigaos-gcc
OBJDUMP = $(AMIGA)/bin/m68k-amigaos-objdump
SFDC    = $(AMIGA)/bin/sfdc
WARN    = -Os -noixemul -fomit-frame-pointer -fno-common -fbbb=- -Wall -Wno-pointer-sign -Iinclude -Isrc
CFLAGS  = $(WARN) -m68020
LIBFLAGS = -fno-builtin -ffreestanding -DTUI_LIB
NIX13   = $(AMIGA)/m68k-amigaos/libnix/lib/libnix13.a
CPUS    = 000 020 060
ARITH_000 = $(NIX13)

all: $(foreach c,$(CPUS),build/$(c)/tui.library) build/libtui.a build/libtuiglue.a \
     build/libtuiglue_static.a build/tuitest build/tuitest_static build/tuibench cutilscheck

build/start.o: src/start.s
	@mkdir -p build
	$(CC) -c $< -o $@

# one library per processor: build/CPU/tui.library
define LIBRARY
build/$(1)/lib.o: src/lib.c src/version.h include/libraries/tui.h
	@mkdir -p build/$(1)
	$$(CC) $$(WARN) -m68$(1) $$(LIBFLAGS) -c $$< -o $$@

build/$(1)/core.o: src/core.c include/libraries/tui.h
	@mkdir -p build/$(1)
	$$(CC) $$(WARN) -m68$(1) $$(LIBFLAGS) -c $$< -o $$@

build/$(1)/tui.library: build/start.o build/$(1)/lib.o build/$(1)/core.o
	$$(CC) -m68$(1) -noixemul -nostartfiles -nostdlib -s -Wl,-Map,build/$(1)/tui.map -o $$@ $$^ $$(ARITH_$(1)) -lgcc
	@! grep -E 'libnix|libstubs|libamiga' build/$(1)/tui.map | grep -v -E '^LOAD |libnix13.a\(__u?(mul|div|mod)si[34].o\)' \
	    || (echo "C library code got into $$@"; false)
endef
$(foreach c,$(CPUS),$(eval $(call LIBRARY,$(c))))

# the 060 one must not have the 64-bit forms of muls.l/mulu.l/divs.l/divu.l
# (three operands in objdump's words, ea,Dh,Dl; divsll is the 32-bit one)
check060: build/060/tui.library
	@! $(OBJDUMP) -m m68k:68060 -d build/060/core.o build/060/lib.o | grep -E '\s(mul|div)[su]l\s+[^ ]*,%?d[0-7],%?d[0-7]' \
	    || (echo "build/060 has a 64-bit multiply or divide"; false)
	@echo "build/060: no 64-bit multiply or divide"

# CUtils' man, top and ctop build core.c and glue.c into themselves with its
# own flags (pure, resident, TUI_STATIC; the user's rule: no LIBS: file for
# CUtils). This builds them that way too, so a change that would break CUtils
# fails here; after changing core.c also run
# make -C ../.. bin/man bin/top bin/ctop
cutilscheck: src/core.c src/glue.c include/tui.h include/libraries/tui.h
	@mkdir -p build/cutils
	$(CC) $(CFLAGS) -resident32 -DTUI_STATIC -c src/core.c -o build/cutils/core.o
	$(CC) $(CFLAGS) -resident32 -DTUI_STATIC -c src/glue.c -o build/cutils/glue.o

# the same core, for a program that wants it inside (no library needed)
build/core.o: src/core.c include/libraries/tui.h
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

build/libtui.a: build/core.o
	rm -f $@ && $(AMIGA)/bin/m68k-amigaos-ar rcs $@ $^

# the old tui_* names (include/tui.h) over the library, or over libtui.a
build/glue.o: src/glue.c include/tui.h include/libraries/tui.h
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

build/glue_static.o: src/glue.c include/tui.h include/libraries/tui.h
	@mkdir -p build
	$(CC) $(CFLAGS) -DTUI_STATIC -c $< -o $@

build/libtuiglue.a: build/glue.o
	rm -f $@ && $(AMIGA)/bin/m68k-amigaos-ar rcs $@ $^

build/libtuiglue_static.a: build/glue_static.o
	rm -f $@ && $(AMIGA)/bin/m68k-amigaos-ar rcs $@ $^

build/tuitest: tests/tuitest.c build/libtuiglue.a
	$(CC) $(CFLAGS) -s -o $@ $< -Lbuild -ltuiglue

build/tuitest_static: tests/tuitest.c build/libtuiglue_static.a build/libtui.a
	$(CC) $(CFLAGS) -DTUI_STATIC -s -o $@ $< -Lbuild -ltuiglue_static -ltui

# how fast the library draws (tests/tuibench.c); a 68000 build of it too
build/tuibench: tests/tuibench.c build/libtuiglue.a
	$(CC) $(CFLAGS) -s -o $@ $< -Lbuild -ltuiglue

headers: tui.sfd
	$(SFDC) --mode=clib --output=include/clib/tui_protos.h tui.sfd
	$(SFDC) --mode=proto --output=include/proto/tui.h tui.sfd
	$(SFDC) --target=m68k-amigaos --mode=macros --output=include/inline/tui.h tui.sfd

clean:
	rm -rf build

.PHONY: all headers clean check060 cutilscheck
