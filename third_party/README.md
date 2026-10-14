# third_party - the squeezing libraries under lib/cz*.c

Copied from the projects' own repositories on 2026-10-14, trimmed to the files CUtils
uses. Each keeps its licence file next to it; all fit CUtils' MIT licence. They are built
into build/libcu.a by the Makefile (MZOBJ, BZOBJ, XZOBJ, ZSOBJ) with CUtils' own flags
(`-Os -m68020 -resident32 -fbbb=-`) and their warnings silenced (`-w`, only for these
files), so each command links only the parts it calls. Sizes: code + data for the 68020.

| Library | Version (commit) | Licence | Used for | Size |
|---|---|---|---|---|
| [miniz](https://github.com/richgel999/miniz) | 3.1.2 (2948eeb) | MIT (`miniz/LICENSE`) | gzip: deflate (tdefl), inflate (tinfl) | 5 KB unpack, 10 KB pack |
| [bzip2](https://gitlab.com/bzip2/bzip2) (libbzip2) | 1.1.0 dev (66c46b8) | BSD-style (`bzip2/COPYING`) | bzip2 both ways | 15 KB unpack, 30 KB both |
| [XZ Embedded](https://github.com/tukaani-project/xz-embedded) | ae63ae3 | 0BSD (`xz/COPYING`) | xz unpack | 16 KB |
| [zstd](https://github.com/facebook/zstd) (the decoder) | 1.6.0 dev (49cf517) | BSD (`zstd/LICENSE`; zstd is dual BSD/GPLv2, `zstd/COPYING`) | zstd unpack | 45 KB |

## What was taken, what changed

- **miniz**: `miniz_tdef.c/.h`, `miniz_tinfl.c/.h`, `miniz_common.h`, `miniz_export.h`
  only (no zlib API, no zip archives, no stdio). `miniz.h` is CUtils' own short stand-in
  for the real one: the platform switches fixed to the portable code (so the PC test
  build runs the very code the 68020 runs), the strategy enum, and `mz_adler32` as a
  no-op (only used for zlib headers, which CUtils never writes; gzip's CRC-32 is in
  lib/cz_crc.c). Cut out of the .c files (marked `CUtils:`): the in-memory helpers
  (`*_mem_to_heap/mem/output/callback`), the PNG writer and the `*_alloc/free` pairs.
- **bzip2**: `blocksort.c huffman.c crctable.c randtable.c compress.c decompress.c
  bzlib.h bzlib_private.h bz_version.h`, built with `BZ_NO_STDIO` (lib/cz_bzerr.c gives
  it `bz_internal_error`). `bzlib.c` is split in two, unchanged otherwise:
  `bzlib_enc.c` (the compression API) and `bzlib_dec.c` (the decompression API), so an
  unpacker does not link the packer; the stdio file functions, BuffToBuff and
  bzlibVersion are left out. Its memory comes through cu_alloc (lib/cz_bz.c).
- **XZ Embedded**: `linux/lib/xz/{xz_dec_stream,xz_dec_lzma2,xz_dec_bcj,xz_crc32,
  xz_crc64,xz_sha256}.c` with their headers, `linux/include/linux/xz.h` and
  `userspace/xz_config.h`, all in `xz/`. `xz_config.h` turns on concatenated streams
  (`xz_dec_catrun`), CRC64, SHA-256, every BCJ filter (x86, ARM, ARM-Thumb, ARM64,
  RISC-V, PowerPC, IA-64, SPARC) and `XZ_DEC_DYNALLOC` only. **One change in
  `xz_dec_lzma2.c`** (all under `#ifdef XZ_DEC_GROW`, marked `CUtils:`): the dictionary
  starts at 64 KB and doubles when a chunk needs room, up to the size the file asks for
  (8 MB for xz -6). Without it every xz -6 file, even 100 bytes, wanted 8 MB at once,
  which the 8 MB test A1200 does not have. The buffer only wraps around once it has
  its full size, so growing is a plain copy.
- **zstd**: `lib/zstd.h`, `lib/zstd_errors.h`, `lib/decompress/{zstd_decompress,
  zstd_decompress_block,huf_decompress,zstd_ddict}.c` and `lib/common/{entropy_common,
  fse_decompress,xxhash,zstd_common,error_private}.c` with the headers they include;
  unchanged. Built with `ZSTD_LEGACY_SUPPORT=0` (no v0.7-and-older frames),
  `ZSTD_DISABLE_ASM`, `ZSTD_NO_INTRINSICS`, `HUF_FORCE_DECOMPRESS_X1` and
  `ZSTD_FORCE_DECOMPRESS_SEQUENCES_SHORT` (the small decoders: measured on the 14 MHz
  68020, -O2 and the big decoders gained 12 % for 34 KB more), `ZSTD_NO_TRACE`,
  `ZSTD_STRIP_ERROR_STRINGS` (lib/cz_zst.c words its own errors).

## Checked

tests/cz-pc/run.sh builds these and lib/cz*.c for Linux (with the sanitizers) and for
the 68020 (run under vamos) and compares both on a corpus made by the real gzip, bzip2,
xz and zstd, damaged and cut files included: bebbo's gcc has miscompiled small structs
passed by value before (docs/tar.md), and zstd does exactly that.
