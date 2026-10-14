/* cz_bz.c - bzip2 read, with the original libbzip2: streams one after
 * another (pbzip2's files, or .bz2 files joined with cat), every block's
 * CRC and the stream's checked by the library. The decoder needs about
 * 3.7 MB for a -9 file; when that is not there it starts again in the
 * library's small mode (about 2.3 MB, slower). */
#include "cz_int.h"
#include "bzlib.h"

enum { P_START, P_DATA, P_END };

/* the library's memory through cu_alloc (so a test build can limit it) */
static void *bzalloc(void *u, int n, int m) { (void)u; return cu_alloc((ULONG)n * m); }
static void bzfree(void *u, void *p) { (void)u; cu_free(p); }

typedef struct {
    bz_stream s;
    int small, inited, phase;
    char magic[4];               /* the stream's first bytes, for a start in small mode */
} Bz;

static int start(CzIn *z, Bz *g)
{
    int r;
    if (g->inited) BZ2_bzDecompressEnd(&g->s);
    g->inited = 0;
    memset(&g->s, 0, sizeof g->s);
    g->s.bzalloc = bzalloc; g->s.bzfree = bzfree;
    if (cz_avail(z, 4) < 0) return -1;
    if (z->ilen - z->ipos >= 4) memcpy(g->magic, z->in + z->ipos, 4);
    r = BZ2_bzDecompressInit(&g->s, 0, g->small);
    if (r != BZ_OK) { cz_seterr(r == BZ_MEM_ERROR ? "out of memory" : "bzip2 would not start"); return -1; }
    g->inited = 1;
    g->phase = P_START;
    return 0;
}

static int bz_open(CzIn *z)
{
    Bz *g = cu_alloc(sizeof *g);
    if (!g) { cz_seterr("out of memory"); return -1; }
    memset(g, 0, sizeof *g);
    z->p = g;
    if (start(z, g) < 0) { cu_free(g); z->p = NULL; return -1; }
    return 0;
}

/* the big tables were not there: the same stream again, in small mode */
static int smaller(CzIn *z, Bz *g)
{
    int r;
    if (g->small) return -1;
    g->small = 1;
    BZ2_bzDecompressEnd(&g->s);
    memset(&g->s, 0, sizeof g->s);
    g->s.bzalloc = bzalloc; g->s.bzfree = bzfree;
    if (BZ2_bzDecompressInit(&g->s, 0, 1) != BZ_OK) { g->inited = 0; return -1; }
    g->s.next_in = g->magic;
    g->s.avail_in = 4;
    r = BZ2_bzDecompress(&g->s);
    return r == BZ_OK && !g->s.avail_in ? 0 : -1;
}

static LONG bz_read(CzIn *z, UBYTE *buf, LONG n)
{
    Bz *g = z->p;
    for (;;) {
        LONG have, used, got;
        int r;
        if (g->phase == P_END) return 0;
        if (z->ipos == z->ilen && cz_more(z) < 0) return -1;
        have = z->ilen - z->ipos;
        g->s.next_in = (char *)z->in + z->ipos;
        g->s.avail_in = have;
        g->s.next_out = (char *)buf;
        g->s.avail_out = n;
        r = BZ2_bzDecompress(&g->s);
        used = have - g->s.avail_in;
        got = n - g->s.avail_out;
        z->ipos += used;
        if (r == BZ_STREAM_END) {
            /* another stream after it, the end, or bytes that are not bzip2 */
            if (cz_avail(z, 4) < 0) return -1;
            have = z->ilen - z->ipos;
            if (!have) g->phase = P_END;
            else if (have >= 4 && !memcmp(z->in + z->ipos, "BZh", 3) && z->in[z->ipos + 3] >= '1' && z->in[z->ipos + 3] <= '9') {
                if (start(z, g) < 0) return -1;
            } else { z->trailing = 1; g->phase = P_END; }
            if (got) return got;
            continue;
        }
        if (r == BZ_OK) {
            if (got) { g->phase = P_DATA; return got; }
            if (!used && z->ieof) { cz_seterr("truncated"); return -1; }
            if (used) g->phase = P_DATA;
            continue;
        }
        if (r == BZ_MEM_ERROR && g->phase == P_START) {
            if (smaller(z, g) == 0) { g->phase = P_DATA; continue; }
            cz_seterr("out of memory");
            return -1;
        }
        cz_seterr(r == BZ_DATA_ERROR_MAGIC ? "not bzip2" : r == BZ_MEM_ERROR ? "out of memory" : "damaged: data or CRC wrong");
        return -1;
    }
}

static void bz_close(CzIn *z)
{
    Bz *g = z->p;
    if (g && g->inited) BZ2_bzDecompressEnd(&g->s);
    cu_free(g);
}

const CzKind cz_bzip2 = { CZ_BZIP2, bz_open, bz_read, bz_close };
