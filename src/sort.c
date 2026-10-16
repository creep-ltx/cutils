/* sort - lines in order.
 *
 *   sort [-bdfhiMnrsuVcC] [-k POS1[,POS2]]... [-t SEP] [-o FILE] [file | pattern | - ...]
 *   sort FROM [TO] [COLSTART n] [CASE] [NUMERIC] [LEXICOGRAPHIC] [DESCENDING] [SKIPBLANKS]
 *
 * All the files (no file, or -: the input) are read, sorted together and
 * written out. Plain order is by byte (as GNU sort in the C locale).
 *   -n numeric (-12, 3.5; a line without a number is 0)   -h human sizes
 *   (2K before 1M)   -V version / natural order (file9 before file10)
 *   -M month names (JAN..DEC)   -g as -n   -f fold case (Latin-1 too)
 *   -d only blanks, letters and digits count   -i only printable ones
 *   -b leading blanks do not count   -r reverse   -u one of each run of
 *   equal lines (equal as the keys see them)   -s stable: equal lines stay
 *   in their input order (else, as GNU, whole lines decide in the end)
 *   -k POS1[,POS2] a key: fields F[.C] (from 1), flags after it (-k2n,
 *   -k1.3,1.5r, -k3,3fb); several -k: the first decides, ties go on to
 *   the next; a key with no flags takes the global ones   -t SEP fields
 *   split at SEP (\t a tab), else at the start of each run of blanks
 *   -o FILE the output to FILE, written after all is read (it may be one
 *   of the inputs)   -c check the order: the first line out of it is
 *   told, return code 5   -C the same, silently   -S, -T accepted, unused.
 * The sort is a merge sort (stable, bottom-up); lines live in big chunks,
 * so a few MB sort in seconds. A 0 byte ends a line's text.
 *
 * It replaces C:Sort, so it also takes Amiga Sort's template:
 *   FROM,TO,COLSTART/K,CASE/S,NUMERIC/S,LEXICOGRAPHIC/S,DESCENDING/S,
 *   SKIPBLANKS/S,BUFSIZE/K/N
 * That way it is read when any of those keywords is there (case blind,
 * KEY=value as well), or when there are exactly two names and no -flags
 * (`sort a b` sorts a into b, as C:Sort did; one name is the Unix way).
 * Then, as the real Sort 46.4 (checked side by side in tests/cases):
 * FROM sorted into TO (written, nothing shown; no TO: shown), case blind
 * unless CASE, equal lines in input order; COLSTART n compares from
 * column n (1 = the start); SKIPBLANKS skips blanks there first; NUMERIC:
 * the number at the start (blanks, -, digits), lines without one first;
 * LEXICOGRAPHIC: a dictionary order (accents and case only break ties,
 * CASE ignored); DESCENDING: the whole order turned round; the end of a
 * line sorts as a newline would (so "ab<tab>" comes before "ab"). BUFSIZE
 * is ignored (memory grows as needed). Unlike the real one, empty input
 * is not remarked on, and numbers longer than 32 bits still compare right.
 *
 * Return code 0, 5 a file could not be read or -c found disorder, 10 bad
 * arguments or the output could not be written, 20 out of memory or
 * Ctrl-C. */
#include "cu.h"

const char *cu_name = "sort";
static const char version[] = "$VER: sort 0.1b2 (16.10.2026)";

#define USAGE "sort [-bdfhiMnrsuVcC] [-k POS1[,POS2]] [-t SEP] [-o FILE] [file ...] | FROM [TO] [COLSTART n] [CASE] [NUMERIC] [LEXICOGRAPHIC] [DESCENDING] [SKIPBLANKS]"

#define F_B   1      /* blanks at the start of the key skipped */
#define F_BE  2      /* blanks skipped before the end position's characters */
#define F_F   4      /* fold case */
#define F_D   8      /* dictionary */
#define F_I   16     /* printable only */
#define F_N   32
#define F_H   64
#define F_V   128
#define F_M   256
#define F_R   512

typedef struct { LONG sf, sc, ef, ec; int fl; } Key;   /* ef < 0: to the end of the line */

#define MAXKEYS 16
static Key keys[MAXKEYS];
static int nkeys, gfl, stable, unique, check, quiet;
static int tab = -1;                 /* the -t separator, -1: blanks */
static CuLines lines;
static unsigned char upt[256], ctyp[256];
#define T_ALNUM 1
#define T_BLANK 2
#define T_PRINT 4

static void tables(void)
{
    int c;
    for (c = 0; c < 256; c++) {
        int u = c;
        if ((c >= 'a' && c <= 'z') || (c >= 0xe0 && c <= 0xfe && c != 0xf7)) u = c - 32;
        upt[c] = u;
        if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= 0xc0 && c != 0xd7 && c != 0xf7)) ctyp[c] |= T_ALNUM;
        if (c == ' ' || c == '\t') ctyp[c] |= T_BLANK;
        if ((c >= 0x20 && c < 0x7f) || c >= 0xa0) ctyp[c] |= T_PRINT;
    }
}

#define BLANK(c) (ctyp[(unsigned char)(c)] & T_BLANK)
#define DIGIT(c) ((c) >= '0' && (c) <= '9')

/* ---- comparing parts of lines ---- */

typedef const unsigned char UC;

static int bytecmp(UC *a, UC *b)        /* whole lines, unsigned */
{
    while (*a && *a == *b) a++, b++;
    return (int)*a - (int)*b;
}

static int txtcmp(UC *a, UC *ae, UC *b, UC *be, int fl)
{
    if (!(fl & (F_F | F_D | F_I))) {
        for (; a < ae && b < be; a++, b++) if (*a != *b) return (int)*a - (int)*b;
    } else {
        int skip = fl & F_D ? T_ALNUM | T_BLANK : fl & F_I ? T_PRINT : 0;
        for (;;) {
            int ca, cb;
            if (skip) {
                while (a < ae && !(ctyp[*a] & skip)) a++;
                while (b < be && !(ctyp[*b] & skip)) b++;
            }
            if (a >= ae || b >= be) break;
            ca = fl & F_F ? upt[*a] : *a;
            cb = fl & F_F ? upt[*b] : *b;
            if (ca != cb) return ca - cb;
            a++, b++;
        }
    }
    return (a < ae) - (b < be);
}

/* a number as -n reads it: blanks, -, digits, .digits */
typedef struct { int sign; UC *i, *f, *end; LONG il, fl; } Num;

static void getnum(UC *s, UC *e, Num *n, int frac)
{
    int neg = 0, nz = 0;
    while (s < e && BLANK(*s)) s++;
    if (s < e && *s == '-') neg = 1, s++;
    while (s < e && *s == '0') s++;
    n->i = s;
    while (s < e && DIGIT(*s)) s++;
    n->il = s - n->i;
    if (n->il) nz = 1;
    n->f = s; n->fl = 0;
    if (frac && s < e && *s == '.') {
        n->f = ++s;
        while (s < e && DIGIT(*s)) { if (*s != '0') nz = 1; s++; }
        n->fl = s - n->f;
    }
    n->end = s;
    n->sign = nz ? (neg ? -1 : 1) : 0;
}

static int magcmp(Num *a, Num *b)       /* the sizes, signs aside */
{
    LONG k;
    int r;
    if (a->il != b->il) return a->il < b->il ? -1 : 1;
    for (k = 0; k < a->il; k++) if (a->i[k] != b->i[k]) return a->i[k] < b->i[k] ? -1 : 1;
    for (k = 0; k < a->fl || k < b->fl; k++) {
        int ca = k < a->fl ? a->f[k] : '0', cb = k < b->fl ? b->f[k] : '0';
        if (ca != cb) return ca < cb ? -1 : 1;
    }
    r = 0;
    return r;
}

static int numcmp(UC *a, UC *ae, UC *b, UC *be)
{
    Num x, y;
    getnum(a, ae, &x, 1); getnum(b, be, &y, 1);
    if (x.sign != y.sign) return x.sign < y.sign ? -1 : 1;
    if (!x.sign) return 0;
    return x.sign * magcmp(&x, &y);
}

static int suffix(Num *n, UC *e)
{
    static const char sfx[] = "KMGTPEZY";
    const char *p;
    int c;
    if (n->end >= e) return 0;
    c = *n->end == 'k' ? 'K' : *n->end;
    p = c ? strchr(sfx, c) : NULL;
    return p ? p - sfx + 1 : 0;
}

static int humcmp(UC *a, UC *ae, UC *b, UC *be)
{
    Num x, y;
    int sx, sy;
    getnum(a, ae, &x, 1); getnum(b, be, &y, 1);
    if (x.sign != y.sign) return x.sign < y.sign ? -1 : 1;
    if (!x.sign) return 0;
    sx = suffix(&x, ae); sy = suffix(&y, be);
    if (sx != sy) return x.sign * (sx < sy ? -1 : 1);
    return x.sign * magcmp(&x, &y);
}

static int month(UC *s, UC *e)
{
    static const char m[] = "JANFEBMARAPRMAYJUNJULAUGSEPOCTNOVDEC";
    int i;
    while (s < e && BLANK(*s)) s++;
    if (e - s < 3) return 0;
    for (i = 0; i < 12; i++)
        if (upt[s[0]] == m[3 * i] && upt[s[1]] == m[3 * i + 1] && upt[s[2]] == m[3 * i + 2]) return i + 1;
    return 0;
}

/* -V: GNU's version order (filevercmp): digits as numbers, letters
 * before other characters, ~ before all, suffixes like .txt .tar.gz
 * only deciding a tie */
static int vorder(int c)
{
    if (DIGIT(c)) return 0;
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) return c;
    if (c == '~') return -1;
    return c ? c + 256 : 0;
}

static int verrev(UC *s, LONG sl, UC *t, LONG tl)
{
    LONG si = 0, ti = 0;
    while (si < sl || ti < tl) {
        int fd = 0;
        while ((si < sl && !DIGIT(s[si])) || (ti < tl && !DIGIT(t[ti]))) {
            int so = si == sl ? 0 : vorder(s[si]), to = ti == tl ? 0 : vorder(t[ti]);
            if (so != to) return so - to;
            si++, ti++;
        }
        while (si < sl && s[si] == '0') si++;
        while (ti < tl && t[ti] == '0') ti++;
        while (si < sl && DIGIT(s[si]) && ti < tl && DIGIT(t[ti])) {
            if (!fd) fd = s[si] - t[ti];
            si++, ti++;
        }
        if (si < sl && DIGIT(s[si])) return 1;
        if (ti < tl && DIGIT(t[ti])) return -1;
        if (fd) return fd;
    }
    return 0;
}

#define ALPHA(c) (((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' && (c) <= 'z'))

static LONG vprefix(UC *s, LONG n)       /* the length without suffixes */
{
    LONG i = 0, p = 0;
    for (;;) {
        if (i >= n) return p;
        i++;
        p = i;
        while (i + 1 < n && s[i] == '.' && (ALPHA(s[i + 1]) || s[i + 1] == '~'))
            for (i += 2; i < n && (ALPHA(s[i]) || DIGIT(s[i]) || s[i] == '~'); i++) ;
    }
}

static int vercmp(UC *a, UC *ae, UC *b, UC *be)
{
    LONG al = ae - a, bl = be - b, ap, bp;
    int r;
    if (!al || !bl) return (al > 0) - (bl > 0);
    if (a[0] == '.' && b[0] != '.') return -1;
    if (b[0] == '.' && a[0] != '.') return 1;
    ap = vprefix(a, al); bp = vprefix(b, bl);
    r = verrev(a, ap, b, bp);
    if (r || (ap == al && bp == bl)) return r;
    return verrev(a, al, b, bl);
}

/* ---- keys: where a key starts and ends in a line (as GNU sort finds them) ---- */

static UC *keybeg(UC *p, UC *lim, Key *k)
{
    LONG f = k->sf;
    if (tab >= 0)
        while (p < lim && f--) { while (p < lim && *p != tab) p++; if (p < lim) p++; }
    else
        while (p < lim && f--) { while (p < lim && BLANK(*p)) p++; while (p < lim && !BLANK(*p)) p++; }
    if (k->fl & F_B) while (p < lim && BLANK(*p)) p++;
    return p + k->sc < lim ? p + k->sc : lim;
}

static UC *keyend(UC *p, UC *lim, Key *k)
{
    LONG f = k->ef, c = k->ec;
    if (f < 0) return lim;
    if (!c) f++;                        /* all of the end field */
    if (tab >= 0)
        while (p < lim && f--) { while (p < lim && *p != tab) p++; if (p < lim && (f || c)) p++; }
    else
        while (p < lim && f--) { while (p < lim && BLANK(*p)) p++; while (p < lim && !BLANK(*p)) p++; }
    if (c) {
        if (k->fl & F_BE) while (p < lim && BLANK(*p)) p++;
        p = p + c < lim ? p + c : lim;
    }
    return p;
}

/* each line's key positions, found once: h[2k] start, h[2k+1] end */
static int hdrn;                       /* LONGs before each line's text */
#define HDR(s) ((LONG *)(s) - hdrn)

static void prep(UC *s, LONG n, LONG *h)
{
    int i;
    for (i = 0; i < nkeys; i++) {
        UC *b = keybeg(s, s + n, &keys[i]), *e = keyend(s, s + n, &keys[i]);
        h[2 * i] = b - s;
        h[2 * i + 1] = (e < b ? b : e) - s;
    }
}

static int keycmp(UC *a, LONG *ha, UC *b, LONG *hb)
{
    int i, r;
    for (i = 0; i < nkeys; i++) {
        Key *k = &keys[i];
        UC *as = a + ha[2 * i], *ae = a + ha[2 * i + 1];
        UC *bs = b + hb[2 * i], *be = b + hb[2 * i + 1];
        if (k->fl & F_N) r = numcmp(as, ae, bs, be);
        else if (k->fl & F_H) r = humcmp(as, ae, bs, be);
        else if (k->fl & F_M) r = month(as, ae) - month(bs, be);
        else if (k->fl & F_V) r = vercmp(as, ae, bs, be);
        else r = txtcmp(as, ae, bs, be, k->fl);
        if (r) return k->fl & F_R ? -r : r;
    }
    return 0;
}

/* the order: the keys, then (unless -s or -u) the whole lines */
static int lastly(const char *a, const char *b, int r)
{
    if (nkeys && (r || stable || unique)) return r;
    r = bytecmp((UC *)a, (UC *)b);
    return gfl & F_R ? -r : r;
}

static int ucmp(const char *a, const char *b)
{
    return lastly(a, b, nkeys ? keycmp((UC *)a, HDR(a), (UC *)b, HDR(b)) : 0);
}

/* no keys: the first 4 bytes (0s after a short line) settle most of it */
static int pcmp(const char *a, const char *b)
{
    ULONG x = *HDR(a), y = *HDR(b);
    if (x != y) return x < y ? -1 : 1;
    if (!(x & 0xff) || !(x & 0xff00) || !(x & 0xff0000) || !(x & 0xff000000)) return 0;
    return bytecmp((UC *)a + 4, (UC *)b + 4);
}

static int rpcmp(const char *a, const char *b) { return pcmp(b, a); }

/* ---- Amiga Sort ---- */

static LONG acol;
static int acase, anum, alex, adesc, askip;
static unsigned char lext[256];

static void lextable(void)
{
    static const char acc[] = "AAAAAAACEEEEIIIIDNOOOOO*OUUUUY";   /* 0xC0..0xDD */
    int c;
    for (c = 0; c < 256; c++) {
        int u = upt[c];
        if (u >= 0xc0 && u <= 0xdd && u != 0xd7) u = acc[u - 0xc0];
        if (c == 0xff) u = 'Y';
        lext[c] = u;
    }
}

/* where the compare starts; NULL: the line ends before COLSTART (such
 * lines come first, as with the real Sort; a line ending just there
 * compares as a newline) */
static UC *astart(UC *s)
{
    LONG k = acol;
    while (k > 0 && *s) s++, k--;
    if (k) return NULL;
    if (askip) while (BLANK(*s)) s++;
    return s;
}

/* bytes through a table; the end of a line counts as a newline */
static int tabcmp(UC *a, UC *b, const unsigned char *t)
{
    for (;;) {
        int ca = *a ? (t ? t[*a] : *a) : '\n', cb = *b ? (t ? t[*b] : *b) : '\n';
        if (ca != cb) return ca - cb;
        if (!*a) return 0;
        a++, b++;
    }
}

static int acmp(const char *x, const char *y)
{
    UC *a = astart((UC *)x), *b = astart((UC *)y);
    if (anum) {                          /* a short line: no number */
        Num p, q;
        int hp, hq;
        if (!a) a = (UC *)"";
        if (!b) b = (UC *)"";
        getnum(a, a + strlen((char *)a), &p, 0); getnum(b, b + strlen((char *)b), &q, 0);
        hp = p.end > a && DIGIT(p.end[-1]);        /* a number at all */
        hq = q.end > b && DIGIT(q.end[-1]);
        if (hp != hq) return hp - hq;
        if (!hp || p.sign != q.sign) return p.sign - q.sign;
        return p.sign * magcmp(&p, &q);
    }
    if (!a || !b) return (a != NULL) - (b != NULL);
    if (alex) {
        int r = tabcmp(a, b, lext);
        return r ? r : tabcmp(a, b, NULL);
    }
    return tabcmp(a, b, acase ? NULL : upt);
}

static const char *const akw[] = { "FROM", "TO", "COLSTART", "BUFSIZE", "CASE", "NUMERIC", "LEXICOGRAPHIC", "DESCENDING", "SKIPBLANKS", NULL };

/* which keyword w is (KEY or KEY=value: *val), else -1 */
static int keyword(const char *w, const char **val)
{
    int i;
    for (i = 0; akw[i]; i++) {
        int n = strlen(akw[i]);
        int j;
        for (j = 0; j < n && upt[(unsigned char)w[j]] == (unsigned char)akw[i][j]; j++) ;
        if (j == n && (!w[n] || w[n] == '=')) {
            *val = w[n] ? w + n + 1 : NULL;
            return i;
        }
    }
    return -1;
}

static int amigaway(void)
{
    int i, names = 0, flags = 0;
    const char *v;
    for (i = 0; i < cu_argc; i++) {
        if (keyword(cu_argv[i], &v) >= 0) return 1;
        if (cu_argv[i][0] == '-' && cu_argv[i][1]) flags++;
        else names++;
    }
    return names == 2 && !flags;
}

static void putlines(BPTR to, int rev)
{
    LONG i;
    if (to) cu_out = to;
    for (i = 0; i < lines.n && cu_rc < 20; i++) {
        cu_puts(lines.v[rev ? lines.n - 1 - i : i]);
        cu_putc('\n');
        if (!(i & 1023) && cu_break()) break;
    }
    cu_flush();
    cu_out = Output();
}

static int readall(const char *path, void *u)
{
    CuIn in;
    char *line, *t;
    LONG n, k = 0;
    int nl;
    (void)u;
    if (!cu_open(&in, path)) return 0;
    while ((n = cu_line(&in, &line, &nl)) >= 0) {
        if (!(t = cu_lines_add(&lines, line, n, hdrn * sizeof(LONG)))) { cu_err(20, NULL, "out of memory"); break; }
        if (nkeys) prep((UC *)t, n, HDR(t));
        else if (hdrn) {
            UC *q = (UC *)t;
            ULONG x = 0;
            int j;
            for (j = 0; j < 4; j++) { x = x << 8 | *q; if (*q) q++; }
            *HDR(t) = x;
        }
        if (!(++k & 1023) && cu_break()) break;
    }
    if (in.err) cu_fault(5, path);
    cu_close(&in);
    return cu_rc >= 20;
}

static BPTR openout(const char *path)
{
    BPTR fh = Open((STRPTR)path, MODE_NEWFILE);
    if (!fh) cu_fault(10, path);
    return fh;
}

static int amigasort(void)
{
    const char *from = NULL, *to = NULL, *v;
    int i, k;
    BPTR fh = 0;
    for (i = 0; i < cu_argc; i++) {
        k = keyword(cu_argv[i], &v);
        if (k < 0) {
            if (!from) from = cu_argv[i];
            else if (!to) to = cu_argv[i];
            else { cu_err(10, NULL, "wrong number of arguments"); return cu_rc; }
            continue;
        }
        if (k <= 3 && !v) {
            if (i + 1 >= cu_argc) { cu_err(10, akw[k], "needs a value"); return cu_rc; }
            v = cu_argv[++i];
        }
        switch (k) {
            case 0: from = v; break;
            case 1: to = v; break;
            case 2: { long n = 0; while (DIGIT(*v)) n = n * 10 + *v++ - '0'; acol = n > 1 ? n - 1 : 0; } break;
            case 3: break;
            case 4: acase = 1; break;
            case 5: anum = 1; break;
            case 6: alex = 1; break;
            case 7: adesc = 1; break;
            case 8: askip = 1; break;
        }
    }
    if (!from) { cu_err(10, NULL, "required argument missing"); return cu_rc; }
    lextable();
    readall(from, NULL);
    if (cu_rc) { if (cu_rc < 10) cu_rc = 10; return cu_rc; }
    if (!cu_msort(lines.v, lines.n, acmp)) { cu_err(20, NULL, "out of memory"); return cu_rc; }
    if (cu_rc >= 20) return cu_rc;
    if (to && !(fh = openout(to))) return cu_rc;
    putlines(fh, adesc);
    if (fh) Close(fh);
    return cu_rc;
}

/* ---- the Unix way ---- */

static int parsepos(const char **p, LONG *f, LONG *c, int *fl, int end)
{
    const char *s = *p;
    LONG n = 0;
    if (!DIGIT(*s)) return 0;
    while (DIGIT(*s)) n = n * 10 + *s++ - '0';
    if (n < 1) return 0;
    *f = n - 1;
    if (*s == '.') {
        s++;
        if (!DIGIT(*s)) return 0;
        n = 0;
        while (DIGIT(*s)) n = n * 10 + *s++ - '0';
        if (!end && n < 1) return 0;
        *c = end ? n : n - 1;
    }
    for (;; s++) {
        switch (*s) {
            case 'b': *fl |= end ? F_BE : F_B; continue;
            case 'd': *fl |= F_D; continue;
            case 'f': *fl |= F_F; continue;
            case 'i': *fl |= F_I; continue;
            case 'n': case 'g': *fl |= F_N; continue;
            case 'h': *fl |= F_H; continue;
            case 'M': *fl |= F_M; continue;
            case 'V': *fl |= F_V; continue;
            case 'r': *fl |= F_R; continue;
        }
        break;
    }
    *p = s;
    return 1;
}

static int addkey(const char *s)
{
    Key *k = &keys[nkeys];
    const char *p = s;
    if (nkeys == MAXKEYS) { cu_err(10, s, "too many keys"); return 0; }
    memset(k, 0, sizeof *k);
    k->ef = -1;
    if (!parsepos(&p, &k->sf, &k->sc, &k->fl, 0)) goto bad;
    if (*p == ',') {
        p++;
        k->ec = 0;
        if (!parsepos(&p, &k->ef, &k->ec, &k->fl, 1)) goto bad;
    }
    if (*p) goto bad;
    nkeys++;
    return 1;
bad:
    cu_err(10, s, "not a key (F[.C][flags][,F[.C][flags]])");
    return 0;
}

/* -c: the input read through, each line against the one before */
static char *prev;
static LONG prevsize, hprev[2 * MAXKEYS], hcur[2 * MAXKEYS];
static int haveprev, disorder;

static int checkone(const char *path, void *u)
{
    CuIn in;
    char *line;
    LONG n, ln = 0;
    int nl;
    (void)u;
    if (!cu_open(&in, path)) return 0;
    while (!disorder && (n = cu_line(&in, &line, &nl)) >= 0) {
        int r = 0;
        ln++;
        if (nkeys) prep((UC *)line, n, hcur);
        if (haveprev) r = lastly(prev, line, nkeys ? keycmp((UC *)prev, hprev, (UC *)line, hcur) : 0);
        if (haveprev && (r > 0 || (unique && !r))) {
            if (!quiet) {
                char what[CU_PATH + 16], num[12], *t = num + sizeof num - 1;
                char *why = cu_alloc(n + 12);
                LONG v = ln;
                *t = 0;
                do *--t = '0' + v % 10; while (v /= 10);
                strcpy(what, path ? path : "-");
                strcat(what, ":"); strcat(what, t);
                if (why) { strcpy(why, "disorder: "); strcat(why, line); }
                cu_err(5, what, why ? why : "disorder");
                cu_free(why);
            } else if (cu_rc < 5) cu_rc = 5;
            disorder = 1;
            break;
        }
        if (n + 1 > prevsize) {
            cu_free(prev);
            prevsize = n + 256;
            if (!(prev = cu_alloc(prevsize))) { cu_err(20, NULL, "out of memory"); break; }
        }
        memcpy(prev, line, n + 1);
        memcpy(hprev, hcur, sizeof hcur);
        haveprev = 1;
        if (!(ln & 1023) && cu_break()) break;
    }
    if (in.err) cu_fault(5, path);
    cu_close(&in);
    return disorder || cu_rc >= 20;
}

int main(void)
{
    char *v, *out = NULL;
    int c, i;
    BPTR fh = 0;
    (void)version;
    if (cu_args(USAGE) < 0) return cu_rc;
    tables();
    if (amigaway()) return amigasort();
    while ((c = cu_opt("bdfgihMnrsuVcCk:t:o:S:T:", &v)) >= 0) switch (c) {
        case 'b': gfl |= F_B; break;
        case 'd': gfl |= F_D; break;
        case 'f': gfl |= F_F; break;
        case 'i': gfl |= F_I; break;
        case 'n': case 'g': gfl |= F_N; break;
        case 'h': gfl |= F_H; break;
        case 'M': gfl |= F_M; break;
        case 'V': gfl |= F_V; break;
        case 'r': gfl |= F_R; break;
        case 's': stable = 1; break;
        case 'u': unique = 1; break;
        case 'c': check = 1; break;
        case 'C': check = quiet = 1; break;
        case 'k': if (!addkey(v)) return cu_rc; break;
        case 't':
            if (v[0] == '\\' && v[1] == 't' && !v[2]) tab = '\t';
            else if (v[0] && !v[1]) tab = (unsigned char)v[0];
            else { cu_err(10, v, "the separator must be one character"); return cu_rc; }
            break;
        case 'o': out = v; break;
        case 'S': case 'T': break;
        default: return cu_rc;
    }
    for (i = 0; i < nkeys; i++)          /* a key without flags takes the global ones */
        if (!keys[i].fl) keys[i].fl = gfl | (gfl & F_B ? F_BE : 0);
    if (!nkeys && (gfl & ~F_R)) {        /* options but no key: the whole line is one */
        keys[0].sf = keys[0].sc = 0; keys[0].ef = -1; keys[0].ec = 0;
        keys[0].fl = gfl;
        nkeys = 1;
    }
    hdrn = nkeys ? 2 * nkeys : 1;
    if (check) {
        if (!cu_argc) checkone(NULL, NULL);
        for (i = 0; i < cu_argc && !disorder && cu_rc < 20; i++) cu_each(cu_argv[i], checkone, NULL);
        cu_flush();
        return cu_rc;
    }
    if (!cu_argc) readall(NULL, NULL);
    for (i = 0; i < cu_argc && cu_rc < 20; i++) cu_each(cu_argv[i], readall, NULL);
    if (cu_rc >= 20) return cu_rc;
    if (!cu_msort(lines.v, lines.n, nkeys ? ucmp : gfl & F_R ? rpcmp : pcmp)) { cu_err(20, NULL, "out of memory"); return cu_rc; }
    if (cu_rc >= 20) return cu_rc;
    if (unique && lines.n) {             /* the first of each run of equal lines */
        LONG k = 1;
        for (i = 1; i < lines.n; i++)
            if (ucmp(lines.v[k - 1], lines.v[i])) lines.v[k++] = lines.v[i];
        lines.n = k;
    }
    if (out && !(fh = openout(out))) return cu_rc;
    putlines(fh, 0);
    if (fh) Close(fh);
    return cu_rc;
}
