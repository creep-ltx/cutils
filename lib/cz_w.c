/* cz_w.c - squeezed streams written (cz.h): plain bytes here, the kinds
 * through the packers in cz_packs[] (cz_gzw.c, cz_bzw.c). Its own file so
 * a program that only unpacks links no packer. */
#include "cz_int.h"

static const CzPack *find(int kind)
{
    int i;
    for (i = 0; cz_packs[i]; i++)
        if (cz_packs[i]->kind == kind) return cz_packs[i];
    return NULL;
}

int cz_can_write(int kind) { return kind == CZ_NONE || find(kind) != NULL; }

CzOut *cz_create(BPTR fh, int kind, int level)
{
    CzOut *z;
    const CzPack *k = NULL;
    if (kind != CZ_NONE && !(k = find(kind))) { cz_seterr("that kind is not in this build"); return NULL; }
    if (level < 0 || level > 9) { cz_seterr("level must be 1 to 9"); return NULL; }
    if (!(z = cu_alloc(sizeof *z))) { cz_seterr("out of memory"); return NULL; }
    memset(z, 0, sizeof *z);
    z->fh = fh;
    z->kind = kind;
    z->k = k;
    if (k && k->create(z, level) < 0) { cu_free(z); return NULL; }
    return z;
}

void cz_set_name(CzOut *z, const char *name, ULONG mtime)
{
    z->name = name;
    z->mtime = mtime;
}

LONG cz_write(CzOut *z, const void *buf, LONG n)
{
    if (z->failed) return -1;
    if (n <= 0) return 0;
    if (!z->k) {
        if (cz_put(z->fh, buf, n) < 0) { z->failed = 1; return -1; }
        return n;
    }
    if (z->k->write(z, buf, n) < 0) { z->failed = 1; return -1; }
    return n;
}

int cz_finish(CzOut *z)
{
    int r = z->failed ? -1 : 0;
    if (z->k) {
        if (!z->failed) r = z->k->finish(z);
        else z->k->drop(z);
    }
    cu_free(z);
    return r;
}
