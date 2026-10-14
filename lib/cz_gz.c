/* cz_gz.c - gzip read (RFC 1952) with miniz's inflate (tinfl): members
 * one after another (a multi-member .gz, or .gz files joined with cat),
 * the header's FEXTRA, FNAME, FCOMMENT and FHCRC (checked), the CRC-32
 * and length of each member checked. Fixed memory: tinfl's state and its
 * 32 KB window, whatever the file's size. */
#include "cz_int.h"
#include "miniz.h"

#define DICT TINFL_LZ_DICT_SIZE

enum { P_DATA, P_TRAILER, P_NEXT, P_END };

typedef struct {
    tinfl_decompressor r;
    UBYTE dict[DICT];
    ULONG t[256];                /* the CRC table */
    LONG dofs;                   /* where tinfl writes next in dict */
    LONG opos, oavail;           /* made, not given yet */
    ULONG crc, size;             /* of this member so far */
    int phase;
    UBYTE back[4];               /* bytes tinfl read ahead, given back */
    int nback, bpos;
} Gz;

/* the next byte, 0..255; -1 the end, -2 trouble */
static int gbyte(CzIn *z, Gz *g)
{
    if (g->bpos < g->nback) return g->back[g->bpos++];
    if (z->ipos == z->ilen) {
        LONG r = cz_more(z);
        if (r < 0) return -2;
        if (z->ipos == z->ilen) return -1;
    }
    return z->in[z->ipos++];
}

/* one member's header, after its first two bytes were seen */
static int header(CzIn *z, Gz *g)
{
    UBYTE h[10];
    ULONG hcrc;
    int i, c, flg;
    for (i = 0; i < 10; i++) {
        if ((c = gbyte(z, g)) < 0) goto eof;
        h[i] = c;
    }
    if (h[0] != 0x1f || h[1] != 0x8b) { cz_seterr("not gzip"); return -1; }
    if (h[2] != 8) { cz_seterr("unsupported: not deflate"); return -1; }
    flg = h[3];
    if (flg & 0xe0) { cz_seterr("unsupported: unknown header flags"); return -1; }
    hcrc = cz_crc32(g->t, 0, h, 10);
    if (flg & 4) {                       /* FEXTRA */
        UBYTE x[2];
        LONG len;
        for (i = 0; i < 2; i++) {
            if ((c = gbyte(z, g)) < 0) goto eof;
            x[i] = c;
        }
        hcrc = cz_crc32(g->t, hcrc, x, 2);
        for (len = x[0] | x[1] << 8; len > 0; len--) {
            UBYTE b;
            if ((c = gbyte(z, g)) < 0) goto eof;
            b = c;
            hcrc = cz_crc32(g->t, hcrc, &b, 1);
        }
    }
    for (i = 8; i <= 16; i += 8)         /* FNAME, FCOMMENT: zero-ended */
        if (flg & i) {
            UBYTE b;
            do {
                if ((c = gbyte(z, g)) < 0) goto eof;
                b = c;
                hcrc = cz_crc32(g->t, hcrc, &b, 1);
            } while (c);
        }
    if (flg & 2) {                       /* FHCRC: the low 16 bits of the CRC-32 so far */
        int lo, hi;
        if ((lo = gbyte(z, g)) < 0 || (hi = gbyte(z, g)) < 0) { c = lo < 0 ? lo : hi; goto eof; }
        if ((ULONG)(lo | hi << 8) != (hcrc & 0xffff)) { cz_seterr("damaged: header CRC wrong"); return -1; }
    }
    tinfl_init(&g->r);
    g->dofs = g->opos = g->oavail = 0;
    g->crc = g->size = 0;
    g->nback = g->bpos = 0;
    g->phase = P_DATA;
    return 0;
eof:
    cz_seterr(c == -2 ? "read error" : "truncated");
    return -1;
}

static int gz_open(CzIn *z)
{
    Gz *g = cu_alloc(sizeof *g);
    if (!g) { cz_seterr("out of memory"); return -1; }
    memset(g, 0, sizeof *g);
    cz_crc_init(g->t);
    z->p = g;
    if (header(z, g) < 0) { cu_free(g); z->p = NULL; return -1; }
    return 0;
}

/* the member's 8-byte end: CRC-32 and length (mod 2^32), low bytes first */
static int trailer(CzIn *z, Gz *g)
{
    UBYTE t[8];
    int i, c;
    for (i = 0; i < 8; i++) {
        if ((c = gbyte(z, g)) < 0) { cz_seterr(c == -2 ? "read error" : "truncated"); return -1; }
        t[i] = c;
    }
    if ((t[0] | t[1] << 8 | t[2] << 16 | (ULONG)t[3] << 24) != g->crc) { cz_seterr("damaged: CRC wrong"); return -1; }
    if ((t[4] | t[5] << 8 | t[6] << 16 | (ULONG)t[7] << 24) != g->size) { cz_seterr("damaged: length wrong"); return -1; }
    g->phase = P_NEXT;
    return 0;
}

/* after a member: another one, the end, or bytes that are not gzip */
static int next(CzIn *z, Gz *g)
{
    if (g->bpos < g->nback) { z->trailing = 1; g->phase = P_END; return 0; }
    if (cz_avail(z, 2) < 0) return -1;
    if (z->ipos == z->ilen) { g->phase = P_END; return 0; }
    if (z->ilen - z->ipos >= 2 && z->in[z->ipos] == 0x1f && z->in[z->ipos + 1] == 0x8b)
        return header(z, g);
    z->trailing = 1;
    g->phase = P_END;
    return 0;
}

static LONG gz_read(CzIn *z, UBYTE *buf, LONG n)
{
    Gz *g = z->p;
    LONG done = 0;
    for (;;) {
        if (g->oavail) {
            LONG k = g->oavail < n - done ? g->oavail : n - done;
            memcpy(buf + done, g->dict + g->opos, k);
            g->opos += k; g->oavail -= k; done += k;
            if (done == n) return done;
            continue;
        }
        if (g->phase == P_TRAILER) {
            if (trailer(z, g) < 0) return -1;
            continue;
        }
        if (g->phase == P_NEXT) {
            if (next(z, g) < 0) return -1;
            continue;
        }
        if (g->phase == P_END) return done;
        {
            size_t insz, outsz;
            tinfl_status st;
            if (z->ipos == z->ilen && cz_more(z) < 0) return -1;
            insz = z->ilen - z->ipos;
            outsz = DICT - g->dofs;
            st = tinfl_decompress(&g->r, z->in + z->ipos, &insz, g->dict, g->dict + g->dofs, &outsz,
                                  z->ieof ? 0 : TINFL_FLAG_HAS_MORE_INPUT);
            z->ipos += insz;
            if (outsz) {
                g->crc = cz_crc32(g->t, g->crc, g->dict + g->dofs, outsz);
                g->size += outsz;
                g->opos = g->dofs; g->oavail = outsz;
                g->dofs = (g->dofs + outsz) & (DICT - 1);
            }
            if (st == TINFL_STATUS_DONE) {
                /* whole bytes still in tinfl's bit buffer belong to the trailer */
                ULONG bb = g->r.m_bit_buf;
                int nb = g->r.m_num_bits >> 3;
                g->nback = g->bpos = 0;
                while (nb-- > 0 && g->nback < 4) { g->back[g->nback++] = bb & 0xff; bb >>= 8; }
                g->phase = P_TRAILER;
            } else if (st == TINFL_STATUS_FAILED_CANNOT_MAKE_PROGRESS) {
                cz_seterr("truncated"); return -1;
            } else if (st < 0) {
                cz_seterr("damaged: bad deflate data"); return -1;
            }
        }
    }
}

static void gz_close(CzIn *z) { cu_free(z->p); }

const CzKind cz_gzip = { CZ_GZIP, gz_open, gz_read, gz_close };
