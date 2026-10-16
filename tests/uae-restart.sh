#!/usr/bin/env bash
# uae-restart.sh - bring the private FS-UAE (port 1240) back after a crash.
# Several sessions may call it at once: one restarts, the others wait.
cd "$(dirname "$0")"
exec 9>/tmp/cutils-uae.lock
flock 9
./w ping >/dev/null 2>&1 && { echo "up"; exit 0; }   # another session did it
CTERM_DIR=${CTERM_DIR:-../../CTerm}       # relative to tests/ (cd above)
U=$CTERM_DIR/tests/files/uae.sh
$U stop
($U start >/tmp/uaestart.log 2>&1 9>&- &)   # 9>&-: else the emulator keeps the lock for ever
sleep 5
for i in $(seq 60); do ./w ping >/dev/null 2>&1 && { echo "up"; exit 0; }; sleep 3; done
echo "uae-restart.sh: no wasabi after 3 minutes" >&2; exit 1
