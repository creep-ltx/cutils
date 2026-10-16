#!/usr/bin/env bash
# shot.sh NAME WAIT CMD... - type each CMD in a CTerm tab on rig.sh's FS-UAE (each shown
# after an echoed prompt; a CMD starting with ! runs unshown), wait WAIT seconds, grab the
# screen, cut the empty rows off into docs/images/NAME.png, then end CTerm.
# PR= the prompt to show (1.AmigaOS3.2:T/demo). KEEP=1: leave CTerm running.
export WASABI_HOST=127.0.0.1 WASABI_PORT=1255
CU=$(cd "$(dirname "$0")/../.." && pwd)
H=$(amiga-test a1200-32-mui5 dir --as cutils-docs-030 --port 1255 --label AmigaOS3.2) || exit 1
PR=${PR:-1.AmigaOS3.2:T/demo}; name=$1 wait=$2; shift 2
{ printf 'Path SYS:T/CU ADD\nPrompt "%%N.%%S> "\nCD SYS:T/demo\n'
  for c in "$@"; do
    if [ "${c#!}" != "$c" ]; then printf '%s\n' "${c#!}"
    else e=${c//\"/*\"}; printf 'Echo "%s> %s"\n%s\n' "$PR" "$e" "$c"; fi
  done; } > "$H/S/xshot"
wasabi run "Copy ENVARC:CTerm.prefs ENV:" >/dev/null
wasabi run --max-time 5 "Run >NIL: SYS:T/CT/CTerm FROM S:xshot" >/dev/null
sleep $((3 + wait))
if wasabi windows | grep -q -i 'failure'; then echo "$name: a Software Failure requester is up" >&2; exit 1; fi
out=$(wasabi grab) || exit 1
python3 "$(dirname "$0")/crop.py" "$out" "$CU/docs/images/$name.png" && echo "docs/images/$name.png"
[ -n "$KEEP" ] && exit 0
for a in $(wasabi ps 2>/dev/null | awk '/CTerm/ {print $1}'); do wasabi kill "$a" >/dev/null 2>&1; done
sleep 3
