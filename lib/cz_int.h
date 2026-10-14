/* cz_int.h - what lib/cz*.c share inside (not for commands; cz.h is the
 * interface). Each kind is a small table of functions in its own file, so
 * a command links only the kinds it names:
 *
 *   readers  cz_gzip (cz_gz.c), cz_bzip2 (cz_bz.c), cz_xz (cz_xz.c), cz_zstd (cz_zst.c)
 *   packers  cz_gzip_pack (cz_gzw.c), cz_bzip2_pack (cz_bzw.c)
 *
 * cz.c finds a reader in cz_kinds[], cz_w.c a packer in cz_packs[]. Both
 * lists come from cz_all.c / cz_allw.c (every kind) unless the command
 * defines its own with CZ_READERS() / CZ_PACKERS() (cz.h): the linker then
 * takes the command's and never pulls the all-kinds objects out of the
 * archive. */
#ifndef CZ_INT_H
#define CZ_INT_H

#include "cu.h"
#include "cz.h"

#define CZ_IBUF 32768            /* the input buffer */

struct CzKind {                  /* a reader */
    int kind;
    int (*open)(CzIn *z);        /* the header read, state made: 0, else -1 (cz_seterr said why) */
    LONG (*read)(CzIn *z, UBYTE *buf, LONG n);   /* >0 bytes, 0 the end, -1 trouble */
    void (*close)(CzIn *z);
};

struct CzPack {                  /* a packer */
    int kind;
    int (*create)(CzOut *z, int level);          /* 0, else -1 */
    LONG (*write)(CzOut *z, const UBYTE *buf, LONG n);
    int (*finish)(CzOut *z);     /* the end written, the state freed: 0, else -1 */
    void (*drop)(CzOut *z);      /* the state freed after trouble */
};

struct CzIn {
    BPTR fh;
    const CzKind *k;             /* NULL: plain bytes */
    UBYTE *in;                   /* CZ_IBUF bytes, in[ipos..ilen) not used yet */
    LONG ipos, ilen;
    int ieof;                    /* the file has no more */
    int state;                   /* 0 going, 1 the end, -1 trouble (sticky) */
    int kind;
    int trailing;                /* bytes after the last stream, ignored */
    void *p;                     /* the kind's own */
};

struct CzOut {
    BPTR fh;
    const CzPack *k;             /* NULL: plain bytes */
    int kind, failed;
    const char *name;            /* gzip header: the original name (NULL none) */
    ULONG mtime;                 /* gzip header: Unix seconds (0 none) */
    int started;                 /* the header is written */
    void *p;
};

extern const CzKind *const cz_kinds[];
extern const CzPack *const cz_packs[];

void cz_seterr(const char *why);
LONG cz_more(CzIn *z);           /* more input after in[ilen]: bytes read, 0 the end, -1 trouble */
int cz_avail(CzIn *z, LONG n);   /* at least n bytes in the buffer (fewer at the end): 0, -1 trouble */
int cz_put(BPTR fh, const void *buf, LONG n);   /* Write all of it: 0, else -1 (said) */

/* CRC-32 (gzip's, as zlib's crc32()): cz_crc_init() fills a 256-entry
 * table; cz_crc32(t, 0, p, n) starts, the result goes on into the next call */
void cz_crc_init(ULONG *t);
ULONG cz_crc32(const ULONG *t, ULONG crc, const UBYTE *p, LONG n);

#endif
