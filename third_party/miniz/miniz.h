/* miniz.h - CUtils' cut-down stand-in for miniz's own miniz.h: only the
 * raw deflate (tdefl) and inflate (tinfl) parts are kept, no zlib API, no
 * zip archives, no stdio. The platform switches are fixed to the portable
 * code paths, so the PC test build (tests/cz-pc) runs exactly the code the
 * 68020 build runs. The original header is in miniz 3.1.2's sources. */
#pragma once

#include "miniz_export.h"

#if defined(__GNUC__)
#define MZ_FORCEINLINE __inline__ __attribute__((__always_inline__))
#else
#define MZ_FORCEINLINE
#endif

#define MINIZ_NO_STDIO
#define MINIZ_NO_TIME
#define MINIZ_NO_ARCHIVE_APIS
#define MINIZ_NO_ZLIB_APIS

/* the same, portable code on every machine */
#define MINIZ_X86_OR_X64_CPU 0
#define MINIZ_LITTLE_ENDIAN 0
#define MINIZ_USE_UNALIGNED_LOADS_AND_STORES 0
#define MINIZ_HAS_64BIT_REGISTERS 0

#include <stddef.h>

typedef unsigned long mz_ulong;

/* strategies, for tdefl_create_comp_flags_from_zip_params() */
enum { MZ_DEFAULT_STRATEGY = 0, MZ_FILTERED = 1, MZ_HUFFMAN_ONLY = 2, MZ_RLE = 3, MZ_FIXED = 4 };
#define MZ_DEFAULT_LEVEL 6

/* tdefl calls it only with TDEFL_WRITE_ZLIB_HEADER / TDEFL_COMPUTE_ADLER32,
 * which CUtils never sets (gzip carries a CRC-32, computed in lib/cz_gz.c) */
#define mz_adler32(adler, p, n) (adler)

#include "miniz_common.h"
#include "miniz_tdef.h"
#include "miniz_tinfl.h"
