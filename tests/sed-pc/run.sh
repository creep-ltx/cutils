#!/usr/bin/env bash
# run.sh [N] - sed checked on the PC before any Amiga: sed.c built for Linux
# (tests/sed-pc/Makefile, against cu.h/shim.c here) is compared with GNU sed
# on every case in cases.py and N random s/// cases (default 400), and the
# real 68020 bin/sed under vamos with the Linux build (run.py says more).
set -e
T=$(cd "$(dirname "$0")" && pwd)
make -s -C "$T/../.." bin/sed
make -s -C "$T" all
exec python3 -I "$T/run.py" "$@"
