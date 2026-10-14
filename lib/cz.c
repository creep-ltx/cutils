/* cz.c - the squeezing streams (cz.h), the common part: the kind found
 * from the first bytes, the input buffer, plain bytes, the words for
 * trouble. Each kind is in its own file (cz_int.h says how a program gets
 * only the kinds it names); cz_w.c makes the packed streams. */
#include "cz_int.h"

static const char *err = "";

const char *cz_error(void) { return err; }
void cz_seterr(const char *why) { err = why; }

LONG cz_more(CzIn *z)
{
    LONG r;
    if (z->ieof) return 0;
    if (z->ipos == z->ilen) z->ipos = z->ilen = 0;
    else if (z->ipos && z->ilen == CZ_IBUF) {
        memmove(z->in, z->in + z->ipos, z->ilen - z->ipos);
        z->ilen -= z->ipos; z->ipos = 0;
    }
    if (z->ilen == CZ_IBUF) return CZ_IBUF;     /* full: nothing to do */
    r = Read(z->fh, z->in + z->ilen, CZ_IBUF - z->ilen);
    if (r < 0) { err = "read error"; return -1; }
    if (!r) z->ieof = 1;
    z->ilen += r;
    return r;
}

int cz_avail(CzIn *z, LONG n)
{
    while (z->ilen - z->ipos < n && !z->ieof)
        if (cz_more(z) < 0) return -1;
    return 0;
}

int cz_put(BPTR fh, const void *buf, LONG n)
{
    if (n > 0 && Write(fh, (APTR)buf, n) != n) { err = "write error"; return -1; }
    return 0;
}

/* the kind from its magic bytes */
static int sniff(const UBYTE *b, LONG n)
{
    if (n >= 2 && b[0] == 0x1f && b[1] == 0x8b) return CZ_GZIP;
    if (n >= 4 && b[0] == 'B' && b[1] == 'Z' && b[2] == 'h' && b[3] >= '1' && b[3] <= '9') return CZ_BZIP2;
    if (n >= 6 && b[0] == 0xfd && b[1] == '7' && b[2] == 'z' && b[3] == 'X' && b[4] == 'Z' && b[5] == 0) return CZ_XZ;
    if (n >= 4 && b[0] == 0x28 && b[1] == 0xb5 && b[2] == 0x2f && b[3] == 0xfd) return CZ_ZSTD;
    if (n >= 4 && (b[0] & 0xf0) == 0x50 && b[1] == 0x2a && b[2] == 0x4d && b[3] == 0x18) return CZ_ZSTD;   /* a skippable frame first */
    return CZ_NONE;
}

static const char *const notkind[] = { "", "not gzip", "not bzip2", "not xz", "not zstd" };

CzIn *cz_open(BPTR fh, int kind, int *found)
{
    CzIn *z;
    int k, i;
    if (found) *found = CZ_NONE;
    if (kind < CZ_AUTO || kind > CZ_ZSTD) { err = "unknown kind"; return NULL; }
    if (!(z = cu_alloc(sizeof *z + CZ_IBUF))) { err = "out of memory"; return NULL; }
    memset(z, 0, sizeof *z);
    z->fh = fh;
    z->in = (UBYTE *)(z + 1);
    if (kind != CZ_NONE && cz_avail(z, 6) < 0) goto fail;
    k = kind == CZ_NONE ? CZ_NONE : sniff(z->in, z->ilen);
    if (found) *found = k;
    if (kind != CZ_AUTO && kind != k) { err = notkind[kind]; goto fail; }
    z->kind = k;
    if (k == CZ_NONE) return z;
    for (i = 0; cz_kinds[i]; i++)
        if (cz_kinds[i]->kind == k) break;
    if (!cz_kinds[i]) { err = "that kind is not in this build"; goto fail; }
    z->k = cz_kinds[i];
    if (z->k->open(z) < 0) goto fail;
    return z;
fail:
    cu_free(z);
    return NULL;
}

LONG cz_read(CzIn *z, void *buf, LONG n)
{
    LONG r;
    if (z->state) return z->state < 0 ? -1 : 0;
    if (n <= 0) return 0;
    if (!z->k) {
        if (z->ipos < z->ilen) {             /* what the sniffing read */
            r = z->ilen - z->ipos;
            if (r > n) r = n;
            memcpy(buf, z->in + z->ipos, r);
            z->ipos += r;
            return r;
        }
        if ((r = Read(z->fh, buf, n)) < 0) { err = "read error"; z->state = -1; }
        else if (!r) z->state = 1;
        return r;
    }
    r = z->k->read(z, buf, n);
    if (r < 0) z->state = -1;
    else if (!r) z->state = 1;
    return r;
}

void cz_close(CzIn *z)
{
    if (!z) return;
    if (z->k) z->k->close(z);
    cu_free(z);
}

int cz_trailing(CzIn *z) { return z->trailing; }

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
