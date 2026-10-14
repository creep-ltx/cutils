/* cut - parts of each line: fields, characters or bytes.
 *
 *   cut -f LIST [-d SEP] [-s] [--complement] [--output-delimiter=S] [file ...]
 *   cut -c LIST | -b LIST [--complement] [file | pattern | - ...]
 *
 * LIST: numbers from 1, joined by commas: N, N-M, N- (to the end), -M
 * (from the start); in any order, the parts come out in the line's order.
 *   -f the fields, split at SEP (a tab unless -d; \t is a tab too), put
 *   out joined by SEP (or --output-delimiter=S); a line without SEP comes
 *   out whole, unless -s   -c the characters   -b the bytes (the same:
 *   Latin-1 is a byte a character)   --complement all but the LIST
 * No file, or -: the input.
 *
 * Return code 0, 5 a file could not be read, 10 bad arguments, 20 Ctrl-C. */
#include "cu.h"

const char *cu_name = "cut";
static const char version[] = "$VER: cut 0.1b1 (14.10.2026)";

#define MAXR 64
static LONG rlo[MAXR], rhi[MAXR];     /* ranges, 1-based, hi < 0: to the end */
static int nr, fields, only, compl;
static int sep = '\t';
static const char *osep;
static LONG oseplen = 1;

static int list(const char *s)
{
    const char *p = s;
    while (*p) {
        LONG lo = 0, hi;
        int dl = 0, dh = 0;
        if (nr == MAXR) { cu_err(10, s, "too many ranges"); return 0; }
        while (*p >= '0' && *p <= '9') lo = lo * 10 + *p++ - '0', dl = 1;
        if (*p == '-') {
            p++;
            hi = 0;
            while (*p >= '0' && *p <= '9') hi = hi * 10 + *p++ - '0', dh = 1;
            if (!dl && !dh) goto bad;            /* "-" alone */
            if (!dl) lo = 1;
            if (!dh) hi = -1;
        } else {
            if (!dl) goto bad;
            hi = lo;
        }
        if (!lo || (hi >= 0 && hi < lo)) goto bad;
        rlo[nr] = lo; rhi[nr] = hi; nr++;
        if (*p == ',') { p++; if (!*p) goto bad; }
        else if (*p) goto bad;
    }
    if (!nr) goto bad;
    return 1;
bad:
    cu_err(10, s, "not a list (N, N-M, N-, -M, with commas)");
    return 0;
}

static int sel(LONG i)
{
    int k;
    for (k = 0; k < nr; k++) if (i >= rlo[k] && (rhi[k] < 0 || i <= rhi[k])) return !compl;
    return compl;
}

static void doline(const char *s, LONG n)
{
    if (!fields) {
        LONG i, a = -1;
        for (i = 0; i < n; i++) {          /* runs of chosen bytes, put out whole */
            if (sel(i + 1)) { if (a < 0) a = i; }
            else if (a >= 0) { cu_put(s + a, i - a); a = -1; }
        }
        if (a >= 0) cu_put(s + a, n - a);
    } else {
        const char *p = s, *e = s + n, *f;
        LONG i = 1;
        int any = 0;
        if (!memchr(s, sep, n)) {
            if (only) return;
            cu_put(s, n);
            cu_putc('\n');
            return;
        }
        for (;; i++) {
            f = memchr(p, sep, e - p);
            if (!f) f = e;
            if (sel(i)) {
                if (any) { if (osep) cu_put(osep, oseplen); else cu_putc(sep); }
                cu_put(p, f - p);
                any = 1;
            }
            if (f == e) break;
            p = f + 1;
        }
    }
    cu_putc('\n');
}

static int one(const char *path, void *u)
{
    CuIn in;
    char *line;
    LONG n, k = 0;
    int nl;
    (void)u;
    if (!cu_open(&in, path)) return 0;
    while ((n = cu_line(&in, &line, &nl)) >= 0) {
        doline(line, n);
        if (!(++k & 255) && cu_break()) break;
    }
    if (in.err) cu_fault(5, path);
    cu_close(&in);
    return cu_rc >= 20;
}

int main(void)
{
    char *v;
    int c, i, j, mode = 0;
    (void)version;
    if (cu_args("cut -f LIST [-d SEP] [-s] | -c LIST | -b LIST [--complement] [file ...]") < 0) return cu_rc;
    /* the long options, taken out before the short ones are read */
    for (i = j = 0; i < cu_argc; i++) {
        char *w = cu_argv[i];
        if (!strcmp(w, "--complement")) compl = 1;
        else if (!strncmp(w, "--output-delimiter=", 19)) { osep = w + 19; oseplen = strlen(osep); }
        else cu_argv[j++] = w;
        if (!strcmp(w, "--")) { for (i++; i < cu_argc; i++) cu_argv[j++] = cu_argv[i]; break; }
    }
    cu_argc = j;
    cu_argv[j] = NULL;
    while ((c = cu_opt("f:c:b:d:sn", &v)) >= 0) switch (c) {
        case 'f': case 'c': case 'b':
            if (mode && mode != c) { cu_err(10, NULL, "only one of -f, -c, -b"); return cu_rc; }
            mode = c;
            if (!list(v)) return cu_rc;
            break;
        case 'd':
            if (v[0] == '\\' && v[1] == 't' && !v[2]) sep = '\t';
            else if (v[0] && !v[1]) sep = (unsigned char)v[0];
            else { cu_err(10, v, "the separator must be one character"); return cu_rc; }
            break;
        case 's': only = 1; break;
        case 'n': break;
        default: return cu_rc;
    }
    if (!mode) { cu_err(10, NULL, "-f, -c or -b, with a list"); return cu_rc; }
    fields = mode == 'f';
    if (!cu_argc) one(NULL, NULL);
    for (i = 0; i < cu_argc && cu_rc < 20; i++) cu_each(cu_argv[i], one, NULL);
    cu_flush();
    return cu_rc;
}
