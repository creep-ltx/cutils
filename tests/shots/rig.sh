#!/usr/bin/env bash
# rig.sh start|stop|put|reset - the FS-UAE for the README's screenshots: CTerm's rig
# (CTerm repo, tests/rig.sh; CTERM_DIR, default the sibling ../CTerm) as instance cutils-docs on port 1255, a 68030 with
# AmiSSL; this tree's bin/ in SYS:T/CU (on the path in CTerm's tabs) and its pages in
# SYS:T/Man (MAN:); CTerm on its own 256-colour full screen with the Nord theme; the
# demo files of demo.sh in SYS:T/demo.
RIG=cutils-docs PORT=1255 SSL=1 CONF=030
CTERM_DIR=$(cd "${CTERM_DIR:-$(dirname "$0")/../../../CTerm}" && pwd) || exit 1
. "$CTERM_DIR/tests/rig.sh"
D=$CTERM_DIR                             # rig.sh's put copies CTerm from here
CU=$(cd "$(dirname "$0")/../.." && pwd)
if [ "$1" = start ] || [ "$1" = put ]; then
  H=$(amiga-test $M dir "${A[@]}") || exit 1
  mkdir -p "$H/T/CU" "$H/T/Man/english"
  cp "$CU"/bin/* "$H/T/CU/"
  cp -r "$CU/man/english/CUtils" "$H/T/Man/english/"
  grep -q 'SYS:T/CU' "$H/S/User-Startup" || printf 'Path SYS:T/CU ADD\nAssign MAN: SYS:T/Man\n' >> "$H/S/User-Startup"
  # mode= is the hex display id; before CTerm 0.3b51 a failed own screen was saved as wb,
  # so this is written again every time
  P="$H/Prefs/Env-Archive/CTerm.prefs"
  [ -f "$P" ] || printf 'scrollbar=0\nutf8=1\n' > "$P"
  sed -i -e '/^screen=/d' -e '/^full=/d' -e '/^theme=/d' -e '/^mode=/d' "$P"
  printf 'screen=own8\nfull=1\ntheme=Nord\n' >> "$P"
  printf 'CET-1CEST,M3.5.0,M10.5.0/3' > "$H/Prefs/Env-Archive/TZ"   # tar's times as on the user's A1200
  bash "$(dirname "$0")/demo.sh" "$H/T/demo"
fi
rig "$@"
