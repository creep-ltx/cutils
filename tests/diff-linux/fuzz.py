#!/usr/bin/env python3
# fuzz.py N - random file pairs: our diff (Linux build) against GNU diff:
# the patch must apply, the edit count must be minimal, and how often the
# output is the very same as GNU's. Build the Linux diff first (cu.h and
# shim.c here stand in for CUtils' lib and the dos calls), from CUtils/:
#   gcc -O2 -w -Itests/diff-linux -o tests/diff-linux/ldiff src/diff.c tests/diff-linux/shim.c
# fuzz2.py: the same for -i -w -b -B. Needs GNU diff and patch.
import random, subprocess, sys, os
D = os.path.dirname(os.path.abspath(__file__))
L = os.path.join(D, 'ldiff')
n = int(sys.argv[1]) if len(sys.argv) > 1 else 300
same = {'n': 0, 'u': 0}
bad = 0
def count(out):
    return sum(1 for l in out.split('\n') if (l[:1] in '+-<>') and not l.startswith(('---', '+++')))
for t in range(n):
    rnd = random.Random(t)
    alpha = [f'line {c}' for c in 'abcdefg'[:rnd.randint(2, 7)]] + ['', '}', '{']
    a = [rnd.choice(alpha) for _ in range(rnd.randint(0, 40))]
    b = list(a)
    for _ in range(rnd.randint(0, 8)):
        op = rnd.randint(0, 2)
        if op == 0 and b: del b[rnd.randrange(len(b))]
        elif op == 1: b.insert(rnd.randint(0, len(b)), rnd.choice(alpha))
        elif b: b[rnd.randrange(len(b))] = rnd.choice(alpha)
    ta = '\n'.join(a) + ('\n' if a and rnd.random() < .8 else '')
    tb = '\n'.join(b) + ('\n' if b and rnd.random() < .8 else '')
    open('/tmp/fz_a', 'w').write(ta); open('/tmp/fz_b', 'w').write(tb)
    for fmt in ('n', 'u'):
        args = ['-u'] if fmt == 'u' else []
        ours = subprocess.run([L] + args + ['/tmp/fz_a', '/tmp/fz_b'], capture_output=True, text=True)
        gnu = subprocess.run(['diff'] + args + ['/tmp/fz_a', '/tmp/fz_b'], capture_output=True, text=True)
        mini = subprocess.run(['diff', '--minimal'] + args + ['/tmp/fz_a', '/tmp/fz_b'], capture_output=True, text=True)
        rc_ok = (ours.returncode == 0) == (gnu.returncode == 0)
        strip = lambda s: '\n'.join(l for l in s.split('\n') if not l.startswith(('---', '+++')) or fmt == 'n')
        if strip(ours.stdout) == strip(gnu.stdout): same[fmt] += 1
        okcount = count(ours.stdout) == count(mini.stdout)
        okpatch = True
        if fmt == 'u' and ours.stdout:
            open('/tmp/fz_p', 'w').write(ours.stdout)
            subprocess.run(['cp', '/tmp/fz_a', '/tmp/fz_c'])
            r = subprocess.run(['patch', '-s', '/tmp/fz_c', '/tmp/fz_p'], capture_output=True, text=True)
            okpatch = r.returncode == 0 and open('/tmp/fz_c').read() == tb
        if fmt == 'n' and ours.stdout:
            open('/tmp/fz_p', 'w').write(ours.stdout)
            subprocess.run(['cp', '/tmp/fz_a', '/tmp/fz_c'])
            r = subprocess.run(['patch', '-s', '--normal', '/tmp/fz_c', '/tmp/fz_p'], capture_output=True, text=True)
            okpatch = r.returncode == 0 and open('/tmp/fz_c').read() == tb
        if not (rc_ok and okcount and okpatch):
            bad += 1
            if bad < 5:
                print(f'case {t} {fmt}: rc_ok={rc_ok} count={okcount} patch={okpatch}')
                print('A:', a, repr(ta[-1:])); print('B:', b, repr(tb[-1:]))
                print(ours.stdout); print('GNU:'); print(gnu.stdout)
print(f'{n} pairs, {bad} bad; same as GNU: normal {same["n"]}, unified {same["u"]}')
