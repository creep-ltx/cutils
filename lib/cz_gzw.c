/* cz_gzw.c - gzip written (RFC 1952) with miniz's deflate (tdefl): one
 * member, a header with the name and date only when cz_set_name() gave
 * them, the OS byte 1 (Amiga), then CRC-32 and length. tdefl's state is
 * about 320 KB, taken once per stream. */
#include "cz_int.h"
#include "miniz.h"

typedef struct {
    tdefl_compressor d;
    ULONG t[256];
    ULONG crc, size;
    int level;
} GzW;

static mz_bool putbuf(const void *buf, int len, void *u)
{
    CzOut *z = u;
    return cz_put(z->fh, buf, len) == 0;
}

static int gzw_create(CzOut *z, int level)
{
    GzW *g = cu_alloc(sizeof *g);
    if (!g) { cz_seterr("out of memory"); return -1; }
    if (!level) level = 6;
    g->level = level;
    cz_crc_init(g->t);
    g->crc = g->size = 0;
    if (tdefl_init(&g->d, putbuf, z, tdefl_create_comp_flags_from_zip_params(level, -15, MZ_DEFAULT_STRATEGY)) != TDEFL_STATUS_OKAY) {
        cu_free(g);
        cz_seterr("deflate would not start");
        return -1;
    }
    z->p = g;
    return 0;
}

static int start(CzOut *z)
{
    GzW *g = z->p;
    UBYTE h[10];
    h[0] = 0x1f; h[1] = 0x8b; h[2] = 8;
    h[3] = z->name && *z->name ? 8 : 0;
    h[4] = z->mtime; h[5] = z->mtime >> 8; h[6] = z->mtime >> 16; h[7] = z->mtime >> 24;
    h[8] = g->level == 9 ? 2 : g->level == 1 ? 4 : 0;
    h[9] = 1;                            /* the OS: Amiga */
    z->started = 1;
    if (cz_put(z->fh, h, 10) < 0) return -1;
    if (h[3] && cz_put(z->fh, z->name, strlen(z->name) + 1) < 0) return -1;
    return 0;
}

static LONG gzw_write(CzOut *z, const UBYTE *buf, LONG n)
{
    GzW *g = z->p;
    if (!z->started && start(z) < 0) return -1;
    g->crc = cz_crc32(g->t, g->crc, buf, n);
    g->size += n;
    if (tdefl_compress_buffer(&g->d, buf, n, TDEFL_NO_FLUSH) < 0) {
        cz_seterr("write error");
        return -1;
    }
    return n;
}

static int gzw_finish(CzOut *z)
{
    GzW *g = z->p;
    UBYTE t[8];
    int r = -1;
    if (!z->started && start(z) < 0) goto out;
    if (tdefl_compress_buffer(&g->d, NULL, 0, TDEFL_FINISH) != TDEFL_STATUS_DONE) { cz_seterr("write error"); goto out; }
    t[0] = g->crc; t[1] = g->crc >> 8; t[2] = g->crc >> 16; t[3] = g->crc >> 24;
    t[4] = g->size; t[5] = g->size >> 8; t[6] = g->size >> 16; t[7] = g->size >> 24;
    r = cz_put(z->fh, t, 8);
out:
    cu_free(g);
    z->p = NULL;
    return r;
}

static void gzw_drop(CzOut *z) { cu_free(z->p); z->p = NULL; }

const CzPack cz_gzip_pack = { CZ_GZIP, gzw_create, gzw_write, gzw_finish, gzw_drop };
