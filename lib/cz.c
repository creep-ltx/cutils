/* cz.c - the squeezing streams (cz.h). THIS IS A STAND-IN: plain bytes only,
 * so tar can be written and tested before the real codecs land; the codec
 * session replaces this file. */
#include "cu.h"
#include "cz.h"

struct CzIn { BPTR fh; };
struct CzOut { BPTR fh; };

static const char *err = "";

const char *cz_error(void) { return err; }

CzIn *cz_open(BPTR fh, int kind, int *found)
{
    CzIn *z;
    if (kind > CZ_NONE) { err = "that kind is not in this build"; return NULL; }
    if (!(z = cu_alloc(sizeof *z))) { err = "out of memory"; return NULL; }
    z->fh = fh;
    if (found) *found = CZ_NONE;
    return z;
}

LONG cz_read(CzIn *z, void *buf, LONG n) { LONG r = Read(z->fh, buf, n); if (r < 0) err = "read error"; return r; }
void cz_close(CzIn *z) { cu_free(z); }

CzOut *cz_create(BPTR fh, int kind, int level)
{
    CzOut *z;
    (void)level;
    if (kind != CZ_NONE) { err = "that kind is not in this build"; return NULL; }
    if (!(z = cu_alloc(sizeof *z))) { err = "out of memory"; return NULL; }
    z->fh = fh;
    return z;
}

LONG cz_write(CzOut *z, const void *buf, LONG n) { LONG r = Write(z->fh, (APTR)buf, n); if (r != n) { err = "write error"; return -1; } return r; }
int cz_finish(CzOut *z) { cu_free(z); return 0; }

int cz_kind_of_name(const char *name)
{
    static const struct { const char *e; int k; } t[] = {
        { ".tgz", CZ_GZIP }, { ".gz", CZ_GZIP }, { ".tbz2", CZ_BZIP2 }, { ".tbz", CZ_BZIP2 }, { ".bz2", CZ_BZIP2 },
        { ".txz", CZ_XZ }, { ".xz", CZ_XZ }, { ".tzst", CZ_ZSTD }, { ".zst", CZ_ZSTD }, { NULL, 0 }
    };
    int i, n = strlen(name);
    for (i = 0; t[i].e; i++) {
        int e = strlen(t[i].e);
        if (n > e && !stricmp(name + n - e, t[i].e)) return t[i].k;
    }
    return CZ_NONE;
}

const char *cz_kind_name(int kind)
{
    return kind == CZ_GZIP ? "gzip" : kind == CZ_BZIP2 ? "bzip2" : kind == CZ_XZ ? "xz" : kind == CZ_ZSTD ? "zstd" : "none";
}

int cz_can_write(int kind) { return kind == CZ_NONE; }
