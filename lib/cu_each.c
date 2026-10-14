/* cu_each.c - patterns and the names they stand for (cu.h). */
#include <stdlib.h>
#include "cu.h"

/* ---- patterns ---- */

int cu_haswild(const char *s)
{
    for (; *s; s++) if (*s == '#' || *s == '?' || *s == '*' || *s == '|' || *s == '(' || *s == '[' || *s == '~') return 1;
    return 0;
}

/* * as #? (csh hands #? on, the ROM Shell hands * as typed) */
static int amigapat(const char *pat, char *to, int size)
{
    int k = 0;
    for (; *pat; pat++) {
        if (k + 3 > size) return 0;
        if (*pat == '*') { to[k++] = '#'; to[k++] = '?'; } else to[k++] = *pat;
    }
    to[k] = 0;
    return 1;
}

int cu_pattern(const char *pat, char *parsed, int size)
{
    char t[300];
    if (!amigapat(pat, t, sizeof t)) return 0;
    return ParsePatternNoCase(t, parsed, size) >= 0;
}

int cu_match(const char *parsed, const char *name)
{
    return MatchPatternNoCase((STRPTR)parsed, (STRPTR)name);
}

static int lc(int c)
{
    c &= 0xff;
    if ((c >= 'A' && c <= 'Z') || (c >= 0xc0 && c <= 0xde && c != 0xd7)) c += 32;
    return c;
}

/* natural, case blind: "file9" before "file10" (as ls sorts) */
int cu_natcmp(const char *a, const char *b)
{
    while (*a && *b) {
        if (*a >= '0' && *a <= '9' && *b >= '0' && *b <= '9') {
            const char *x = a, *y = b;
            int lx, ly;
            while (*x == '0') x++;
            while (*y == '0') y++;
            for (lx = 0; x[lx] >= '0' && x[lx] <= '9'; lx++) ;
            for (ly = 0; y[ly] >= '0' && y[ly] <= '9'; ly++) ;
            if (lx != ly) return lx - ly;
            for (; lx; lx--, x++, y++) if (*x != *y) return *x - *y;
            a = x; b = y;
            continue;
        }
        if (lc(*a) != lc(*b)) return lc(*a) - lc(*b);
        a++; b++;
    }
    return lc(*a) - lc(*b);
}

static int cmpname(const void *a, const void *b) { return cu_natcmp(*(char **)a, *(char **)b); }

int cu_each(const char *arg, int (*fn)(const char *path, void *u), void *u)
{
    struct AnchorPath *ap;
    char pat[300];
    char **names = NULL;
    int n = 0, max = 0, i, r = 0;
    LONG e;
    BPTR l;
    if (!cu_haswild(arg)) return fn(arg, u);
    if ((l = Lock((STRPTR)arg, SHARED_LOCK))) { UnLock(l); return fn(arg, u); }  /* a name as typed */
    if (!amigapat(arg, pat, sizeof pat)) { cu_err(10, arg, "pattern too long"); return 1; }
    if (!(ap = cu_alloc(sizeof *ap + CU_PATH))) { cu_err(20, NULL, "out of memory"); return 1; }
    memset(ap, 0, sizeof *ap);
    ap->ap_BreakBits = SIGBREAKF_CTRL_C;
    ap->ap_Strlen = CU_PATH;
    for (e = MatchFirst(pat, ap); !e; e = MatchNext(ap)) {
        if (n == max) {
            char **t = cu_alloc((max = max ? max * 2 : 32) * sizeof *t);
            if (!t) break;
            if (names) { memcpy(t, names, n * sizeof *t); cu_free(names); }
            names = t;
        }
        if (!(names[n] = cu_strdup((char *)ap->ap_Buf))) break;
        n++;
    }
    MatchEnd(ap);
    cu_free(ap);
    if (e == ERROR_BREAK) { SetSignal(SIGBREAKF_CTRL_C, SIGBREAKF_CTRL_C); cu_break(); r = 1; }
    else if (e != ERROR_NO_MORE_ENTRIES && e != 0) { SetIoErr(e); cu_fault(10, arg); r = 1; }
    else if (!n) cu_err(5, arg, "nothing matches");
    if (n > 1) qsort(names, n, sizeof *names, cmpname);
    for (i = 0; i < n; i++) {
        if (!r && !cu_break()) r = fn(names[i], u);
        cu_free(names[i]);
    }
    cu_free(names);
    return r;
}

