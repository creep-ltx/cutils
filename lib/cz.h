/* cz.h - streams that squeeze (lib/cz.c): read gzip, bzip2, xz, zstd (or
 * plain bytes), found by the stream's first bytes; write gzip and bzip2.
 * tar, gzip, gunzip, bzip2, bunzip2, unxz and unzstd use it. docs/tar.md
 * has the decisions.
 *
 * Reading:  CzIn *z = cz_open(fh, CZ_AUTO, &kind);
 *           while ((n = cz_read(z, buf, sizeof buf)) > 0) ...;  (0 the end, -1 trouble)
 *           cz_close(z);       (the BPTR stays open: the caller closes it)
 * Writing:  CzOut *z = cz_create(fh, CZ_GZIP, 6);
 *           cz_write(z, buf, n) ...; cz_finish(z);   (writes the trailer, frees z)
 * Trouble: the functions return NULL / -1 and cz_error() says why in words.
 *
 * Which kinds a program carries: every kind, unless it names its own at
 * file level (then the others are not linked at all):
 *     CZ_READERS(&cz_gzip);                 (the kinds cz_open() knows)
 *     CZ_PACKERS(&cz_gzip_pack);            (the kinds cz_create() makes)
 * cz_open() of a kind the program does not carry: NULL, *found set, and
 * cz_error() "that kind is not in this build". */
#ifndef CZ_H
#define CZ_H

#include <exec/types.h>
#include <dos/dos.h>

enum { CZ_AUTO = -1, CZ_NONE = 0, CZ_GZIP, CZ_BZIP2, CZ_XZ, CZ_ZSTD };

typedef struct CzIn CzIn;
typedef struct CzOut CzOut;

/* kind CZ_AUTO: found from the first bytes (no known header: CZ_NONE, the
 * bytes as they are); *found (may be NULL) says which */
CzIn *cz_open(BPTR fh, int kind, int *found);
LONG cz_read(CzIn *z, void *buf, LONG n);
void cz_close(CzIn *z);

/* kind CZ_NONE, CZ_GZIP or CZ_BZIP2; level 1 (fast) .. 9 (small), 0 the default */
CzOut *cz_create(BPTR fh, int kind, int level);
LONG cz_write(CzOut *z, const void *buf, LONG n);
int cz_finish(CzOut *z);               /* 0 fine, -1 trouble; z is freed either way */

const char *cz_error(void);            /* the last trouble, in words */
int cz_kind_of_name(const char *name); /* by the ending: .gz .tgz .bz2 .tbz .tbz2 .xz .txz .zst .tzst; CZ_NONE */
const char *cz_kind_name(int kind);    /* "gzip", "bzip2", "xz", "zstd", "none" */
int cz_can_write(int kind);            /* this build packs it */

/* ---- added with the real codecs (2026-10-14) ---- */

typedef struct CzKind CzKind;
typedef struct CzPack CzPack;
extern const CzKind cz_gzip, cz_bzip2, cz_xz, cz_zstd;
extern const CzPack cz_gzip_pack, cz_bzip2_pack;
#define CZ_READERS(...) const CzKind *const cz_kinds[] = { __VA_ARGS__, NULL }
#define CZ_PACKERS(...) const CzPack *const cz_packs[] = { __VA_ARGS__, NULL }

/* gzip only, before the first cz_write(): the name (FilePart, may be NULL)
 * and the date (Unix seconds, 0 none) put in the header, as GNU gzip does */
void cz_set_name(CzOut *z, const char *name, ULONG mtime);
/* after cz_read() gave 0: 1 when bytes after the last stream were ignored
 * (GNU: "decompression OK, trailing garbage ignored") */
int cz_trailing(CzIn *z);

#endif
