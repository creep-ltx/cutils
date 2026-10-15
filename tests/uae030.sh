#!/usr/bin/env bash
# uae030.sh start|stop - a second private FS-UAE for tests that need memory:
# the A1200 with a 68030/50 and 32 MB fast RAM (CTerm's A1200-030-50 config),
# the emulated bsdsocket (127.0.0.1 is the PC), AmiSSL 5 in SYS:T/amissl
# (Assign LIBS: SYS:T/amissl ADD), wasabid on port 1241 (tests/w030).
# The drive is a copy of CTerm's 030 test drive (CTerm/tests/files/uae.sh
# with CONF=030 makes /tmp/ctfiles-030), kept in ~/.cache/cutils-uae030
# (/tmp is a small tmpfs here). The emulator runs in a systemd scope of its
# own (2 GB at most); stop ends that scope only.
D=~/.cache/cutils-uae030; H="$D/AmigaOS3.2"; U=cutils-uae030
SRC=${SRC:-/tmp/ctfiles-030/AmigaOS3.2}
case "$1" in
start)
  if systemctl --user is-active --quiet $U.scope; then echo "uae030.sh: runs already"; exit 0; fi
  if [ ! -d "$H" ]; then
    [ -d "$SRC" ] || SRC="$HOME/FS-UAE/Hard Drives/AmigaOS3.2-030"
    mkdir -p "$D" && cp -a "$SRC" "$H" || exit 1
    sed -i -e 's#^  Run >NIL: C:wasabid.*$#  Run >NIL: C:wasabid 1241#' "$H/S/User-Startup"
    grep -q "wasabid 1241" "$H/S/User-Startup" || { echo "uae030.sh: User-Startup not patched" >&2; exit 1; }
    grep -q "SYS:T/amissl" "$H/S/User-Startup" || printf 'Assign LIBS: SYS:T/amissl ADD\n' >> "$H/S/User-Startup"
    [ -d "$H/T/amissl" ] || cp -a /tmp/ctfiles/AmigaOS3.2/T/amissl "$H/T/" 2>/dev/null
  fi
  sed -e "s#^hard_drive_0 = .*#hard_drive_0 = $H#" -e '/^hard_drive_1/d' \
      "$HOME/FS-UAE/Configurations/A1200-030-50.fs-uae" > "$D/uae030.fs-uae"
  echo "bsdsocket_library = 1" >> "$D/uae030.fs-uae"
  [ -n "$FAST" ] && printf "uae_cpu_speed = max\naccuracy = 0\n" >> "$D/uae030.fs-uae"
  (cd ~/FS-UAE && SDL_VIDEODRIVER=offscreen systemd-run --user --scope --unit=$U \
     -p MemoryMax=2G fs-uae "$D/uae030.fs-uae" >"$D/uae.out" 2>&1 &)
  for i in $(seq 1 60); do "$(dirname "$0")/w030" ping >/dev/null 2>&1 && break; sleep 2; done
  "$(dirname "$0")/w030" ping >/dev/null 2>&1 || { echo "uae030.sh: no wasabi on 1241" >&2; exit 1; }
  echo "up"
  ;;
stop)
  systemctl --user stop $U.scope 2>/dev/null
  while systemctl --user is-active --quiet $U.scope; do sleep 1; done
  ;;
*) echo "usage: uae030.sh start|stop" >&2; exit 2 ;;
esac
