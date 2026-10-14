#!/usr/bin/env bash
# one.sh ARGS... - one sed run three ways (GNU, the Linux build, the 68020
# build under vamos) in tests/files, for looking at a difference by hand
T=$(cd "$(dirname "$0")" && pwd)
mkdir -p "$T/w/one" && cp "$T/../../bin/sed" "$T/w/one/sed.am" && cp "$T"/../files/* "$T/w/one/" 2>/dev/null
cd "$T/w/one" || exit 2
echo "--- GNU";   LC_ALL=C sed "$@" </dev/null; echo "(rc $?)"
echo "--- Linux"; "$T/lsed" "$@" </dev/null; echo "(rc $?)"
echo "--- 68020"; vamos -q --cpu 68020 -- ./sed.am "$@" </dev/null; echo "(rc $?)"
