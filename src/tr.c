/* tr - characters changed, deleted or squeezed, input to output.
 *
 *   tr [-cCds] SET1 [SET2]
 *
 *   tr SET1 SET2       each character in SET1 becomes the one at the same
 *                      place in SET2 (SET2 short: its last one repeated)
 *   tr -d SET1         the characters in SET1 left out
 *   tr -s SET1         a run of one character in SET1 becomes one
 *   tr -s SET1 SET2    changed, then runs of those in SET2 squeezed
 *   tr -ds SET1 SET2   SET1 left out, runs of SET2 squeezed
 *   -c, -C             SET1 is every character NOT in it (in byte order)
 * In a SET: a-z ranges; \n \t \r \a \b \f \v \\ and \NNN (octal);
 * [:alpha:] [:upper:] [:lower:] [:digit:] [:alnum:] [:space:] [:blank:]
 * [:punct:] [:cntrl:] [:print:] [:graph:] [:xdigit:]; in SET2 [c*N] (c N
 * times) and [c*] (c until SET2 is as long as SET1). Bytes, Latin-1:
 * upper and lower include À-Þ and à-þ, so `tr [:upper:] [:lower:]` does
 * Swedish too. In the ROM Shell, * inside quotes is the escape: write
 * [c**5] there, or leave the quotes off.
 * It only reads the input and writes the output: `tr a-z A-Z <in >out`.
 *
 * Return code 0, 10 bad arguments, 20 Ctrl-C. */
#include "cu.h"

const char *cu_name = "tr";
static const char version[] = "$VER: tr 0.1b1 (14.10.2026)";

#define MAXSET 8192

typedef struct { unsigned char c[MAXSET]; LONG n; LONG fill; } Set;   /* fill: where [c*] goes, -1 none */

static int isup(int c) { return (c >= 'A' && c <= 'Z') || (c >= 0xc0 && c <= 0xde && c != 0xd7); }
static int islo(int c) { return (c >= 'a' && c <= 'z') || (c >= 0xe0 && c <= 0xfe && c != 0xf7); }

static int inclass(const char *name, int c)
{
    int alpha = isup(c) || islo(c) || c == 0xdf || c == 0xff, digit = c >= '0' && c <= '9';
    int print = (c >= 0x20 && c < 0x7f) || c >= 0xa0;
    if (!strcmp(name, "alpha")) return alpha;
    if (!strcmp(name, "upper")) return isup(c);
    if (!strcmp(name, "lower")) return islo(c);
    if (!strcmp(name, "digit")) return digit;
    if (!strcmp(name, "alnum")) return alpha || digit;
    if (!strcmp(name, "space")) return c == ' ' || (c >= 9 && c <= 13);
    if (!strcmp(name, "blank")) return c == ' ' || c == '\t';
    if (!strcmp(name, "punct")) return print && !alpha && !digit && c != ' ' && c != 0xa0;
    if (!strcmp(name, "cntrl")) return c < 0x20 || (c >= 0x7f && c < 0xa0);
    if (!strcmp(name, "print")) return print;
    if (!strcmp(name, "graph")) return print && c != ' ' && c != 0xa0;
    if (!strcmp(name, "xdigit")) return digit || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    return -1;
}

/* one character, \ escapes read */
static int onechar(const char **pp)
{
    const char *p = *pp;
    int c = (unsigned char)*p++;
    if (c == '\\' && *p) {
        c = (unsigned char)*p++;
        switch (c) {
            case 'n': c = '\n'; break;
            case 't': c = '\t'; break;
            case 'r': c = '\r'; break;
            case 'a': c = 7; break;
            case 'b': c = 8; break;
            case 'f': c = 12; break;
            case 'v': c = 11; break;
            default:
                if (c >= '0' && c <= '7') {
                    int k = 1;
                    c -= '0';
                    while (k < 3 && *p >= '0' && *p <= '7') c = c * 8 + *p++ - '0', k++;
                    c &= 0xff;
                }
        }
    }
    *pp = p;
    return c;
}

static int add(Set *s, int c, LONG times)
{
    while (times-- > 0) {
        if (s->n == MAXSET) { cu_err(10, NULL, "a set is too long"); return 0; }
        s->c[s->n++] = c;
    }
    return 1;
}

static int parse(Set *s, const char *p, int two)
{
    s->n = 0; s->fill = -1;
    while (*p) {
        int c, d;
        if (p[0] == '[' && p[1] == ':') {
            const char *e = strstr(p + 2, ":]");
            char name[12];
            if (e && e - p - 2 < (int)sizeof name) {
                memcpy(name, p + 2, e - p - 2);
                name[e - p - 2] = 0;
                if (inclass(name, 'a') < 0) { cu_err(10, name, "no such class"); return 0; }
                for (c = 0; c < 256; c++) if (inclass(name, c) && !add(s, c, 1)) return 0;
                p = e + 2;
                continue;
            }
        }
        if (p[0] == '[' && p[1] && two) {        /* [c*N] or [c*] */
            const char *q = p + 1;
            c = onechar(&q);
            if (*q == '*') {
                LONG n = 0;
                int base = q[1] == '0' ? 8 : 10;
                q++;
                while (*q >= '0' && *q <= '9') n = n * base + *q++ - '0';
                if (*q == ']') {
                    if (n) { if (!add(s, c, n)) return 0; }
                    else {
                        if (s->fill >= 0) { cu_err(10, NULL, "only one [c*] in SET2"); return 0; }
                        s->fill = s->n;
                        if (!add(s, c, 1)) return 0;
                    }
                    p = q + 1;
                    continue;
                }
            }
        }
        c = onechar(&p);
        if (*p == '-' && p[1]) {
            p++;
            d = onechar(&p);
            if (d < c) { cu_err(10, NULL, "a range is backwards"); return 0; }
            for (; c <= d; c++) if (!add(s, c, 1)) return 0;
            continue;
        }
        if (!add(s, c, 1)) return 0;
    }
    return 1;
}

static Set s1, s2;

int main(void)
{
    char *v, *buf;
    int c, i, o_c = 0, o_d = 0, o_s = 0, last = -1;
    unsigned char map[256], del[256], sq[256];
    LONG n;
    BPTR in;
    (void)version;
    if (cu_args("tr [-cCds] SET1 [SET2]") < 0) return cu_rc;
    while ((c = cu_opt("cCds", &v)) >= 0) switch (c) {
        case 'c': case 'C': o_c = 1; break;
        case 'd': o_d = 1; break;
        case 's': o_s = 1; break;
        default: return cu_rc;
    }
    if (!cu_argc) { cu_err(10, NULL, "no SET1"); return cu_rc; }
    if (cu_argc > 2) { cu_err(10, cu_argv[2], "too many sets"); return cu_rc; }
    if (cu_argc == 2 && o_d && !o_s) { cu_err(10, cu_argv[1], "-d takes one set (with -s, two)"); return cu_rc; }
    if (cu_argc == 1 && !o_d && !o_s) { cu_err(10, NULL, "no SET2"); return cu_rc; }
    if (o_d && o_s && cu_argc < 2) { cu_err(10, NULL, "-ds needs SET2"); return cu_rc; }
    if (!parse(&s1, cu_argv[0], 0)) return cu_rc;
    if (cu_argc == 2 && !parse(&s2, cu_argv[1], 1)) return cu_rc;
    if (o_c) {                           /* every byte not in SET1, in order */
        unsigned char in1[256];
        memset(in1, 0, sizeof in1);
        for (i = 0; i < s1.n; i++) in1[s1.c[i]] = 1;
        for (s1.n = 0, c = 0; c < 256; c++) if (!in1[c]) s1.c[s1.n++] = c;
    }
    if (s2.fill >= 0 && s2.n < s1.n) {   /* [c*]: as long as SET1 */
        LONG extra = s1.n - s2.n;
        if (s2.n + extra > MAXSET) { cu_err(10, NULL, "a set is too long"); return cu_rc; }
        memmove(s2.c + s2.fill + extra, s2.c + s2.fill, s2.n - s2.fill);
        memset(s2.c + s2.fill, s2.c[s2.fill + extra], extra);
        s2.n += extra;
    }
    for (c = 0; c < 256; c++) map[c] = c;
    memset(del, 0, sizeof del);
    memset(sq, 0, sizeof sq);
    if (o_d) {
        for (i = 0; i < s1.n; i++) del[s1.c[i]] = 1;
        if (o_s) for (i = 0; i < s2.n; i++) sq[s2.c[i]] = 1;
    } else if (cu_argc == 2) {
        if (!s2.n) { cu_err(10, NULL, "SET2 is empty"); return cu_rc; }
        for (i = 0; i < s1.n; i++) map[s1.c[i]] = s2.c[i < s2.n ? i : s2.n - 1];
        if (o_s) for (i = 0; i < s2.n; i++) sq[s2.c[i]] = 1;
    } else for (i = 0; i < s1.n; i++) sq[s1.c[i]] = 1;
    if (!(buf = cu_alloc(16384))) { cu_err(20, NULL, "out of memory"); return cu_rc; }
    in = Input();
    while ((n = Read(in, buf, 16384)) > 0) {
        LONG k, o = 0;
        for (k = 0; k < n; k++) {
            int ch = (unsigned char)buf[k];
            if (del[ch]) continue;
            ch = map[ch];
            if (sq[ch] && ch == last) continue;
            last = ch;
            buf[o++] = ch;
        }
        cu_put(buf, o);
        if (cu_break()) break;
    }
    if (n < 0) cu_fault(10, NULL);
    cu_flush();
    return cu_rc;
}
