/* diff - the lines that differ between two files (or two trees).
 *
 *   diff [-uqsiwbBNra] [-U N] [--color=WHEN] FROM TO
 *
 * Normal output as Unix diff ("3c3", "< old", "---", "> new"), or with -u
 * the unified form patch and git use:
 *     --- a.txt   2026-10-14 18:36:41
 *     +++ b.txt   2026-10-14 18:40:02
 *     @@ -1,4 +1,4 @@
 *      context
 *     -old line
 *     +new line
 * In a window the lines are coloured as git does (- red, + green, @@
 * cyan); --color=always (or --colour) also into a file or pipe, never
 * not at all. - is the input. A drawer and a file: the file of the same
 * name in that drawer. A missing new line at the end of a file is said
 * ("\ No newline at end of file"). Files with a 0 byte in their first 8 KB
 * are binary: only "Binary files A and B differ" (unless -a).
 *   -u        unified, 3 lines of context   -U N  with N lines of context
 *   -q        only whether they differ ("Files A and B differ")
 *   -s        say so when they are the same ("Files A and B are identical")
 *   -i        upper and lower case alike     -w  all white space ignored
 *   -b        changes in the amount of white space ignored (and at line ends)
 *   -B        changes that are only empty lines ignored (with -b or -w
 *             also lines of white space)
 *   -a        every file is text
 *   -r        two drawers: compared all the way down, "Only in D: name" for
 *             what one has and the other has not (links to drawers are
 *             not gone into); without -r, files in them only
 *   -N        with -r: a file only on one side is compared with an empty one
 * The algorithm is Myers' O(ND) with the linear space split, the lines
 * hashed first and lines found in only one file set aside (as GNU diff
 * does), so 10000-line files take a few hundred KB; a very different
 * pair is cut short with GNU's heuristic instead of taking minutes.
 * Measured in FS-UAE's A1200 (68020), 2 x 10000 lines: equal 0.6 s, 200
 * changes 4 s, all lines different 7 s; the worst kind, random lines
 * from five values (every line matches thousands), 53 s.
 *
 * Return code 0 the same, 5 different (WARN: on the Amiga 5 is the
 * "not quite" code, as grep's no match; Unix says 1), 10 trouble (a file
 * that could not be read, bad arguments), 20 Ctrl-C. */
#include <stdlib.h>
#include "cu.h"

const char *cu_name = "diff";
static const char version[] = "$VER: diff 0.1b1 (14.10.2026)";

static int o_uni, o_brief, o_same, o_icase, o_allws, o_ws, o_blank, o_text, o_rec, o_new, colour;
static LONG o_ctx = 3;
static char flagtext[40];        /* the flags as given, for "diff -r a/x b/x" */
static int differ;               /* something differed */

#define C_BOLD "\x1b[1m"
#define C_RED  "\x1b[31m"
#define C_GRN  "\x1b[32m"
#define C_CYAN "\x1b[36m"
#define C_OFF  "\x1b[0m"

/* ---- a file in memory ---- */

typedef struct { char *s; LONG len; } Line;

typedef struct {
    const char *name;
    char *data;
    LONG size, n;
    Line *ln;
    LONG *cls;                   /* each line's class: equal lines, equal numbers */
    char *chg;                   /* changed[n], with a 0 before and after */
    int nonl, binary;
    struct DateStamp date;
} File;

static void unload(File *f)
{
    cu_free(f->data);
    cu_free(f->ln);
    cu_free(f->cls);
    if (f->chg) cu_free(f->chg - 1);
    memset(f, 0, sizeof *f);
}

/* all of it into memory, split into lines; empty when path is NULL (-N) */
static int load(File *f, const char *path, const char *name)
{
    BPTR fh;
    LONG cap = 16384, n;
    int own = 1;
    memset(f, 0, sizeof *f);
    f->name = name;
    if (!path) {
        f->date.ds_Days = -2922;                 /* 1970-01-01: "did not exist", as GNU */
    } else {
        if (path[0] == '-' && !path[1]) { fh = Input(); own = 0; DateStamp(&f->date); }
        else {
            BPTR l = Lock((STRPTR)path, SHARED_LOCK);
            struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
            if (l && fib && Examine(l, fib)) {
                f->date = fib->fib_Date;
                if (fib->fib_Size > 0) cap = fib->fib_Size + 1;
            }
            if (fib) FreeDosObject(DOS_FIB, fib);
            if (l) UnLock(l);
            if (!(fh = Open((STRPTR)path, MODE_OLDFILE))) { cu_fault(10, path); return 0; }
        }
        if (!(f->data = cu_alloc(cap))) goto nomem;
        for (;;) {
            if (f->size == cap) {
                char *t = cu_alloc(cap * 2);
                if (!t) goto nomem;
                memcpy(t, f->data, f->size);
                cu_free(f->data);
                f->data = t; cap *= 2;
            }
            n = Read(fh, f->data + f->size, cap - f->size);
            if (n < 0) { cu_fault(10, path); if (own) Close(fh); return 0; }
            if (!n) break;
            f->size += n;
            if (cu_break()) { if (own) Close(fh); return 0; }
        }
        if (own) Close(fh);
    }
    if (!o_text) {
        LONG lim = f->size < 8192 ? f->size : 8192;
        f->binary = lim && memchr(f->data, 0, lim) != NULL;
    }
    return 1;
nomem:
    if (own && path && fh) Close(fh);
    cu_err(20, NULL, "out of memory");
    return 0;
}

/* the lines (only when the bytes differ: equal files are never split) */
static int split(File *f)
{
    LONG i, k = 0, n;
    char *p = f->data, *e = f->data + f->size;
    while (p < e && (p = memchr(p, '\n', e - p))) k++, p++;
    if (f->size && f->data[f->size - 1] != '\n') k++, f->nonl = 1;
    if (!(f->ln = cu_alloc((k + 1) * sizeof *f->ln)) || !(f->chg = cu_alloc(k + 2))) {
        cu_err(20, NULL, "out of memory");
        return 0;
    }
    memset(f->chg, 0, k + 2);
    f->chg++;
    for (i = 0, n = 0; n < k; n++) {
        char *s = f->data + i, *nl = memchr(s, '\n', f->size - i);
        LONG len = nl ? nl - s : f->size - i;
        f->ln[n].s = s; f->ln[n].len = len;
        i += len + 1;
    }
    f->n = k;
    return 1;
}

/* ---- lines compared as the flags say ---- */

static int lower(int c) { return (c >= 'A' && c <= 'Z') || (c >= 0xc0 && c <= 0xde && c != 0xd7) ? c + 32 : c; }
static int isws(int c) { return c == ' ' || c == '\t' || c == '\r' || c == '\f' || c == '\v'; }

/* the next character as compared, -1 at the end */
static int nextc(const Line *l, LONG *p)
{
    int c;
    for (;;) {
        if (*p >= l->len) return -1;
        c = (UBYTE)l->s[(*p)++];
        if (isws(c)) {
            if (o_allws) continue;
            if (o_ws) {
                while (*p < l->len && isws((UBYTE)l->s[*p])) (*p)++;
                if (*p >= l->len) return -1;
                return ' ';
            }
        }
        return o_icase ? lower(c) : c;
    }
}

static int plaincmp;             /* no -i -w -b: bytes as they are */

static ULONG hashline(const Line *l)
{
    ULONG h = 5381;
    if (plaincmp) {
        const UBYTE *s = (const UBYTE *)l->s, *e = s + l->len;
        while (s < e) h = (h << 5) + h + *s++;
    } else {
        LONG p = 0;
        int c;
        while ((c = nextc(l, &p)) >= 0) h = (h << 5) + h + c;
    }
    return h;
}

static int sameline(const Line *a, const Line *b)
{
    LONG p = 0, q = 0;
    int c;
    if (plaincmp) return a->len == b->len && !memcmp(a->s, b->s, a->len);
    do {
        c = nextc(a, &p);
        if (c != nextc(b, &q)) return 0;
    } while (c >= 0);
    return 1;
}

static int blankline(const Line *l)   /* empty (as GNU: white space only counts with -b or -w) */
{
    LONG p = 0;
    return nextc(l, &p) < 0;
}

/* ---- classes: equal lines get equal numbers ---- */

typedef struct { ULONG h; File *f; LONG i; } Cls;

static int classify(File *a, File *b, LONG *ncls)
{
    LONG size = 1024, total = a->n + b->n, *slot, nc = 0, i;
    Cls *c;
    File *f;
    int side;
    while (size < total + total / 2) size <<= 1;
    if (!(slot = cu_alloc(size * sizeof *slot)) || !(c = cu_alloc((total + 1) * sizeof *c))) {
        cu_free(slot);
        return 0;
    }
    memset(slot, 0xff, size * sizeof *slot);
    for (side = 0; side < 2; side++) {
        f = side ? b : a;
        if (!(f->cls = cu_alloc((f->n + 1) * sizeof *f->cls))) { cu_free(slot); cu_free(c); return 0; }
        for (i = 0; i < f->n; i++) {
            ULONG h = hashline(&f->ln[i]);
            int last = i == f->n - 1 && f->nonl;
            LONG k;
            if (last) h ^= 0x5bd1e995;           /* "x" and "x\n" differ */
            for (k = h & (size - 1); slot[k] >= 0; k = (k + 1) & (size - 1)) {
                Cls *e = &c[slot[k]];
                if (e->h == h && sameline(&e->f->ln[e->i], &f->ln[i])
                    && (e->i == e->f->n - 1 && e->f->nonl) == last) break;
            }
            if (slot[k] < 0) { slot[k] = nc; c[nc].h = h; c[nc].f = f; c[nc].i = i; nc++; }
            f->cls[i] = slot[k];
            if (!(i & 1023) && cu_break()) { cu_free(slot); cu_free(c); return 0; }
        }
    }
    cu_free(slot);
    cu_free(c);
    *ncls = nc;
    return 1;
}

/* ---- Myers: the shortest edit script, in linear space ---- */

static LONG *xv, *yv, *fd, *bd;  /* the sequences (classes); the diagonals' furthest points */
static char *xc, *yc;            /* their changed flags */
static LONG too_expensive;

typedef struct { LONG xoff, xlim, yoff, ylim; } Box;

/* the middle snake of the box: a point (*xm, *ym) on an optimal path */
static void midsnake(LONG xoff, LONG xlim, LONG yoff, LONG ylim, LONG *xm, LONG *ym)
{
    LONG dmin = xoff - ylim, dmax = xlim - yoff, fmid = xoff - yoff, bmid = xlim - ylim;
    LONG fmin = fmid, fmax = fmid, bmin = bmid, bmax = bmid, c, d, x, y, tlo, thi;
    int odd = (fmid - bmid) & 1;
    fd[fmid] = xoff;
    bd[bmid] = xlim;
    for (c = 1;; c++) {
        if (fmin > dmin) fd[--fmin - 1] = -1; else fmin++;
        if (fmax < dmax) fd[++fmax + 1] = -1; else fmax--;
        for (d = fmax; d >= fmin; d -= 2) {
            tlo = fd[d - 1]; thi = fd[d + 1];
            x = tlo >= thi ? tlo + 1 : thi;
            y = x - d;
            while (x < xlim && y < ylim && xv[x] == yv[y]) x++, y++;
            fd[d] = x;
            if (odd && bmin <= d && d <= bmax && bd[d] <= x) { *xm = x; *ym = y; return; }
        }
        if (bmin > dmin) bd[--bmin - 1] = 0x7fffffff; else bmin++;
        if (bmax < dmax) bd[++bmax + 1] = 0x7fffffff; else bmax--;
        for (d = bmax; d >= bmin; d -= 2) {
            tlo = bd[d - 1]; thi = bd[d + 1];
            x = tlo < thi ? tlo : thi - 1;
            y = x - d;
            while (x > xoff && y > yoff && xv[x - 1] == yv[y - 1]) x--, y--;
            bd[d] = x;
            if (!odd && fmin <= d && d <= fmax && x <= fd[d]) { *xm = x; *ym = y; return; }
        }
        if (c >= too_expensive) {        /* GNU's heuristic: the furthest reaching point so far */
            LONG fxybest = -1, fxbest = 0, bxybest = 0x7fffffff, bxbest = 0;
            for (d = fmax; d >= fmin; d -= 2) {
                x = fd[d] < xlim ? fd[d] : xlim;
                y = x - d;
                if (ylim < y) x = ylim + d, y = ylim;
                if (fxybest < x + y) fxybest = x + y, fxbest = x;
            }
            for (d = bmax; d >= bmin; d -= 2) {
                x = bd[d] > xoff ? bd[d] : xoff;
                y = x - d;
                if (y < yoff) x = yoff + d, y = yoff;
                if (x + y < bxybest) bxybest = x + y, bxbest = x;
            }
            if ((xlim + ylim) - bxybest < fxybest - (xoff + yoff)) { *xm = fxbest; *ym = fxybest - fxbest; }
            else { *xm = bxbest; *ym = bxybest - bxbest; }
            return;
        }
    }
}

/* the whole compare, boxes on a list on the heap (the stack is small) */
static int compare(LONG nx, LONG ny)
{
    Box *st;
    LONG cap = 64, sp = 0, diags = nx + ny + 3, k;
    if (!(fd = cu_alloc(2 * diags * sizeof *fd)) || !(st = cu_alloc(cap * sizeof *st))) return 0;
    bd = fd + diags;
    fd += ny + 1;                /* diagonals from -(ny+1) to nx+1 */
    bd += ny + 1;
    too_expensive = 1;
    for (k = diags; k; k >>= 2) too_expensive <<= 1;
    if (too_expensive < 256) too_expensive = 256;
    st[sp].xoff = 0; st[sp].xlim = nx; st[sp].yoff = 0; st[sp].ylim = ny; sp++;
    while (sp) {
        Box b = st[--sp];
        LONG xm, ym;
        while (b.xoff < b.xlim && b.yoff < b.ylim && xv[b.xoff] == yv[b.yoff]) b.xoff++, b.yoff++;
        while (b.xlim > b.xoff && b.ylim > b.yoff && xv[b.xlim - 1] == yv[b.ylim - 1]) b.xlim--, b.ylim--;
        if (b.xoff == b.xlim) { while (b.yoff < b.ylim) yc[b.yoff++] = 1; continue; }
        if (b.yoff == b.ylim) { while (b.xoff < b.xlim) xc[b.xoff++] = 1; continue; }
        midsnake(b.xoff, b.xlim, b.yoff, b.ylim, &xm, &ym);
        if ((xm == b.xoff && ym == b.yoff) || (xm == b.xlim && ym == b.ylim)) {   /* no progress: all changed */
            while (b.xoff < b.xlim) xc[b.xoff++] = 1;
            while (b.yoff < b.ylim) yc[b.yoff++] = 1;
            continue;
        }
        if (sp + 2 > cap) {
            Box *t = cu_alloc(cap * 2 * sizeof *t);
            if (!t) { cu_free(st); cu_free(fd - (ny + 1)); return 0; }
            memcpy(t, st, sp * sizeof *t);
            cu_free(st);
            st = t; cap *= 2;
        }
        st[sp].xoff = xm; st[sp].xlim = b.xlim; st[sp].yoff = ym; st[sp].ylim = b.ylim; sp++;
        st[sp].xoff = b.xoff; st[sp].xlim = xm; st[sp].yoff = b.yoff; st[sp].ylim = ym; sp++;
        if (cu_break()) break;
    }
    cu_free(st);
    cu_free(fd - (ny + 1));
    return cu_rc < 20;
}

/* GNU's shift_boundaries: runs of changes slid over equal lines, merged
 * where they can be, and lined up with the other file's changes */
static void shift(File *f, File *o)
{
    char *changed = f->chg;
    const char *other = o->chg;
    const LONG *eq = f->cls;
    LONG i = 0, j = 0, end = f->n, runlength, start, corresponding;
    for (;;) {
        while (i < end && !changed[i]) {
            while (other[j++]) ;
            i++;
        }
        if (i == end) break;
        start = i;
        while (changed[++i]) ;
        while (other[j]) j++;
        do {
            runlength = i - start;
            while (start && eq[start - 1] == eq[i - 1]) {
                changed[--start] = 1;
                changed[--i] = 0;
                while (changed[start - 1]) start--;
                while (other[--j]) ;
            }
            corresponding = other[j - 1] ? i : end;
            while (i != end && eq[start] == eq[i]) {
                changed[start++] = 0;
                changed[i++] = 1;
                while (changed[i]) i++;
                while (other[++j]) corresponding = i;
            }
        } while (runlength != i - start);
        while (corresponding < i) {
            changed[--start] = 1;
            changed[--i] = 0;
            while (other[--j]) ;
        }
    }
}

/* both files' changed flags: lines found in only one file are changes
 * without asking; Myers on the rest */
static int analyse(File *a, File *b)
{
    LONG ncls, *cnt, i, nx = 0, ny = 0, *mx, *my;
    int ok = 0;
    if (!classify(a, b, &ncls)) return 0;
    if (!(cnt = cu_alloc((ncls + 1) * 2 * sizeof *cnt))) return 0;
    memset(cnt, 0, (ncls + 1) * 2 * sizeof *cnt);
    for (i = 0; i < a->n; i++) cnt[2 * a->cls[i]]++;
    for (i = 0; i < b->n; i++) cnt[2 * b->cls[i] + 1]++;
    xv = cu_alloc((a->n + 1) * sizeof *xv); yv = cu_alloc((b->n + 1) * sizeof *yv);
    mx = cu_alloc((a->n + 1) * sizeof *mx); my = cu_alloc((b->n + 1) * sizeof *my);
    xc = cu_alloc(a->n + 1); yc = cu_alloc(b->n + 1);
    if (xv && yv && mx && my && xc && yc) {
        for (i = 0; i < a->n; i++)
            if (cnt[2 * a->cls[i] + 1]) { mx[nx] = i; xv[nx++] = a->cls[i]; } else a->chg[i] = 1;
        for (i = 0; i < b->n; i++)
            if (cnt[2 * b->cls[i]]) { my[ny] = i; yv[ny++] = b->cls[i]; } else b->chg[i] = 1;
        memset(xc, 0, nx + 1); memset(yc, 0, ny + 1);
        if (compare(nx, ny)) {
            for (i = 0; i < nx; i++) if (xc[i]) a->chg[mx[i]] = 1;
            for (i = 0; i < ny; i++) if (yc[i]) b->chg[my[i]] = 1;
            shift(a, b);
            shift(b, a);
            ok = 1;
        }
    } else cu_err(20, NULL, "out of memory");
    cu_free(cnt); cu_free(xv); cu_free(yv); cu_free(mx); cu_free(my); cu_free(xc); cu_free(yc);
    xv = yv = NULL; xc = yc = NULL;
    return ok;
}

/* ---- the changes, and printing them ---- */

typedef struct { LONG a0, a1, b0, b1; int ignore; } Change;

static Change *changes;
static LONG nchanges;

static int collect(File *a, File *b)
{
    LONG i = 0, j = 0, cap = 0;
    nchanges = 0;
    while (i < a->n || j < b->n) {
        Change *c;
        if (i < a->n && j < b->n && !a->chg[i] && !b->chg[j]) { i++; j++; continue; }
        if (nchanges == cap) {
            Change *t = cu_alloc((cap = cap ? cap * 2 : 64) * sizeof *t);
            if (!t) { cu_err(20, NULL, "out of memory"); return 0; }
            if (changes) { memcpy(t, changes, nchanges * sizeof *t); cu_free(changes); }
            changes = t;
        }
        c = &changes[nchanges++];
        c->a0 = i; c->b0 = j;
        while (i < a->n && a->chg[i]) i++;
        while (j < b->n && b->chg[j]) j++;
        c->a1 = i; c->b1 = j;
        c->ignore = 0;
        if (o_blank) {
            LONG k;
            c->ignore = 1;
            for (k = c->a0; k < c->a1 && c->ignore; k++) if (!blankline(&a->ln[k])) c->ignore = 0;
            for (k = c->b0; k < c->b1 && c->ignore; k++) if (!blankline(&b->ln[k])) c->ignore = 0;
        }
        if (c->a0 == c->a1 && c->b0 == c->b1) { nchanges--; break; }   /* cannot happen; never loop */
    }
    return 1;
}

static void range(LONG s, LONG e, int sep)    /* lines s+1..e: "4" or "4,6" */
{
    cu_putnum(s + 1 <= e ? s + 1 : e, 0);
    if (e > s + 1) { cu_putc(sep); cu_putnum(e, 0); }
}

static void line(File *f, LONG i, const char *mark, const char *col)
{
    if (colour && col) cu_puts(col);
    cu_puts(mark);
    cu_put(f->ln[i].s, f->ln[i].len);
    if (colour && col) cu_puts(C_OFF);
    cu_putc('\n');
    if (i == f->n - 1 && f->nonl) cu_puts("\\ No newline at end of file\n");
}

static void normal(File *a, File *b)
{
    LONG k, i;
    for (k = 0; k < nchanges && cu_rc < 20; k++) {
        Change *c = &changes[k];
        if (c->ignore) continue;
        if (colour) cu_puts(C_CYAN);
        if (c->b0 == c->b1) { range(c->a0, c->a1, ','); cu_putc('d'); cu_putnum(c->b0, 0); }
        else if (c->a0 == c->a1) { cu_putnum(c->a0, 0); cu_putc('a'); range(c->b0, c->b1, ','); }
        else { range(c->a0, c->a1, ','); cu_putc('c'); range(c->b0, c->b1, ','); }
        if (colour) cu_puts(C_OFF);
        cu_putc('\n');
        for (i = c->a0; i < c->a1; i++) line(a, i, "< ", C_RED);
        if (c->a0 < c->a1 && c->b0 < c->b1) cu_puts("---\n");
        for (i = c->b0; i < c->b1; i++) line(b, i, "> ", C_GRN);
        if (cu_break()) break;
    }
}

static void stamp(const char *mark, File *f)
{
    char d[20];
    if (colour) cu_puts(C_BOLD);
    cu_puts(mark);
    cu_puts(f->name);
    cu_putc('\t');
    cu_isodate(&f->date, d);
    cu_puts(d);
    if (colour) cu_puts(C_OFF);
    cu_putc('\n');
}

static void unified(File *a, File *b)
{
    LONG k = 0, e, i, j, as, ae, bs, be;
    stamp("--- ", a);
    stamp("+++ ", b);
    while (k < nchanges && cu_rc < 20) {
        int show = 0;
        /* the changes of one hunk: those less than 2N lines apart */
        for (e = k; e + 1 < nchanges && changes[e + 1].a0 - changes[e].a1 <= 2 * o_ctx; e++) ;
        for (i = k; i <= e; i++) if (!changes[i].ignore) show = 1;
        if (!show) { k = e + 1; continue; }
        as = changes[k].a0 - o_ctx; if (as < 0) as = 0;
        bs = changes[k].b0 - (changes[k].a0 - as);
        ae = changes[e].a1 + o_ctx; if (ae > a->n) ae = a->n;
        be = changes[e].b1 + (ae - changes[e].a1);
        if (colour) cu_puts(C_CYAN);
        cu_puts("@@ -");
        cu_putnum(ae - as ? as + 1 : as, 0); if (ae - as != 1) { cu_putc(','); cu_putnum(ae - as, 0); }
        cu_puts(" +");
        cu_putnum(be - bs ? bs + 1 : bs, 0); if (be - bs != 1) { cu_putc(','); cu_putnum(be - bs, 0); }
        cu_puts(" @@");
        if (colour) cu_puts(C_OFF);
        cu_putc('\n');
        i = as;
        for (; k <= e; k++) {
            Change *c = &changes[k];
            for (; i < c->a0; i++) line(a, i, " ", NULL);
            for (i = c->a0; i < c->a1; i++) line(a, i, "-", C_RED);
            for (j = c->b0; j < c->b1; j++) line(b, j, "+", C_GRN);
        }
        for (; i < ae; i++) line(a, i, " ", NULL);
        if (cu_break()) break;
    }
}

/* ---- two files ---- */

static void says(const char *a, const char *mid, const char *b, const char *end)
{
    cu_puts("Files "); cu_puts(a); cu_puts(mid); cu_puts(b); cu_puts(end);
}

/* pa, pb: the paths (NULL: missing, as empty with -N); header: -r's line */
static void difffiles(const char *pa, const char *pb, const char *na, const char *nb, int header)
{
    File a, b;
    LONG k;
    int any = 0;
    if (!load(&a, pa, na)) return;
    if (!load(&b, pb, nb)) { unload(&a); return; }
    if (a.binary || b.binary) {
        if (a.size != b.size || memcmp(a.data, b.data, a.size)) {
            differ = 1;
            if (o_brief) says(na, " and ", nb, " differ\n");
            else { cu_puts("Binary files "); cu_puts(na); cu_puts(" and "); cu_puts(nb); cu_puts(" differ\n"); }
        } else if (o_same) says(na, " and ", nb, " are identical\n");
        goto done;
    }
    plaincmp = !o_icase && !o_allws && !o_ws;
    if (a.size == b.size && !memcmp(a.data, b.data, a.size)) {   /* the common case, at once */
        if (o_same) says(na, " and ", nb, " are identical\n");
        goto done;
    }
    if (o_brief && plaincmp && !o_blank) {                      /* -q: the bytes say it */
        differ = 1;
        says(na, " and ", nb, " differ\n");
        goto done;
    }
    if (!split(&a) || !split(&b) || !analyse(&a, &b) || !collect(&a, &b)) goto done;
    for (k = 0; k < nchanges; k++) if (!changes[k].ignore) any = 1;
    if (!any) { if (o_same) says(na, " and ", nb, " are identical\n"); goto done; }
    differ = 1;
    if (o_brief) { says(na, " and ", nb, " differ\n"); goto done; }
    if (header) {
        if (colour) cu_puts(C_BOLD);
        cu_puts("diff "); cu_puts(flagtext); cu_puts(na); cu_putc(' '); cu_puts(nb);
        if (colour) cu_puts(C_OFF);
        cu_putc('\n');
    }
    if (o_uni) unified(&a, &b); else normal(&a, &b);
done:
    cu_free(changes); changes = NULL; nchanges = 0;
    unload(&a);
    unload(&b);
    cu_flush();
}

/* ---- trees ---- */

typedef struct { char *name; LONG ta, tb; } Ent;   /* type 0: not on that side */

typedef struct Frame {
    struct Frame *up;
    char *pa, *pb;
    Ent *e;
    LONG n, i;
    CuDirEnt *la, *lb;           /* the scans, freed with the frame */
} Frame;

static int icmp(const char *a, const char *b)
{
    for (;; a++, b++) {
        int x = lower((UBYTE)*a), y = lower((UBYTE)*b);
        if (x != y || !x) return x - y;
    }
}

static int cmpent(const void *x, const void *y)
{
    const Ent *a = x, *b = y;
    int r = cu_natcmp(a->name, b->name);
    return r ? r : icmp(a->name, b->name);
}

static LONG kind(LONG t)         /* 1 a drawer to go into, -1 anything else */
{
    return CU_ISDIR(t) ? 1 : -1;
}

/* a link to a drawer is not gone into: a soft link is asked what it is */
static LONG realtype(const char *dir, CuDirEnt *e)
{
    char p[CU_PATH];
    LONG t = e->type;
    if (t == ST_LINKDIR) return 0;
    if (t == ST_SOFTLINK && cu_join(p, dir, e->name)) {
        BPTR l = Lock(p, SHARED_LOCK);
        struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
        t = 0;
        if (l && fib && Examine(l, fib) && fib->fib_DirEntryType < 0) t = fib->fib_DirEntryType;
        if (fib) FreeDosObject(DOS_FIB, fib);
        if (l) UnLock(l);
    }
    return t;
}

static Frame *newframe(const char *pa, const char *pb)
{
    Frame *f = cu_alloc(sizeof *f);
    CuDirEnt *d;
    LONG n = 0, i, k;
    int ok1, ok2;
    if (!f) { cu_err(20, NULL, "out of memory"); return NULL; }
    memset(f, 0, sizeof *f);
    f->pa = cu_strdup(pa); f->pb = cu_strdup(pb);
    f->la = cu_scan(pa, &ok1);
    f->lb = cu_scan(pb, &ok2);
    for (d = f->la; d; d = d->next) n++;
    for (d = f->lb; d; d = d->next) n++;
    if (!f->pa || !f->pb || !(f->e = cu_alloc((n + 1) * sizeof *f->e))) { cu_err(20, NULL, "out of memory"); return f; }
    for (d = f->la; d; d = d->next) { f->e[f->n].name = d->name; f->e[f->n].ta = realtype(pa, d); f->e[f->n].tb = 0; if (f->e[f->n].ta) f->n++; }
    for (d = f->lb; d; d = d->next) { f->e[f->n].name = d->name; f->e[f->n].ta = 0; f->e[f->n].tb = realtype(pb, d); if (f->e[f->n].tb) f->n++; }
    if (f->n > 1) qsort(f->e, f->n, sizeof *f->e, cmpent);
    for (i = k = 0; i < f->n; i++) {          /* the same name on both sides: one entry */
        if (k && !icmp(f->e[k - 1].name, f->e[i].name) && (!f->e[k - 1].ta != !f->e[i].ta)) {
            if (f->e[i].ta) f->e[k - 1].ta = f->e[i].ta; else f->e[k - 1].tb = f->e[i].tb;
            continue;
        }
        f->e[k++] = f->e[i];
    }
    f->n = k;
    return f;
}

static void freeframe(Frame *f)
{
    cu_freeents(f->la);
    cu_freeents(f->lb);
    cu_free(f->e);
    cu_free(f->pa);
    cu_free(f->pb);
    cu_free(f);
}

static void onlyin(const char *dir, const char *name)
{
    cu_puts("Only in ");
    cu_puts(dir);
    cu_puts(": ");
    cu_puts(name);
    cu_putc('\n');
    differ = 1;
}

static void difftrees(const char *pa, const char *pb)
{
    Frame *top = newframe(pa, pb), *f;
    static char a[CU_PATH], b[CU_PATH];
    while ((f = top) && cu_rc < 20) {
        Ent *e;
        if (f->i >= f->n) { top = f->up; freeframe(f); continue; }
        e = &f->e[f->i++];
        if (!cu_join(a, f->pa, e->name) || !cu_join(b, f->pb, e->name)) { cu_err(10, e->name, "path too long"); continue; }
        if (!e->ta || !e->tb) {
            if (o_new && kind(e->ta ? e->ta : e->tb) < 0) difffiles(e->ta ? a : NULL, e->tb ? b : NULL, a, b, 1);
            else onlyin(e->ta ? f->pa : f->pb, e->name);
        } else if (kind(e->ta) > 0 && kind(e->tb) > 0) {
            if (o_rec) {
                Frame *n = newframe(a, b);
                if (n) { n->up = top; top = n; }
            } else {
                cu_puts("Common subdirectories: "); cu_puts(a); cu_puts(" and "); cu_puts(b); cu_putc('\n');
            }
        } else if (kind(e->ta) != kind(e->tb)) {
            cu_puts("File "); cu_puts(a); cu_puts(kind(e->ta) > 0 ? " is a drawer while file " : " is a file while file ");
            cu_puts(b); cu_puts(kind(e->tb) > 0 ? " is a drawer\n" : " is a file\n");
            differ = 1;
        } else difffiles(a, b, a, b, 1);
        cu_flush();
        if (cu_break()) break;
    }
    while (top) { f = top->up; freeframe(top); top = f; }
}

static LONG typeof_(const char *p)    /* 1 drawer, -1 file, 0 not there (said) */
{
    BPTR l;
    struct FileInfoBlock *fib;
    LONG t = 0;
    if (p[0] == '-' && !p[1]) return -1;
    if (!(l = Lock((STRPTR)p, SHARED_LOCK))) { cu_fault(10, p); return 0; }
    if ((fib = AllocDosObject(DOS_FIB, NULL)) && Examine(l, fib)) t = fib->fib_DirEntryType > 0 ? 1 : -1;
    else cu_fault(10, p);
    if (fib) FreeDosObject(DOS_FIB, fib);
    UnLock(l);
    return t;
}

int main(void)
{
    char *v, *pa, *pb, joined[CU_PATH];
    int c, i, col = -1, ok;
    LONG ta, tb;
    (void)version;
    if (cu_args("diff [-uqsiwbBNra] [-U N] [--color=WHEN] FROM TO") < 0) return cu_rc;
    for (i = 0; i < cu_argc; i++) {           /* --color, before the short flags */
        v = cu_argv[i];
        if (!strcmp(v, "--")) break;
        if (!strncmp(v, "--colour", 8) || !strncmp(v, "--color", 7)) {
            char *e = strchr(v, '=');
            col = !e || !strcmp(e + 1, "always") ? 1 : !strcmp(e + 1, "never") ? 0 : -1;
            memmove(&cu_argv[i], &cu_argv[i + 1], (cu_argc - i) * sizeof *cu_argv);
            cu_argc--; i--;
        } else if (v[0] == '-' && v[1] && v[1] != '-' && strlen(flagtext) + strlen(v) + 2 < sizeof flagtext) {
            strcat(flagtext, v);
            strcat(flagtext, " ");
            if (v[strlen(v) - 1] == 'U' && i + 1 < cu_argc && strlen(flagtext) + strlen(cu_argv[i + 1]) + 2 < sizeof flagtext) {
                strcat(flagtext, cu_argv[++i]);    /* -U 5 */
                strcat(flagtext, " ");
            }
        }
    }
    while ((c = cu_opt("uU:qsiwbBNra", &v)) >= 0) switch (c) {
        case 'u': o_uni = 1; break;
        case 'U':
            o_ctx = cu_num(v, &ok);
            if (!ok || o_ctx < 0) { cu_err(10, v, "not a count"); return cu_rc; }
            o_uni = 1;
            break;
        case 'q': o_brief = 1; break;
        case 's': o_same = 1; break;
        case 'i': o_icase = 1; break;
        case 'w': o_allws = 1; break;
        case 'b': o_ws = 1; break;
        case 'B': o_blank = 1; break;
        case 'N': o_new = 1; break;
        case 'r': o_rec = 1; break;
        case 'a': o_text = 1; break;
        default: return cu_rc;
    }
    if (cu_argc != 2) { cu_err(10, NULL, cu_argc < 2 ? "two files to compare (diff ? for usage)" : "only two files at a time"); return cu_rc; }
    colour = col >= 0 ? col : IsInteractive(Output());
    pa = cu_argv[0]; pb = cu_argv[1];
    if (!(ta = typeof_(pa)) || !(tb = typeof_(pb))) return cu_rc;
    if (ta > 0 && tb > 0) difftrees(pa, pb);
    else {
        if (ta > 0 || tb > 0) {                  /* a file and a drawer: the file of that name in it */
            const char *file = ta > 0 ? pb : pa, *dir = ta > 0 ? pa : pb;
            if (file[0] == '-' && !file[1]) { cu_err(10, dir, "is a drawer, and - has no name"); return cu_rc; }
            if (!cu_join(joined, dir, (const char *)FilePart((STRPTR)file))) { cu_err(10, dir, "path too long"); return cu_rc; }
            if (ta > 0) pa = joined; else pb = joined;
            if (!typeof_(joined)) return cu_rc;
        }
        difffiles(pa, pb, pa, pb, 0);
    }
    cu_flush();
    if (differ && cu_rc < 5) cu_rc = 5;
    return cu_rc;
}
