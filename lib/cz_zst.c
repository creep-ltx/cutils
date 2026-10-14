/* cz_zst.c - zstd read, with zstd's own decoder (no legacy formats):
 * frames one after another, skippable frames passed over, the frame's
 * checksum (XXH64) checked when it has one. The window is as big as the
 * frame asks (the file's size for small files, 8 MB for most -19 files),
 * at most 128 MB (zstd's own default limit; --long files beyond it: "needs
 * too much memory"). */
#include "cz_int.h"
#define ZSTD_STATIC_LINKING_ONLY
#include "zstd.h"
#include "zstd_errors.h"

typedef struct {
    ZSTD_DStream *ds;
    int done;
    size_t last;                 /* what the last call that moved anything said: 0 a frame ended */
} Zs;

static int zs_open(CzIn *z)
{
    Zs *g = cu_alloc(sizeof *g);
    if (!g) { cz_seterr("out of memory"); return -1; }
    g->done = 0;
    g->last = 1;
    if (!(g->ds = ZSTD_createDStream())) { cu_free(g); cz_seterr("out of memory"); return -1; }
    ZSTD_initDStream(g->ds);
    z->p = g;
    return 0;
}

static LONG zs_read(CzIn *z, UBYTE *buf, LONG n)
{
    Zs *g = z->p;
    ZSTD_inBuffer in;
    ZSTD_outBuffer out;
    size_t r;
    for (;;) {
        if (g->done) return 0;
        if (z->ipos == z->ilen && cz_more(z) < 0) return -1;
        in.src = z->in; in.size = z->ilen; in.pos = z->ipos;
        out.dst = buf; out.size = n; out.pos = 0;
        r = ZSTD_decompressStream(g->ds, &out, &in);
        if (in.pos != (size_t)z->ipos || out.pos) g->last = r;
        z->ipos = in.pos;
        if (ZSTD_isError(r)) {
            switch (ZSTD_getErrorCode(r)) {
            case ZSTD_error_prefix_unknown: cz_seterr("not zstd"); break;
            case ZSTD_error_checksum_wrong: cz_seterr("damaged: checksum wrong"); break;
            case ZSTD_error_memory_allocation: cz_seterr("out of memory"); break;
            case ZSTD_error_frameParameter_windowTooLarge: cz_seterr("needs too much memory"); break;
            case ZSTD_error_frameParameter_unsupported: cz_seterr("unsupported: zstd frame parameters"); break;
            case ZSTD_error_dictionary_wrong: cz_seterr("unsupported: needs a dictionary"); break;
            default: cz_seterr("damaged: bad data"); break;
            }
            return -1;
        }
        if (out.pos) return out.pos;
        if (z->ipos == z->ilen && z->ieof) {
            if (g->last) { cz_seterr("truncated"); return -1; }
            g->done = 1;
            return 0;
        }
    }
}

static void zs_close(CzIn *z)
{
    Zs *g = z->p;
    if (g) ZSTD_freeDStream(g->ds);
    cu_free(g);
}

const CzKind cz_zstd = { CZ_ZSTD, zs_open, zs_read, zs_close };
