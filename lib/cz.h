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
 * Trouble: the functions return NULL / -1 and cz_error() says why in words. */
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

#endif
