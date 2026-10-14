#!/usr/bin/env python3
# mktar.py - makes tar's fixture archives in tests/files (tar-*.tar) with GNU
# tar and Python's tarfile, the same bytes each time (fixed dates, owners,
# order). Run it from anywhere: python3 -I tests/mktar.py
import io, os, shutil, subprocess, sys, tarfile, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, 'files')
T = 1577934245          # 2020-01-02 03:04:05 UTC
GNU = ['tar', '--sort=name', '--mtime=@%d' % T, '--owner=0', '--group=0', '--numeric-owner']
LONGDIR = 'd' * 40
LONGFILE = 'f' * 100
LONGTGT = 'target/' + 't' * 110


def gnutree(top):
    os.makedirs(f'{top}/empty')
    os.makedirs(f'{top}/sub/{LONGDIR}')
    open(f'{top}/zero', 'w').close()
    with open(f'{top}/text', 'w') as f: f.write('hello\n')
    with open(f'{top}/big', 'w') as f:
        for i in range(29128): f.write('%07d abcdefghijklmnopqrstuvwxyz\n' % i)
    os.symlink('text', f'{top}/soft')
    os.symlink('../text', f'{top}/sub/up')
    os.symlink(LONGTGT, f'{top}/sub/lsoft')
    os.link(f'{top}/text', f'{top}/sub/hard')
    with open(f'{top}/sub/{LONGDIR}/{LONGFILE}', 'w') as f: f.write('long\n')
    with open(f'{top}/fér', 'w') as f: f.write('accent\n')
    for p in ('zero', 'text', 'big', f'sub/{LONGDIR}/{LONGFILE}', 'fér'): os.chmod(f'{top}/{p}', 0o644)


def ustartree(top):
    a = 'a' * 60; b = 'b' * 60
    os.makedirs(f'{top}/{a}/{b}')
    with open(f'{top}/{a}/{b}/' + 'c' * 50, 'w') as f: f.write('split by the prefix\n')
    for name, mode in (('rw', 0o644), ('rwx', 0o755), ('ro', 0o444)):
        with open(f'{top}/{name}', 'w') as f: f.write(name + '\n')
        os.chmod(f'{top}/{name}', mode)
    os.symlink('rw', f'{top}/soft')
    os.link(f'{top}/rw', f'{top}/hard')


def gnutar(fmt, out, top, src):
    subprocess.run(GNU + ['--format=' + fmt, '-cf', out, top], cwd=src, check=True)


def member(t, name, data=b'', typ=tarfile.REGTYPE, link='', pax=None, mode=0o644):
    i = tarfile.TarInfo(name)
    i.type, i.linkname, i.mode, i.mtime = typ, link, mode, T
    i.size = len(data) if typ == tarfile.REGTYPE else 0
    if pax: i.pax_headers = pax
    t.addfile(i, io.BytesIO(data) if typ == tarfile.REGTYPE else None)


def main():
    src = tempfile.mkdtemp()
    try:
        gnutree(f'{src}/g'); gnutar('gnu', f'{OUT}/tar-gnu.tar', 'g', src)
        ustartree(f'{src}/u'); gnutar('ustar', f'{OUT}/tar-ustar.tar', 'u', src)
        os.makedirs(f'{src}/v/dir')
        for n in ('v/a', 'v/dir/b'):
            with open(f'{src}/{n}', 'w') as f: f.write(n + '\n')
            os.chmod(f'{src}/{n}', 0o644)
        gnutar('v7', f'{OUT}/tar-v7.tar', 'v', src)
    finally:
        shutil.rmtree(src)

    # pax, with the Amiga's own keys and a global header
    with tarfile.open(f'{OUT}/tar-pax.tar', 'w', format=tarfile.PAX_FORMAT,
                      pax_headers={'comment': 'made by mktar.py'}) as t:
        member(t, 'p/', typ=tarfile.DIRTYPE, mode=0o755, pax={'AMIGA.comment': 'a drawer note'})
        member(t, 'p/' + 'x' * 70 + '/' + 'y' * 100, b'very long\n')
        member(t, 'p/amiga', b'bits\n', pax={'AMIGA.protect': 'hsparwed', 'AMIGA.comment': 'Ein Kommentar ü'})
        member(t, 'p/nodel', b'no d\n', mode=0o755, pax={'AMIGA.protect': '----rwe-'})
        member(t, 'p/mtime', b'pax date\n', pax={'mtime': '1000000000.5'})
        member(t, 'p/lsoft', typ=tarfile.SYMTYPE, link='../' + 'z' * 150)

    # names that try to get out
    with tarfile.open(f'{OUT}/tar-evil.tar', 'w', format=tarfile.USTAR_FORMAT) as t:
        for n in ('/abs.f', '../up.f', 'a/../../x.f', 'Work:vol.f', 'ok/./dot.f', 'ok//twice.f', 'ok/co:lon.f'):
            member(t, n, n.encode() + b'\n')
        member(t, 'esc', typ=tarfile.SYMTYPE, link='/etc')
        member(t, 'esc/passwd', b'not through the link\n')

    # a small one, then damaged and cut copies
    buf = io.BytesIO()
    with tarfile.open(fileobj=buf, mode='w', format=tarfile.USTAR_FORMAT) as t:
        member(t, 's/one', b'1' * 700)
        member(t, 's/two', b'2' * 300)
        member(t, 's/three', b'3\n')
    small = buf.getvalue()
    open(f'{OUT}/tar-small.tar', 'wb').write(small)
    bad = bytearray(small)
    bad[1536 + 10] ^= 0x20                         # the second header (one is 512 + 1024 bytes of data)
    open(f'{OUT}/tar-bad.tar', 'wb').write(bytes(bad))
    open(f'{OUT}/tar-short.tar', 'wb').write(small[:1536 + 512 + 100])
    open(f'{OUT}/tar-noend.tar', 'wb').write(small[:1536 + 512 + 512 + 512 + 512])

    # squeezed copies (tarz.t): gzip -n, so no name or date inside
    for cmd, ext in ((['gzip', '-n9c'], 'tgz'), (['bzip2', '-9c'], 'tar.bz2'), (['xz', '-c'], 'tar.xz'), (['zstd', '-qc'], 'tar.zst')):
        open(f'{OUT}/tar-small.{ext}', 'wb').write(subprocess.run(cmd, input=small, capture_output=True, check=True).stdout)

    # a member bigger than 8 GB (a base-256 size): said, not choked on
    h = bytearray(tarfile.TarInfo('huge').tobuf(tarfile.GNU_FORMAT))[:512]
    h[124:136] = b'\x80' + (9 << 30).to_bytes(11, 'big')
    h[148:156] = b' ' * 8
    s = sum(h)
    h[148:156] = b'%06o\0 ' % s
    open(f'{OUT}/tar-huge.tar', 'wb').write(bytes(h) + bytes(1024))


main()
