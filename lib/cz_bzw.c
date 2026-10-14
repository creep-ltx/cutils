/* cz_bzw.c - bzip2 written with the original libbzip2: level 1..9 is the
 * block size in 100 KB (bzip2's -1..-9; 9 the default, as bzip2). It
 * needs about 400 KB + 8 times the block: 7.6 MB at -9, 1.2 MB at -1. */
#include "cz_int.h"
#include "bzlib.h"

#define OBUF 16384

static void *bzalloc(void *u, int n, int m) { (void)u; return cu_alloc((ULONG)n * m); }
static void bzfree(void *u, void *p) { (void)u; cu_free(p); }

typedef struct {
    bz_stream s;
    char out[OBUF];
} BzW;

static int bzw_create(CzOut *z, int level)
{
    BzW *g = cu_alloc(sizeof *g);
    int r;
    if (!g) { cz_seterr("out of memory"); return -1; }
    memset(&g->s, 0, sizeof g->s);
    g->s.bzalloc = bzalloc; g->s.bzfree = bzfree;
    r = BZ2_bzCompressInit(&g->s, level ? level : 9, 0, 0);
    if (r != BZ_OK) {
        cu_free(g);
        cz_seterr(r == BZ_MEM_ERROR ? "out of memory (a lower level needs less)" : "bzip2 would not start");
        return -1;
    }
    z->p = g;
    return 0;
}

/* the packer run over the input (BZ_RUN or BZ_FINISH), written as it fills */
static int run(CzOut *z, const UBYTE *buf, LONG n, int action)
{
    BzW *g = z->p;
    int r;
    g->s.next_in = (char *)buf;
    g->s.avail_in = n;
    for (;;) {
        g->s.next_out = g->out;
        g->s.avail_out = OBUF;
        r = BZ2_bzCompress(&g->s, action);
        if (r < 0) { cz_seterr("bzip2 failed"); return -1; }
        if (cz_put(z->fh, g->out, OBUF - g->s.avail_out) < 0) return -1;
        if (action == BZ_FINISH ? r == BZ_STREAM_END : !g->s.avail_in) return 0;
    }
}

static LONG bzw_write(CzOut *z, const UBYTE *buf, LONG n)
{
    return run(z, buf, n, BZ_RUN) < 0 ? -1 : n;
}

static int bzw_finish(CzOut *z)
{
    BzW *g = z->p;
    int r = run(z, NULL, 0, BZ_FINISH);
    BZ2_bzCompressEnd(&g->s);
    cu_free(g);
    z->p = NULL;
    return r;
}

static void bzw_drop(CzOut *z)
{
    BzW *g = z->p;
    if (g) { BZ2_bzCompressEnd(&g->s); cu_free(g); }
    z->p = NULL;
}

const CzPack cz_bzip2_pack = { CZ_BZIP2, bzw_create, bzw_write, bzw_finish, bzw_drop };
