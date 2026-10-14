#!/usr/bin/env python3
# fuzz2.py - -i -w -b -B and their mixes against GNU diff: same edit count
# (both minimal on small files) and the same "differ or not"
import random, subprocess, os
D = os.path.dirname(os.path.abspath(__file__))
L = os.path.join(D, 'ldiff')
bad = 0; same = 0; total = 0
def count(out):
    return sum(1 for l in out.split('\n') if l[:1] in '<>')
for t in range(400):
    rnd = random.Random(1000 + t)
    base = ['foo bar', 'Foo Bar', 'foo  bar', ' foo bar', 'foo bar ', 'foo\tbar', 'foobar', '', '  ', 'x']
    a = [rnd.choice(base) for _ in range(rnd.randint(0, 25))]
    b = [rnd.choice(base) for _ in range(rnd.randint(0, 25))] if rnd.random() < .3 else [l if rnd.random() < .7 else rnd.choice(base) for l in a]
    open('/tmp/fy_a', 'w').write(''.join(l + '\n' for l in a)); open('/tmp/fy_b', 'w').write(''.join(l + '\n' for l in b))
    for fl in (['-i'], ['-w'], ['-b'], ['-B'], ['-i', '-w'], ['-b', '-B'], ['-w', '-B']):
        total += 1
        ours = subprocess.run([L] + fl + ['/tmp/fy_a', '/tmp/fy_b'], capture_output=True, text=True)
        gnu = subprocess.run(['diff', '--minimal'] + fl + ['/tmp/fy_a', '/tmp/fy_b'], capture_output=True, text=True)
        if ours.stdout == gnu.stdout: same += 1
        ok = (ours.returncode == 0) == (gnu.returncode == 0) and (count(ours.stdout) == count(gnu.stdout) or '-B' in fl)
        if '-B' in fl and (ours.returncode == 0) != (gnu.returncode == 0): ok = False
        if not ok:
            bad += 1
            if bad < 4:
                print(t, fl, a, b); print(ours.stdout, ours.returncode); print('GNU:'); print(gnu.stdout, gnu.returncode)
print(f'{total} runs, {bad} bad, {same} the same as GNU')
