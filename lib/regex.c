/* regex.c - POSIX extended regular expressions (from CShell's [[ =~ ]]): the
 * parts scripts use - . [...] [^...] [[:alpha:]] ^ $ ( ) | * + ? {n,m},
 * \x for a plain x, and GNU's \w \W \s \S. Compiled to a small program and
 * run as a Pike VM (Russ Cox, "Regular Expression Matching: the Virtual
 * Machine Approach"): every thread steps through the text together, so no
 * pattern can make it slow (no backtracking blow-up). Leftmost match;
 * among those the first by the pattern's order (Perl's, not POSIX's
 * longest - the same for what scripts write). Compiled once, run per line. */
#include <string.h>
#include "cu.h"

enum { R_CHAR, R_ANY, R_CLASS, R_BOL, R_EOL, R_SPLIT, R_JMP, R_SAVE, R_MATCH };

typedef struct { unsigned char op, c; short x, y; } RInst;    /* x, y: jumps, relative */

#define RMAXI  400
#define RMAXC  24
#define RSLOTS (2 * RE_GROUPS)

typedef struct {
    const char *p;
    RInst code[RMAXI];
    int n, err, ngroup, nocase;
    unsigned char cls[RMAXC][32];
    int ncls;
    void *mark, *ta, *tb;               /* the machine's lists, made once by re_comp */
    int nslots;                         /* the capture slots in use (copied per thread) */
    int hasfirst;                       /* first: the bytes a match can start with */
    unsigned char first[32];
} RC;

static int lower(int c) { c &= 0xff; return (c >= 'A' && c <= 'Z') || (c >= 0xc0 && c <= 0xde && c != 0xd7) ? c + 32 : c; }

static int emit(RC *r, int op, int c, int x, int y)
{
    if (r->n >= RMAXI) { r->err = 1; return r->n; }
    r->code[r->n].op = op; r->code[r->n].c = c; r->code[r->n].x = x; r->code[r->n].y = y;
    return r->n++;
}

/* room for one instruction at at (the code after it moves up one) */
static int insert(RC *r, int at)
{
    if (r->n >= RMAXI) { r->err = 1; return 0; }
    memmove(r->code + at + 1, r->code + at, (r->n - at) * sizeof r->code[0]);
    r->n++;
    return 1;
}

static void setbit(unsigned char *s, int c) { s[(c & 0xff) >> 3] |= 1 << (c & 7); }

static int named(unsigned char *s, const char *name, int n)
{
    static const char *const names[] = { "alpha", "digit", "alnum", "upper", "lower", "space", "blank", "punct", "xdigit", "print", "graph", "cntrl", NULL };
    int k, c;
    for (k = 0; names[k]; k++) if ((int)strlen(names[k]) == n && !memcmp(names[k], name, n)) break;
    if (!names[k]) return 0;
    for (c = 0; c < 256; c++) {
        int al = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'), di = c >= '0' && c <= '9', ok = 0;
        switch (k) {
        case 0: ok = al; break;
        case 1: ok = di; break;
        case 2: ok = al || di; break;
        case 3: ok = c >= 'A' && c <= 'Z'; break;
        case 4: ok = c >= 'a' && c <= 'z'; break;
        case 5: ok = c == ' ' || (c >= 9 && c <= 13); break;
        case 6: ok = c == ' ' || c == '\t'; break;
        case 7: ok = c > 32 && c < 127 && !al && !di; break;
        case 8: ok = di || ((c | 32) >= 'a' && (c | 32) <= 'f'); break;
        case 9: ok = c >= 32 && c < 127; break;
        case 10: ok = c > 32 && c < 127; break;
        case 11: ok = c < 32 || c == 127; break;
        }
        if (ok) setbit(s, c);
    }
    return 1;
}

static int newclass(RC *r)
{
    if (r->ncls >= RMAXC) { r->err = 1; return 0; }
    memset(r->cls[r->ncls], 0, 32);
    return r->ncls++;
}

static void finishclass(RC *r, int k, int neg)
{
    int c;
    if (r->nocase)
        for (c = 0; c < 256; c++)
            if (r->cls[k][c >> 3] & (1 << (c & 7))) { setbit(r->cls[k], lower(c)); if (c >= 'a' && c <= 'z') setbit(r->cls[k], c - 32); }
    if (neg) for (c = 0; c < 32; c++) r->cls[k][c] ^= 0xff;
    emit(r, R_CLASS, 0, k, 0);
}

static void bracket(RC *r)
{
    int k = newclass(r), neg = 0, first = 1;
    if (r->err) return;
    if (*r->p == '^') { neg = 1; r->p++; }
    while (*r->p && (*r->p != ']' || first)) {
        int a = (unsigned char)*r->p, b;
        first = 0;
        if (a == '[' && r->p[1] == ':') {
            const char *e = strstr(r->p + 2, ":]");
            if (e && named(r->cls[k], r->p + 2, e - r->p - 2)) { r->p = e + 2; continue; }
        }
        r->p++;
        if (*r->p == '-' && r->p[1] && r->p[1] != ']') {
            b = (unsigned char)r->p[1];
            r->p += 2;
            for (; a <= b; a++) setbit(r->cls[k], a);
            continue;
        }
        setbit(r->cls[k], a);
    }
    if (*r->p != ']') { r->err = 1; return; }
    r->p++;
    finishclass(r, k, neg);
}

static void alt(RC *r);

static void atom(RC *r)
{
    char c = *r->p++;
    switch (c) {
    case '(': {
        int g = ++r->ngroup;
        if (g < RE_GROUPS) emit(r, R_SAVE, 0, 2 * g, 0);
        alt(r);
        if (*r->p != ')') { r->err = 1; return; }
        r->p++;
        if (g < RE_GROUPS) emit(r, R_SAVE, 0, 2 * g + 1, 0);
        return;
    }
    case '[': bracket(r); return;
    case '.': emit(r, R_ANY, 0, 0, 0); return;
    case '^': emit(r, R_BOL, 0, 0, 0); return;
    case '$': emit(r, R_EOL, 0, 0, 0); return;
    case '*': case '+': case '?': r->err = 1; return;
    case '\\':
        c = *r->p;
        if (!c) { r->err = 1; return; }
        r->p++;
        if (c == 'w' || c == 'W' || c == 's' || c == 'S') {
            int k = newclass(r);
            if (r->err) return;
            named(r->cls[k], (c | 32) == 'w' ? "alnum" : "space", 5);
            if ((c | 32) == 'w') setbit(r->cls[k], '_');
            finishclass(r, k, c == 'W' || c == 'S');
            return;
        }
        break;
    }
    emit(r, R_CHAR, r->nocase ? lower(c) : (unsigned char)c, 0, 0);
}

/* {n}, {n,}, {n,m}: -1 when it is not one (then { is a plain character) */
static int braces(const char *p, int *lo, int *hi)
{
    const char *q = p + 1;
    int a = 0, b, any = 0;
    while (*q >= '0' && *q <= '9') { a = a * 10 + *q++ - '0'; any = 1; }
    if (!any) return -1;
    b = a;
    if (*q == ',') {
        q++;
        if (*q >= '0' && *q <= '9') { b = 0; while (*q >= '0' && *q <= '9') b = b * 10 + *q++ - '0'; }
        else b = -1;
    }
    if (*q != '}' || (b >= 0 && b < a) || a > 100 || b > 100) return -1;
    *lo = a; *hi = b;
    return q + 1 - p;
}

static void repeat(RC *r)
{
    int s = r->n;
    atom(r);
    while (!r->err && (*r->p == '*' || *r->p == '+' || *r->p == '?' || *r->p == '{')) {
        int e = r->n, len = e - s, lo, hi, k, used;
        char q = *r->p;
        if (q == '{') {
            RInst t[RMAXI];
            if ((used = braces(r->p, &lo, &hi)) < 0) return;
            r->p += used;
            memcpy(t, r->code + s, len * sizeof t[0]);
            r->n = s;
            for (k = 0; k < lo; k++) { if (r->n + len > RMAXI) { r->err = 1; return; } memcpy(r->code + r->n, t, len * sizeof t[0]); r->n += len; }
            if (hi < 0) {                       /* the rest as x* */
                int at = emit(r, R_SPLIT, 0, 1, len + 2);
                if (r->n + len > RMAXI) { r->err = 1; return; }
                memcpy(r->code + r->n, t, len * sizeof t[0]); r->n += len;
                emit(r, R_JMP, 0, at - r->n, 0);
            } else for (k = lo; k < hi; k++) {  /* each one more optional */
                emit(r, R_SPLIT, 0, 1, len + 1);
                if (r->n + len > RMAXI) { r->err = 1; return; }
                memcpy(r->code + r->n, t, len * sizeof t[0]); r->n += len;
            }
            continue;
        }
        r->p++;
        if (q == '+') { int at = emit(r, R_SPLIT, 0, 0, 1); r->code[at].x = s - at; continue; }
        if (!insert(r, s)) return;
        r->code[s].op = R_SPLIT; r->code[s].x = 1;
        if (q == '?') { r->code[s].y = len + 1; continue; }
        {   /* * */
            int at = emit(r, R_JMP, 0, 0, 0);
            r->code[at].x = s - at;
            r->code[s].y = r->n - s;
        }
    }
}

static void alt(RC *r)
{
    int s = r->n;
    while (*r->p && *r->p != '|' && *r->p != ')' && !r->err) repeat(r);
    if (*r->p == '|' && !r->err) {
        int j, l2;
        r->p++;
        if (!insert(r, s)) return;
        r->code[s].op = R_SPLIT; r->code[s].x = 1;
        j = emit(r, R_JMP, 0, 0, 0);
        l2 = r->n;
        r->code[s].y = l2 - s;
        alt(r);
        r->code[j].x = r->n - j;
    }
}

/* ---- the machine ---- */

typedef struct { int pc; int caps[RSLOTS]; } Thr;
typedef struct { Thr *t; int n; } TList;
typedef struct { RC *r; const char *s; int len, gen; int *mark; } VM;

static void add(VM *v, TList *l, int pc, int *caps, int sp)
{
    RInst *in;
    if (v->mark[pc] == v->gen) return;
    v->mark[pc] = v->gen;
    in = &v->r->code[pc];
    switch (in->op) {
    case R_JMP: add(v, l, pc + in->x, caps, sp); return;
    case R_SPLIT: add(v, l, pc + in->x, caps, sp); add(v, l, pc + in->y, caps, sp); return;
    case R_SAVE: {
        int old = caps[in->x];
        caps[in->x] = sp;
        add(v, l, pc + 1, caps, sp);
        caps[in->x] = old;
        return;
    }
    case R_BOL: if (sp == 0) add(v, l, pc + 1, caps, sp); return;
    case R_EOL: if (sp == v->len) add(v, l, pc + 1, caps, sp); return;
    }
    l->t[l->n].pc = pc;
    memcpy(l->t[l->n].caps, caps, v->r->nslots * sizeof(int));
    l->n++;
}

/* The bytes a match can start with, into r->first: 0 when any can (., an
 * empty match), then there is no skipping ahead. */
static int firstset(RC *r, int pc, unsigned char *seen)
{
    RInst *in;
    int c;
    if (seen[pc]) return 1;
    seen[pc] = 1;
    in = &r->code[pc];
    switch (in->op) {
    case R_CHAR:
        setbit(r->first, in->c);
        c = in->c;
        if (r->nocase && ((c >= 'a' && c <= 'z') || (c >= 0xe0 && c <= 0xfe && c != 0xf7))) setbit(r->first, c - 32);
        return 1;
    case R_CLASS:
        for (c = 0; c < 32; c++) r->first[c] |= r->cls[in->x][c];
        return 1;
    case R_BOL: case R_SAVE: return firstset(r, pc + 1, seen);
    case R_JMP: return firstset(r, pc + in->x, seen);
    case R_SPLIT: return firstset(r, pc + in->x, seen) && firstset(r, pc + in->y, seen);
    }
    return 0;                                /* . $ or the end: anything */
}

/* Compiled once (grep runs it on every line): NULL when the pattern is bad. */
void *re_comp(const char *re, int nocase)
{
    RC *r = cu_alloc(sizeof *r);
    if (!r) return NULL;
    memset(r, 0, sizeof *r);
    r->p = re; r->nocase = nocase;
    emit(r, R_SAVE, 0, 0, 0);
    alt(r);
    if (*r->p) r->err = 1;                  /* a ) with no ( */
    emit(r, R_SAVE, 0, 1, 0);
    emit(r, R_MATCH, 0, 0, 0);
    if (r->err) { cu_free(r); return NULL; }
    r->mark = cu_alloc(r->n * sizeof(int));
    r->ta = cu_alloc(r->n * sizeof(Thr));
    r->tb = cu_alloc(r->n * sizeof(Thr));
    if (!r->mark || !r->ta || !r->tb) { re_free(r); return NULL; }
    r->nslots = 2 * ((r->ngroup < RE_GROUPS ? r->ngroup : RE_GROUPS - 1) + 1);
    memset(r->mark, 0, r->n * sizeof(int));
    r->hasfirst = firstset(r, 0, r->mark);
    return r;
}

void re_free(void *prog)
{
    RC *r = prog;
    if (!r) return;
    cu_free(r->mark); cu_free(r->ta); cu_free(r->tb); cu_free(r);
}

/* s[from..len), with ^ only at s[0]: 1 a match (caps[0], caps[1] its start
 * and end), 0 none, -1 no memory */
int re_exec(void *prog, const char *s, int len, int from, int *caps)
{
    RC *r = prog;
    VM v;
    TList a, b, *cl = &a, *nl = &b, *t;
    int sp, k, matched = 0, start[RSLOTS];
    int nocase = r->nocase;
    v.r = r; v.s = s; v.len = len; v.gen = 0;
    v.mark = r->mark; a.t = r->ta; b.t = r->tb;
    for (k = 0; k < r->n; k++) v.mark[k] = -1;
    for (k = 0; k < RSLOTS; k++) start[k] = -1;
    a.n = b.n = 0;
    for (sp = from; ; sp++) {
        if (!matched && !cl->n && r->hasfirst) {   /* nothing going on: on to a byte that can start one */
            while (sp < len && !((r->first[(unsigned char)s[sp] >> 3] >> (s[sp] & 7)) & 1)) sp++;
            if (sp >= len) break;
        }
        if (!matched) add(&v, cl, 0, start, sp);   /* a new start here, behind the older ones */
        if (!cl->n) break;
        v.gen++;
        nl->n = 0;
        for (k = 0; k < cl->n; k++) {
            Thr *th = &cl->t[k];
            RInst *in = &r->code[th->pc];
            int c = sp < v.len ? (unsigned char)s[sp] : -1, ok = 0;
            if (in->op == R_MATCH) {
                matched = 1;
                memcpy(caps, th->caps, sizeof th->caps);
                break;                       /* the threads after it have lower priority */
            }
            if (c < 0) continue;
            if (in->op == R_CHAR) ok = (nocase ? lower(c) : c) == in->c;
            else if (in->op == R_ANY) ok = 1;
            else if (in->op == R_CLASS) ok = (r->cls[in->x][c >> 3] >> (c & 7)) & 1;
            if (ok) add(&v, nl, th->pc + 1, th->caps, sp + 1);
        }
        if (sp >= v.len && !nl->n) break;
        t = cl; cl = nl; nl = t;
        if (sp >= v.len) {                   /* matches waiting at the very end */
            for (k = 0; k < cl->n; k++) if (r->code[cl->t[k].pc].op == R_MATCH) { matched = 1; memcpy(caps, cl->t[k].caps, sizeof cl->t[0].caps); break; }
            break;
        }
    }
    return matched;
}
