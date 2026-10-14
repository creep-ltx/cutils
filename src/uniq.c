/* uniq - runs of equal lines told once.
 *
 *   uniq [-cdui] [-f N] [-s N] [-w N] [IN [OUT]]
 *
 * Lines that follow each other and are equal come out once (sort first
 * for all of them). IN missing or -: the input; OUT: written instead of
 * shown.
 *   -c each line after its count   -d only lines that came more than once
 *   -u only lines that came once   -i case blind (Latin-1 too)
 *   -f N the first N fields (blanks, then non-blanks) do not count
 *   -s N nor the first N characters (after the fields)   -w N only the
 *   first N characters (after those) count
 *
 * Return code 0, 5 IN could not be read, 10 bad arguments or OUT could
 * not be written, 20 Ctrl-C. */
#include "cu.h"

const char *cu_name = "uniq";
static const char version[] = "$VER: uniq 0.1b1 (14.10.2026)";

static int o_count, o_dup, o_uni, o_icase;
static LONG o_fields, o_chars, o_width = -1;

static int lower(int c) { c &= 0xff; return (c >= 'A' && c <= 'Z') || (c >= 0xc0 && c <= 0xde && c != 0xd7) ? c + 32 : c; }

/* the part of a line that counts */
static const char *part(const char *s, LONG n, LONG *len)
{
    const char *e = s + n;
    LONG f = o_fields, c = o_chars;
    while (f-- > 0 && s < e) {
        while (s < e && (*s == ' ' || *s == '\t')) s++;
        while (s < e && *s != ' ' && *s != '\t') s++;
    }
    s = c < e - s ? s + c : e;
    *len = o_width >= 0 && o_width < e - s ? o_width : e - s;
    return s;
}

static int same(const char *a, LONG an, const char *b, LONG bn)
{
    LONG al, bl, i;
    a = part(a, an, &al);
    b = part(b, bn, &bl);
    if (al != bl) return 0;
    if (!o_icase) return !memcmp(a, b, al);
    for (i = 0; i < al; i++) if (lower(a[i]) != lower(b[i])) return 0;
    return 1;
}

static void out(const char *s, LONG n, LONG count)
{
    if ((o_dup && count < 2) || (o_uni && count > 1)) return;
    if (o_count) { cu_putnum(count, 7); cu_putc(' '); }
    cu_put(s, n);
    cu_putc('\n');
}

int main(void)
{
    char *v, *line, *prev = NULL;
    LONG n, pn = 0, psize = 0, count = 0, k = 0;
    int c, ok, nl;
    CuIn in;
    BPTR fh = 0;
    (void)version;
    if (cu_args("uniq [-cdui] [-f N] [-s N] [-w N] [IN [OUT]]") < 0) return cu_rc;
    while ((c = cu_opt("cduif:s:w:", &v)) >= 0) switch (c) {
        case 'c': o_count = 1; break;
        case 'd': o_dup = 1; break;
        case 'u': o_uni = 1; break;
        case 'i': o_icase = 1; break;
        case 'f': case 's': case 'w': {
            LONG x = cu_num(v, &ok);
            if (!ok || x < 0) { cu_err(10, v, "not a count"); return cu_rc; }
            if (c == 'f') o_fields = x; else if (c == 's') o_chars = x; else o_width = x;
            break;
        }
        default: return cu_rc;
    }
    if (cu_argc > 2) { cu_err(10, cu_argv[2], "one input and one output at most"); return cu_rc; }
    if (!cu_open(&in, cu_argc ? cu_argv[0] : NULL)) return cu_rc;
    if (cu_argc == 2 && strcmp(cu_argv[1], "-")) {
        if (!(fh = Open((STRPTR)cu_argv[1], MODE_NEWFILE))) { cu_fault(10, cu_argv[1]); cu_close(&in); return cu_rc; }
        cu_out = fh;
    }
    while ((n = cu_line(&in, &line, &nl)) >= 0) {
        if (count && same(prev, pn, line, n)) count++;
        else {
            if (count) out(prev, pn, count);
            if (n + 1 > psize) {
                cu_free(prev);
                psize = n + 256;
                if (!(prev = cu_alloc(psize))) { cu_err(20, NULL, "out of memory"); break; }
            }
            memcpy(prev, line, n);
            pn = n;
            count = 1;
        }
        if (!(++k & 1023) && cu_break()) break;
    }
    if (count && cu_rc < 20) out(prev, pn, count);
    if (in.err) cu_fault(5, cu_argc ? cu_argv[0] : NULL);
    cu_close(&in);
    cu_flush();
    if (fh) Close(fh);
    return cu_rc;
}
