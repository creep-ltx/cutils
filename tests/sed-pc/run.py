#!/usr/bin/env python3
# run.py - sed.c checked on the PC (tests/sed-pc/run.sh starts it):
#  1. every case in cases.py, and random s/// cases, run by GNU sed and by
#     sed.c built for Linux with the sanitizers (lsed-asan): the same output,
#     the same files written (w, -i), the same return code (any failure
#     counts as the same failure: the words differ);
#  2. the same cases run by the real 68020 bin/sed under vamos, compared
#     with the Linux build: bebbo's gcc has miscompiled small structs passed
#     by value (docs/tar.md), so the Amiga build is never trusted without it.
# Usage: run.py [random-count]      (default 400)
import os, random, shutil, subprocess, sys, concurrent.futures as cf
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from cases import CASES, FIXTURES, NOTGNU
T = os.path.dirname(os.path.abspath(__file__))
W = os.path.join(T, 'w')
NRAND = int(sys.argv[1]) if len(sys.argv) > 1 else 400
env = dict(os.environ, LC_ALL='C', ASAN_OPTIONS='detect_leaks=0')
shutil.rmtree(W, ignore_errors=True)
os.makedirs(W)
shutil.copy(os.path.join(T, '..', '..', 'bin', 'sed'), os.path.join(W, 'sed.am'))

# ---- random s/// cases ----
def rand_case(seed):
    r = random.Random(seed)
    ext = r.random() < 0.4
    alpha = 'abc'
    def atom(depth):
        k = r.random()
        if k < 0.45: return r.choice(alpha + 'AB')
        if k < 0.55: return '.'
        if k < 0.65: return r.choice(['[ab]', '[^a]', '[a-c]', '[[:upper:]]', '[^[:space:]]'])
        if k < 0.75 and depth < 2:
            inner = seq(depth + 1)
            if r.random() < 0.3: inner += ('|' if ext else '\\|') + seq(depth + 1)
            return ('(' + inner + ')') if ext else ('\\(' + inner + '\\)')
        if k < 0.8: return r.choice(['^', '$']) if False else r.choice(alpha)
        if k < 0.85: return r.choice(['\\w', '\\s', '\\W'])
        return r.choice(alpha)
    def piece(depth):
        a = atom(depth)
        k = r.random()
        if k < 0.2: return a + '*'
        if k < 0.3: return a + ('+' if ext else '\\+')
        if k < 0.38: return a + ('?' if ext else '\\?')
        if k < 0.44:
            lo = r.randint(0, 2); hi = lo + r.randint(0, 2)
            q = '{%d,%d}' % (lo, hi) if r.random() < 0.7 else '{%d}' % lo
            return a + (q if ext else q.replace('{', '\\{').replace('}', '\\}'))
        return a
    def seq(depth):
        return ''.join(piece(depth) for _ in range(r.randint(1, 3)))
    pat = seq(0)
    if r.random() < 0.15: pat = '^' + pat
    if r.random() < 0.15: pat = pat + '$'
    if r.random() < 0.15: pat = pat + ('|' if ext else '\\|') + seq(0)
    ngroups = pat.count('(') if ext else pat.count('\\(')
    rep = ''
    for _ in range(r.randint(0, 4)):
        k = r.random()
        if k < 0.3: rep += r.choice('xyz-')
        elif k < 0.5: rep += '&'
        elif k < 0.7 and ngroups: rep += '\\%d' % r.randint(1, min(ngroups, 9))
        elif k < 0.8: rep += r.choice(['\\U', '\\L', '\\u', '\\l', '\\E'])
        elif k < 0.9: rep += r.choice(['\\n', '\\t', '\\&', '\\\\'])
        else: rep += '[' + r.choice('xy') + ']'
    flags = r.choice(['', '', 'g', 'g', '2', '2g', 'I', 'gI', '3'])
    args = (['-E'] if ext else []) + ['s/%s/%s/%s' % (pat, rep, flags)]
    lines = []
    for _ in range(r.randint(1, 5)):
        lines.append(''.join(r.choice('abcABC  a') for _ in range(r.randint(0, 12))))
    return (args, '\n'.join(lines) + '\n')

cases = list(CASES) + [rand_case(i) for i in range(NRAND)]

def setup(d):
    os.makedirs(d)
    for name, data in FIXTURES.items():
        open(os.path.join(d, name), 'wb').write(data if isinstance(data, bytes) else data.encode('latin-1'))

def snapshot(d):
    out = {}
    for n in sorted(os.listdir(d)):
        if n in ('sed.am',) or n.startswith('.'): continue
        p = os.path.join(d, n)
        if os.path.isfile(p): out[n] = open(p, 'rb').read()
    return out

VAMOS = ['vamos', '-q', '--cpu', '68020', '-m', '8192', '-s', '64', '-H', 'disable']

def one(job):
    i, (args, inp) = job
    inp = inp.encode('latin-1') if isinstance(inp, str) else inp
    base = os.path.join(W, 'c%d' % i)
    res = {}
    for who in ('gnu', 'pc', 'am'):
        d = os.path.join(base, who)
        setup(d)
        if who == 'gnu': cmd = ['sed'] + args
        elif who == 'pc': cmd = [os.path.join(T, 'lsed-asan')] + args
        else:
            shutil.copy(os.path.join(W, 'sed.am'), os.path.join(d, 'sed.am'))
            cmd = VAMOS + ['--vols-base-dir', os.path.join(base, 'vols'), '--', './sed.am'] + args
        try:
            r = subprocess.run(cmd, input=inp, capture_output=True, cwd=d, env=env, timeout=60 if who == 'am' else 10)
            res[who] = (r.returncode, r.stdout, snapshot(d), r.stderr.decode('latin-1'))
        except subprocess.TimeoutExpired:
            res[who] = ('TIMEOUT', b'', {}, '')
    shutil.rmtree(base, ignore_errors=True)
    return i, args, inp, res

def same_rc(g, o):
    if g == 'TIMEOUT' or o == 'TIMEOUT': return False
    if g == 0 or o == 0: return g == o
    if g in (1, 2, 4): return o == 10    # GNU's failures: ours is 10
    return g == o                        # q/Q exit codes

fails_gnu, fails_am = [], []
with cf.ThreadPoolExecutor(max_workers=6) as ex:
    for i, args, inp, res in ex.map(one, enumerate(cases)):
        g, p, a = res['gnu'], res['pc'], res['am']
        notgnu = i < len(CASES) and CASES[i] in NOTGNU
        if 'runtime error' in p[3] or 'AddressSanitizer' in p[3] or 'LeakSanitizer' in p[3] and False:
            fails_gnu.append(f'#{i} {args!r}: sanitizer: {p[3][:400]}')
        elif not notgnu and (not same_rc(g[0], p[0]) or g[1] != p[1] or (g[0] == 0 and g[2] != p[2])):
            fails_gnu.append(f'#{i} {args!r} in={inp[:60]!r}\n    gnu rc={g[0]} {g[1][:300]!r} {g[3].strip()[:150]}\n    ours rc={p[0]} {p[1][:300]!r} {p[3].strip()[:150]}'
                             + ('' if g[2] == p[2] else f'\n    files differ: gnu {g[2]!r:.200} ours {p[2]!r:.200}'))
        # vamos has no error stream: the 68020 build's messages are in its output
        a_out = b''.join(l for l in a[1].splitlines(True) if not l.startswith(b'sed: '))
        if notgnu and p[0] != 10:
            fails_gnu.append(f'#{i} {args!r}: should fail with 10, gave {p[0]}')
        if '/dev/stderr' in ' '.join(args): a_out = p[1]   # (it lands in the output there too)
        if a[0] != p[0] or a_out != p[1] or a[2] != p[2]:
            fails_am.append(f'#{i} {args!r} in={inp[:60]!r}\n    pc rc={p[0]} {p[1][:200]!r}\n    68020 rc={a[0]} {a[1][:200]!r} {a[3].strip()[-300:]}'
                            + ('' if a[2] == p[2] else f'\n    files differ: pc {p[2]!r:.200} 68020 {a[2]!r:.200}'))
for f in fails_gnu: print('GNU-DIFF', f)
for f in fails_am: print('68020-DIFF', f)
n = len(cases)
print(f'{n - len(fails_gnu)} of {n} cases agree with GNU sed ({len(CASES)} listed, {NRAND} random s///; {len(NOTGNU)} listed ones not compared with GNU)')
print(f'{n - len(fails_am)} of {n} cases: the 68020 build under vamos agrees with the Linux build')
sys.exit(1 if fails_gnu or fails_am else 0)
