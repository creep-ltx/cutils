# tui.library - the toolkit for programs inside the terminal (README.md).
# Cross-built with bebbo's gcc (~/opt/amiga), -fbbb=- as the whole
# terminal family (Knowledge/amiga/toolchain-and-testing.md).
#
#   make            tui.library, libtui.a (the toolkit built into a program,
#                   TUI_STATIC), libtuiglue.a (the old tui_* names) and tuitest
#   make headers    clib/ proto/ inline/ again from tui.sfd (after adding a call)
#
# The library links nothing but its own code (-nostartfiles -nostdlib):
# check build/tui.map for libnix or libstubs - there must be none.

AMIGA   = $(HOME)/opt/amiga
CC      = $(AMIGA)/bin/m68k-amigaos-gcc
SFDC    = $(AMIGA)/bin/sfdc
CFLAGS  = -Os -m68020 -noixemul -fomit-frame-pointer -fno-common -fbbb=- -Wall -Wno-pointer-sign -Iinclude -Isrc
LIBFLAGS = $(CFLAGS) -fno-builtin -ffreestanding -DTUI_LIB

all: build/tui.library build/libtui.a build/libtuiglue.a build/libtuiglue_static.a build/tuitest build/tuitest_static

build/tui.library: build/start.o build/lib.o build/core_lib.o
	$(CC) -m68020 -noixemul -nostartfiles -nostdlib -s -Wl,-Map,build/tui.map -o $@ $^ -lgcc
	@! grep -E 'libnix|libstubs|libamiga' build/tui.map || (echo "C library code got into tui.library"; false)

build/start.o: src/start.s
	@mkdir -p build
	$(CC) -c $< -o $@

build/lib.o: src/lib.c src/version.h include/libraries/tui.h
	@mkdir -p build
	$(CC) $(LIBFLAGS) -c $< -o $@

build/core_lib.o: src/core.c include/libraries/tui.h
	@mkdir -p build
	$(CC) $(LIBFLAGS) -c $< -o $@

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

headers: tui.sfd
	$(SFDC) --mode=clib --output=include/clib/tui_protos.h tui.sfd
	$(SFDC) --mode=proto --output=include/proto/tui.h tui.sfd
	$(SFDC) --target=m68k-amigaos --mode=macros --output=include/inline/tui.h tui.sfd

clean:
	rm -rf build

.PHONY: all headers clean
