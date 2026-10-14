#!/usr/bin/env bash
# run.sh [NAME...] - each tests/cases/NAME.t run in FS-UAE (or W=... another
# wasabi wrapper) and compared with its expected output.
# A case file: blocks of
#     $ command line           (run by the ROM Shell, in RAM:cut/ as cd)
#     expected output lines
#     ? rc                     (optional: the return code it must give)
# blank lines inside the expected output are kept; a block ends at the next $.
# The commands are in RAM:cu/ (on the path for the run), tests/files/* in RAM:cut/.
# S=x: RAM:cux and RAM:cutx instead, so several sessions can test at once.
# The emulator dead (a crash): tests/uae-restart.sh.
cd "$(dirname "$0")"; W=${W:-./w}; S=${S:-}; export S
$W ping >/dev/null || { echo "run.sh: no wasabi (start CTerm/tests/files/uae.sh)"; exit 2; }
$W run "MakeDir >NIL: RAM:cu$S RAM:cut$S" >/dev/null 2>&1
for f in ../bin/*; do $W put "$f" "RAM:cu$S/$(basename "$f")" >/dev/null || exit 2; done
$W run "Protect RAM:cu$S/#? +rep" >/dev/null
for f in files/*; do $W put "$f" "RAM:cut$S/$(basename "$f")" >/dev/null; done
names=("$@"); [ ${#names[@]} = 0 ] && names=($(cd cases && ls *.t | sed 's/\.t$//'))
bad=0; n=0
for name in "${names[@]}"; do
  python3 - "$W" "cases/$name.t" <<'PY' || bad=1
import subprocess, sys, os
S = os.environ.get('S', '')
w, path = sys.argv[1], sys.argv[2]
blocks, cur = [], None
for line in open(path).read().split('\n'):
    if line.startswith('$ '):
        cur = {'cmd': line[2:], 'out': [], 'rc': None}; blocks.append(cur)
    elif cur is not None and line.startswith('? ') and line[2:].strip().isdigit():
        cur['rc'] = int(line[2:])
    elif cur is not None:
        cur['out'].append(line)
fail = 0
for b in blocks:
    while b['out'] and b['out'][-1] == '': b['out'].pop()
    want = '\n'.join(b['out'])
    script = f'FailAt 21\nCD RAM:cut{S}\nPath RAM:cu{S} ADD\n' + b['cmd'] + f'\nSetEnv curc{S} $RC\n'
    open(f'/tmp/cutest{S}.script', 'w').write(script)
    subprocess.run([w, 'put', f'/tmp/cutest{S}.script', f'RAM:cutest{S}.script'], capture_output=True)
    r = subprocess.run([w, 'run', '--max-time', '60', f'Execute RAM:cutest{S}.script'], capture_output=True, text=True, errors='replace')
    got = (r.stdout + r.stderr).rstrip('\n')
    rc = subprocess.run([w, 'run', f'GetEnv curc{S}'], capture_output=True, text=True).stdout.strip()
    ok = got == want and (b['rc'] is None or str(b['rc']) == rc)
    if not ok:
        fail = 1
        print(f"FAIL {path}: $ {b['cmd']}  (rc {rc}, want {b['rc']})")
        import difflib
        for l in difflib.unified_diff(want.split('\n'), got.split('\n'), 'want', 'got', lineterm='', n=1):
            print('    ' + l)
print(('ok   ' if not fail else 'BAD  ') + path + f" ({len(blocks)} cases)")
sys.exit(fail)
PY
done
exit $bad
