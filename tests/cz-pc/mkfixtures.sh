#!/usr/bin/env bash
# mkfixtures.sh - how tests/files/cz* were made (with the PC's gzip 1.15,
# bzip2 1.0.8, xz 5.8.4, zstd 1.5.7), for the cases gzip.t ... unzstd.t.
# Kept for the record: the files are in git, do not remake them for a run.
set -e
cd "$(dirname "$0")/../files"
for i in $(seq 1 25); do echo "line $i of the squeeze test: gzip bzip2 xz zstd"; done > czdata
gzip -9 -n -c czdata > czdata.gz
bzip2 -1 -c czdata > czdata.bz2          # -1: a -9 file needs 3.7 MB to unpack, the test emulator has 8 MB
xz -6 -c czdata > czdata.xz
zstd -q -19 -c czdata > czdata.zst
head -c 400 czdata | gzip -n -c > czmulti.gz      # two members, together czdata
tail -c +401 czdata | gzip -n -c >> czmulti.gz
cp czdata.gz czbad.gz                    # a byte in the deflate data changed
printf '\x55' | dd of=czbad.gz bs=1 seek=40 conv=notrunc 2>/dev/null
python3 -I -c "d = bytearray(open('czdata.gz', 'rb').read()); d[-8] ^= 0xff; open('czcrc.gz', 'wb').write(d)"   # the CRC wrong
head -c 60 czdata.xz > czcut.xz          # cut short
cat czdata.gz > czjunk.gz                # bytes after the member
printf 'junk\n' >> czjunk.gz
printf 'tarball stand-in\n' | gzip -n -c > cztar.tgz
cp czdata czwrong.gz                     # not gzip at all
