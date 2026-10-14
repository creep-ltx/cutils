# tar and the squeezers - design (decided with the user, 2026-10-14)

Two jobs, and a .tar.gz uses both: **tar bundles** files and drawers into one stream (names,
dates, protection; not smaller), **a squeezer** (gzip, bzip2, xz, zstd) makes one stream
smaller. The squeezers live in lib/cz.c (interface lib/cz.h) with the libraries in
third_party/.

## Decided

- **One binary** (the user, after the sizes were measured): tar carries every kind, about
  130 KB. Measured for the 68020 at -Os: gzip unpack 6 KB, pack 12 KB; bzip2 unpack 12 KB,
  pack 17 KB more; xz unpack 7 KB; zstd unpack 45 KB. Kept in separate source files, so it
  can become modules (LIBS:...) later if Ahiru wants to share it.
- **tar reads** tar (v7, ustar, GNU long names and long links, pax), plain or squeezed with
  gzip, bzip2, xz or zstd - the kind found from the first bytes, never from the name.
- **tar creates** plain, gzip (-z) and bzip2 (-j). xz and zstd packing later (big, slow on a
  68k); the interface already has room for them.
- **Small commands too**, each carrying only its own part: gzip, gunzip, bzip2, bunzip2, unxz,
  unzstd (GNU's flags: -d -c -k -f -v -1..-9, many files, `-c` to the output).
- **The libraries** (all licences fit MIT; their licence files go with them in third_party/):
  miniz (MIT) for gzip, the original libbzip2 (BSD-style), xz-embedded (0BSD), zstd's
  decoder (BSD).
- **GNU tar's flags**: c x t (also as -c), f, v, z j J --zstd (reading never needs them),
  -C drawer, --strip-components N, -k keep existing files, --exclude PATTERN, -O to the
  output, -p (protection is always kept anyway), -a auto (the kind from the archive's name
  when creating).
- **Amiga details kept**: the Unix mode's rwx map to the Amiga's r w e (w also d); the Amiga
  bits with no Unix twin (s p a h, a missing d) and the file note go in pax extended headers
  (`AMIGA.protect`, `AMIGA.comment`), so an archive made on the Amiga comes back exactly; other
  tars ignore them. Dates both ways (Unix seconds <-> DateStamp, the 2046 fix in cu_date.c).
  Soft links as symlinks; hard links within the archive as links.
- **Safety when unpacking**: no name may climb out (`..`, a leading `/` or a volume name) -
  such parts are dropped with a word, as GNU tar does; existing files replaced unless -k;
  names longer than the drawer's file system allows (FFS: 30) get a clear message, not a
  silent cut.

## The trap

bebbo's gcc 6.5 miscompiles a small struct passed in and returned by value at -O1/-Os
(Knowledge/amiga/toolchain-and-testing.md); zstd's decoder does exactly that (seq_t). Every
decoder is checked on the PC: the same code built for Linux and for the 68020 (run under
vamos --cpu 68020) must give identical bytes on many files, before anything runs on the A1200.
