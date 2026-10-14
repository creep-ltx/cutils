#!/usr/bin/env bash
# run.sh - the squeezers (lib/cz*.c) checked on the PC before any Amiga:
#  1. a corpus made with the PC's own gzip, bzip2, xz and zstd (several
#     levels and options, 0 bytes to 5 MB, multi-member and joined
#     streams, every gzip header field) unpacked by czt built for Linux
#     (pc-asan: with the sanitizers) and for the 68020 (am, under vamos):
#     both must give the original bytes and the same words;
#  2. damaged and cut files: both must fail, in the same words, never hang;
#  3. czt's gzip and bzip2 packing: the PC's gzip -t / bzip2 -t take it,
#     it unpacks to the original, and the 68020 build writes the same bytes.
# bebbo's gcc has miscompiled small structs passed by value (docs/tar.md),
# so the 68020 build is never trusted without this.
# Usage: tests/cz-pc/run.sh [quick]     (quick: no files over 1 MB)
set -e
T=$(cd "$(dirname "$0")" && pwd)
make -s -C "$T/../.." build/libcu.a
make -s -C "$T" all
W="$T/w"
rm -rf "$W"; mkdir -p "$W"
cp "$T/pc" "$T/pc-asan" "$T/am" "$W/"
cd "$W"
exec python3 -I - "${1:-full}" <<'PY'
import os, random, subprocess, sys, zlib, struct, threading, concurrent.futures as cf
quick = sys.argv[1] == 'quick'
rng = random.Random(1)
def sh(cmd):
    subprocess.run(cmd, shell=True, check=True)

# ---- the plain contents ----
def text(n, seed):
    r = random.Random(seed)
    words = ['amiga', 'the', 'workbench', 'squeeze', 'data', 'stream', 'of', 'and', 'kickstart', 'chip', 'fast',
             'memory', 'blitter', 'copper', 'a', 'in', 'to', 'is', 'files', 'drawer']
    out = []
    size = 0
    while size < n:
        line = ' '.join(r.choice(words) for _ in range(r.randint(1, 14))) + '\n'
        out.append(line); size += len(line)
    return ''.join(out).encode()[:n]
plain = {
    'empty': b'',
    'one': b'x',
    'small': b'Hello, Amiga!\n',
    'text100k': text(100000, 2),
    'zeros1m': bytes(1 << 20),
    'rand64k': rng.randbytes(65536),
    'rand1m': rng.randbytes(1 << 20),
    'mixed': b''.join((text(3000, i) if i % 3 else rng.randbytes(2000)) for i in range(300)),
    'exe': open('/usr/bin/ls', 'rb').read(),
    't512k': text(524288, 4),            # exactly the xz window after three doublings
}
if not quick:
    plain['text3m'] = text(3 << 20, 3)
    plain['mixed5m'] = b''.join((text(9000, i) if i % 4 else rng.randbytes(4000) + bytes(3000)) for i in range(400))[:5 << 20]
for k, v in plain.items():
    open(k, 'wb').write(v)

# ---- packed with the PC's tools ----
cases = []          # (packed file, plain name or None for damaged)
def add(f, p): cases.append((f, p))
for k in plain:
    for lv in (1, 6, 9):
        sh(f'gzip -{lv} -c {k} > {k}.{lv}.gz'); add(f'{k}.{lv}.gz', k)
    for lv in (1, 9):
        sh(f'bzip2 -{lv} -c {k} > {k}.{lv}.bz2'); add(f'{k}.{lv}.bz2', k)
    for lv, opt in ((0, ''), (1, ''), (6, ''), (9, '-e'), (6, '--check=crc32'), (6, '--check=sha256'), (1, '--check=none')):
        name = f'{k}.{lv}{opt.replace("--check=", "")}.xz'
        sh(f'xz -{lv} {opt} -c {k} > {name}'); add(name, k)
    for lv, opt in ((1, ''), (3, ''), (19, ''), (3, '--no-check')):
        name = f'{k}.{lv}{opt}.zst'
        sh(f'zstd -q -{lv} {opt} -c {k} > {name}'); add(name, k)
sh('xz -6 --x86 --lzma2 -c exe > exe.x86.xz'); add('exe.x86.xz', 'exe')
sh('xz -6 --arm --lzma2 -c exe > exe.arm.xz'); add('exe.arm.xz', 'exe')
sh('xz -6 --powerpc --lzma2 -c exe > exe.ppc.xz'); add('exe.ppc.xz', 'exe')
sh('zstd -q --format=zstd --no-content-size -3 -c text100k > text100k.nosize.zst'); add('text100k.nosize.zst', 'text100k')
sh('cat text100k mixed > tm'); plain['tm'] = open('tm', 'rb').read()
# members and streams one after another
sh('cat text100k.6.gz mixed.1.gz > tm.multi.gz'); add('tm.multi.gz', 'tm')
sh('cat text100k.9.bz2 mixed.1.bz2 > tm.multi.bz2'); add('tm.multi.bz2', 'tm')
sh('cat text100k.6.xz mixed.0.xz > tm.multi.xz'); add('tm.multi.xz', 'tm')
sh('cat text100k.3.zst mixed.19.zst > tm.multi.zst'); add('tm.multi.zst', 'tm')
open('tm.pad.xz', 'wb').write(open('text100k.6.xz', 'rb').read() + bytes(8) + open('mixed.0.xz', 'rb').read() + bytes(4))
add('tm.pad.xz', 'tm')
skip = struct.pack('<II', 0x184D2A50, 5) + b'hello'
open('skip.zst', 'wb').write(skip + open('text100k.3.zst', 'rb').read() + struct.pack('<II', 0x184D2A5F, 0))
add('skip.zst', 'text100k')
sh('cat empty.6.gz small.6.gz empty.6.gz > es.gz'); add('es.gz', 'small')
sh('bzip2 -9 -c small > s.bz2; cat empty.9.bz2 s.bz2 > es.bz2'); add('es.bz2', 'small')
sh('pbzip2 -c -b1 -p2 mixed > mixed.pbz.bz2 2>/dev/null || cat mixed.1.bz2 > mixed.pbz.bz2'); add('mixed.pbz.bz2', 'mixed')
# every gzip header field: FTEXT FHCRC FEXTRA FNAME FCOMMENT
def gzhead(data, flags, extra=b'', name=b'', comment=b''):
    h = bytes([0x1f, 0x8b, 8, flags]) + struct.pack('<I', 1234567890) + bytes([0, 3])
    if flags & 4: h += struct.pack('<H', len(extra)) + extra
    if flags & 8: h += name + b'\0'
    if flags & 16: h += comment + b'\0'
    if flags & 2: h += struct.pack('<H', zlib.crc32(h) & 0xffff)
    c = zlib.compressobj(6, zlib.DEFLATED, -15)
    return h + c.compress(data) + c.flush() + struct.pack('<II', zlib.crc32(data), len(data) & 0xffffffff)
d = plain['text100k']
open('hdr.all.gz', 'wb').write(gzhead(d, 1 | 2 | 4 | 8 | 16, b'AB\x04\0abcd' * 30, b'a name.txt', b'a comment'))
add('hdr.all.gz', 'text100k')
open('hdr.hcrc.gz', 'wb').write(gzhead(d, 2)); add('hdr.hcrc.gz', 'text100k')
open('hdr.mix.gz', 'wb').write(gzhead(d, 8, name=b'x') + gzhead(plain['small'], 4 | 16, b'', comment=b'c'))
plain['tsmall'] = d + plain['small']; add('hdr.mix.gz', 'tsmall')
# stored blocks (level 0) and a huge member count
c = zlib.compressobj(0, zlib.DEFLATED, -15)
open('stored.gz', 'wb').write(bytes([0x1f, 0x8b, 8, 0, 0, 0, 0, 0, 0, 3]) + c.compress(plain['rand64k']) + c.flush()
                              + struct.pack('<II', zlib.crc32(plain['rand64k']), 65536))
add('stored.gz', 'rand64k')
many = b''.join(gzhead(plain['text100k'][i * 100:(i + 1) * 100], 0) for i in range(1000))
plain['t100'] = plain['text100k'][:100000]
open('many.gz', 'wb').write(many); add('many.gz', 't100')
for k in ('tsmall', 't100'):
    open(k, 'wb').write(plain[k])
good = list(cases)

# ---- damaged: a byte changed, cut short; and not packed at all ----
bad = []
for f, p in good:
    if not p in ('text100k', 'mixed', 'rand64k', 'small') or f.endswith('none.xz') or 'no-check' in f:
        continue
    data = open(f, 'rb').read()
    for i, pos in enumerate((len(data) // 2, len(data) - 6, len(data) - 1, 12)):
        if pos < 0 or pos >= len(data): continue
        b = bytearray(data); b[pos] ^= 0x55
        open(f'{f}.flip{i}', 'wb').write(bytes(b)); bad.append(f'{f}.flip{i}')
    for i, n in enumerate((len(data) // 2, len(data) - 1, len(data) - 4, 10, 3)):
        if n <= 0 or n >= len(data): continue
        open(f'{f}.cut{i}', 'wb').write(data[:n]); bad.append(f'{f}.cut{i}')
open('garbage.gz', 'wb').write(open('small.6.gz', 'rb').read() + b'garbage')
open('garbage.bz2', 'wb').write(open('small.9.bz2', 'rb').read() + b'garbage')

# ---- run: the PC build and the 68020 build, the same file each ----
# each run its own volumes drawer: several vamos at once share ~/.vamos's RAM: otherwise
VAMOS = 'vamos -q --cpu 68020 -m 262144 -s 64 -H disable --vols-base-dir vols/{} --'
def run(args, step=None):
    a = ' '.join(args)
    try:
        pc = subprocess.run(f'./pc-asan {a} 2>&1', shell=True, capture_output=True, timeout=120)
        fpc = args[2] if args[0] == 'd' else args[4]
        out_pc = open(fpc, 'rb').read() if os.path.exists(fpc) else b'(none)'
        vb = threading.get_ident()
        os.makedirs(f'vols/{vb}', exist_ok=True)
        am = subprocess.run(f'{VAMOS.format(vb)} ./am {a.replace("o.pc", "o.am")}', shell=True, capture_output=True, timeout=1800)
        fam = fpc.replace('o.pc', 'o.am')
        out_am = open(fam, 'rb').read() if os.path.exists(fam) else b'(none)'
    except subprocess.TimeoutExpired:
        return None
    return pc.stdout.decode(errors='replace').strip(), out_pc, am.stdout.decode(errors='replace').strip(), out_am, am.stderr.decode(errors='replace')

fails = []
def check_good(job):
    i, (f, p), step = job
    r = run(['d', f, f'o.pc{i}', str(step)])
    os.remove(f'o.pc{i}') if os.path.exists(f'o.pc{i}') else None
    if r is None: return f'HANG {f}'
    spc, opc, sam, oam, err = r
    os.remove(f'o.am{i}') if os.path.exists(f'o.am{i}') else None
    want = plain[p]
    if ' ok ' not in spc or opc != want: return f'PC wrong {f} (step {step}): {spc}'
    if sam != spc: return f'68020 says other words {f}: pc "{spc}" 68020 "{sam}" {err[-300:]}'
    if oam != opc: return f'68020 bytes differ {f} (step {step}) first at {next((k for k in range(min(len(oam), len(opc))) if oam[k] != opc[k]), min(len(oam), len(opc)))}'
    return None
def check_bad(job):
    i, f = job
    r = run(['d', f, f'o.pc{i}'])
    for x in (f'o.pc{i}', f'o.am{i}'):
        if os.path.exists(x): os.remove(x)
    if r is None: return f'HANG {f}'
    spc, opc, sam, oam, err = r
    if f.startswith('garbage'):
        if not spc.endswith(' ok 14 trailing'): return f'trailing bytes not seen {f}: {spc}'
    elif f.endswith('.cut4') and spc.startswith('none ok'): pass      # 3 bytes: too few to know the kind (gzip's 2 excepted)
    elif f.endswith('.flip3') and (f.endswith('.gz.flip3') or f.startswith('skip')): pass   # the name / a skippable frame: nothing checks it
    elif 'error' not in spc and not f.endswith('.flip2'): return f'not caught {f}: {spc}'
    if 'runtime error' in spc or 'AddressSanitizer' in spc: return f'PC sanitizer {f}: {spc[:300]}'
    if sam != spc: return f'68020 says other words {f}: pc "{spc}" 68020 "{sam}" {err[-300:]}'
    if oam != opc: return f'68020 partial bytes differ {f}'
    return None
def check_pack(job):
    i, k, kind, lv = job
    ext = 'gz' if kind == 1 else 'bz2'
    r = run(['z', str(kind), str(lv), k, f'o.pc{i}.{ext}'])
    if r is None: return f'HANG pack {k}'
    spc, opc, sam, oam, err = r
    tool = 'gzip' if kind == 1 else 'bzip2'
    try:
        ok = subprocess.run(f'{tool} -t o.pc{i}.{ext}', shell=True, capture_output=True).returncode == 0
        back = subprocess.run(f'{tool} -dc o.pc{i}.{ext}', shell=True, capture_output=True).stdout
    finally:
        for x in (f'o.pc{i}.{ext}', f'o.am{i}.{ext}'):
            if os.path.exists(x): os.remove(x)
    if ' ok ' not in spc: return f'pack failed {k} {tool} -{lv}: {spc}'
    if not ok or back != plain[k]: return f'{tool} -t refuses ours: {k} -{lv}'
    if sam != spc or oam != opc: return f'68020 packs other bytes: {k} {tool} -{lv} ({len(opc)} vs {len(oam)}) {sam} {err[-200:]}'
    return None

jobs = []
n = 0
for c in good:
    jobs.append((check_good, (n, c, 65536))); n += 1
    if len(plain[c[1]]) <= 100000 and ('text100k.6' in c[0] or 'small' in c[0] or 'hdr' in c[0] or 'multi' in c[0] or 'skip' in c[0]):
        jobs.append((check_good, (n, c, 7))); n += 1
        if len(plain[c[1]]) < 200 or 'hdr.all' in c[0]:
            jobs.append((check_good, (n, c, 1))); n += 1
for f in bad + ['garbage.gz', 'garbage.bz2', 'text100k', 'empty']:
    jobs.append((check_bad if f not in ('text100k', 'empty') else check_good, (n, f) if f not in ('text100k', 'empty') else (n, (f, f), 65536))); n += 1
for k in ('empty', 'one', 'small', 'text100k', 'zeros1m', 'rand64k', 'mixed', 'exe') + (('text3m',) if not quick else ()):
    for kind, lv in ((1, 1), (1, 6), (1, 9), (2, 1), (2, 9)):
        jobs.append((check_pack, (n, k, kind, lv))); n += 1
print(f'{len(good)} packed files, {len(bad)} damaged, {len(jobs)} runs (each on the PC and under vamos)', flush=True)
done = 0
with cf.ThreadPoolExecutor(max_workers=6) as ex:
    futs = [ex.submit(fn, a) for fn, a in jobs]
    for fu in cf.as_completed(futs):
        r = fu.result(); done += 1
        if r: fails.append(r); print('FAIL', r, flush=True)
        if done % 50 == 0: print(f'  {done}/{len(jobs)}', flush=True)
# ---- a small Amiga ----
# xz -6 asks for an 8 MB window: it grows from 64 KB as needed, so small
# files unpack in little memory (vamos with little RAM: it cannot say no to
# an AllocMem, it stops the run, so only what must fit is tried there)
low = [('t512k.6.xz', 't512k', 1400), ('text100k.6.xz', 'text100k', 1500), ('mixed.9-e.xz', 'mixed', 3000), ('text100k.19.zst', 'text100k', 2000)]
for f, p, kb in low:
    r = subprocess.run(f'vamos -q --cpu 68020 -m {kb} -s 64 -H disable --vols-base-dir vols/low -- ./am d {f} o.low',
                       shell=True, capture_output=True, timeout=600)
    out = open('o.low', 'rb').read() if os.path.exists('o.low') else b''
    say = r.stdout.decode(errors='replace').strip()
    if ' ok ' not in say or out != plain[p]:
        fails.append(f'low memory ({kb} KB) {f}: {say}'); print('FAIL', fails[-1])
    else:
        print(f'ok   {f} in {kb} KB of vamos memory')
# bzip2 -9 wants 3.6 MB in one block to unpack: with less it starts again in
# small mode (1.8 MB the biggest); with less still: "out of memory" (the PC
# build, every block over CZ_MEMLIMIT refused)
for lim, want in ((3000000, 'bzip2 ok 800000'), (1000000, 'bzip2 error: out of memory')):
    r = subprocess.run(f'CZ_MEMLIMIT={lim} ./pc d mixed.9.bz2 o.low', shell=True, capture_output=True, timeout=600)
    say = r.stdout.decode(errors='replace').strip()
    out = open('o.low', 'rb').read()
    low.append(None)
    if say != want or (' ok ' in want and out != plain['mixed']):
        fails.append(f'bzip2 with blocks of {lim} at most: {say}'); print('FAIL', fails[-1])
    else: print(f'ok   bzip2 -9 with no block over {lim}: {say}')
jobs += low
# trailing garbage is not an error: "ok ... trailing"
print(('ok   ' if not fails else 'FAIL ') + f'{len(jobs) - len(fails)} of {len(jobs)} runs agree')
sys.exit(1 if fails else 0)
PY
