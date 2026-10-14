# tests/sed-pc - sed.c built for Linux (lsed, and lsed-asan with the
# sanitizers) against the stand-in cu.h and shim.c, compared with GNU sed
# by run.sh; the 68020 side is the real bin/sed under vamos.
TOP = ../..
SRC = $(TOP)/src/sed.c shim.c regex_pc.c

all: lsed lsed-asan

lsed: $(SRC) cu.h $(TOP)/lib/regex.c
	gcc -O2 -w -I. -c -Dmain=sed_main -o sed-pc.o $(TOP)/src/sed.c
	gcc -O2 -w -I. -o $@ sed-pc.o regex_pc.c shim.c

lsed-asan: $(SRC) cu.h $(TOP)/lib/regex.c
	gcc -O1 -g -w -fsanitize=address,undefined -fno-sanitize-recover=undefined -I. -c -Dmain=sed_main -o sed-asan.o $(TOP)/src/sed.c
	gcc -O1 -g -w -fsanitize=address,undefined -fno-sanitize-recover=undefined -I. -o $@ sed-asan.o regex_pc.c shim.c

clean:
	rm -f lsed lsed-asan sed-asan.o sed-pc.o

.PHONY: all clean
