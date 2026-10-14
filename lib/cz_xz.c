/* cz_xz.c - xz read, with XZ Embedded: streams one after another (and
 * the zero padding between them), LZMA2 with the BCJ filters (x86, ARM,
 * ARM-Thumb, ARM64, RISC-V, PowerPC, IA-64, SPARC), every check xz writes
 * verified: CRC32, CRC64 (the default) and SHA-256 (or none). The window
 * is as big as the file was packed with: 8 MB for xz -6, 64 MB for -9;
 * "out of memory" when it is not there. Bytes after the last stream that
 * are not xz: "damaged", as xz itself says. */
#include "cz_int.h"
#define XZ_USE_CRC64             /* as in third_party/xz/xz_config.h */
#include "xz.h"

typedef struct {
    struct xz_dec *s;
    int done;
} Xz;

static int xz_open(CzIn *z)
{
    Xz *g = cu_alloc(sizeof *g);
    if (!g) { cz_seterr("out of memory"); return -1; }
    xz_crc32_init();
    xz_crc64_init();
    g->done = 0;
    if (!(g->s = xz_dec_init(XZ_DYNALLOC, 1UL << 30))) { cu_free(g); cz_seterr("out of memory"); return -1; }
    z->p = g;
    return 0;
}

static LONG xz_read(CzIn *z, UBYTE *buf, LONG n)
{
    Xz *g = z->p;
    struct xz_buf b;
    enum xz_ret r;
    for (;;) {
        LONG was;
        if (g->done) return 0;
        if (z->ipos == z->ilen && cz_more(z) < 0) return -1;
        b.in = z->in; b.in_pos = was = z->ipos; b.in_size = z->ilen;
        b.out = buf; b.out_pos = 0; b.out_size = n;
        r = xz_dec_catrun(g->s, &b, z->ieof);
        z->ipos = b.in_pos;
        switch (r) {
        case XZ_OK:
            if (b.out_pos) return b.out_pos;
            if (z->ieof && (LONG)b.in_pos == was) { cz_seterr("truncated"); return -1; }
            continue;
        case XZ_STREAM_END:
            g->done = 1;
            return b.out_pos;
        case XZ_MEM_ERROR: cz_seterr("out of memory"); return -1;
        case XZ_MEMLIMIT_ERROR: cz_seterr("needs too much memory"); return -1;
        case XZ_FORMAT_ERROR: cz_seterr("not xz"); return -1;
        case XZ_OPTIONS_ERROR: cz_seterr("unsupported: xz options or filters"); return -1;
        case XZ_UNSUPPORTED_CHECK: cz_seterr("unsupported: integrity check"); return -1;
        case XZ_BUF_ERROR: cz_seterr("truncated"); return -1;
        default: cz_seterr("damaged: data or check wrong"); return -1;
        }
    }
}

static void xz_close(CzIn *z)
{
    Xz *g = z->p;
    if (g) xz_dec_end(g->s);
    cu_free(g);
}

const CzKind cz_xz = { CZ_XZ, xz_open, xz_read, xz_close };
