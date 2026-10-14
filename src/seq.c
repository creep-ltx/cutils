/* seq - count: numbers from FIRST to LAST.
 *
 *   seq [-w] [-s SEP] [-f FORMAT] [FIRST [INC]] LAST
 *
 * FIRST and INC are 1 unless given; INC may be negative (seq 10 -2 1).
 * Whole numbers, or decimals (seq 0 0.25 1): then every number gets as
 * many decimals as the most precise argument. Negative numbers are
 * numbers, not flags (seq -3 3). Nothing at all when FIRST is already
 * past LAST.
 *   -w         equal width, padded with leading zeros (008 009 010)
 *   -s SEP     between the numbers instead of a new line; \t and \n in
 *              it are a tab and a new line (the ROM Shell has no $'...')
 *   -f FORMAT  printf-like: one %d, %f or %g with flags 0 - + and
 *              width and .precision (%03d, %.2f, %5g); text around it
 *              is kept, %% is a %
 * The last number always ends with a new line.
 *
 * Return code 0, 10 bad arguments, 20 Ctrl-C. */
#include "cu.h"

const char *cu_name = "seq";
static const char version[] = "$VER: seq 0.1b1 (14.10.2026)";

typedef long long VAL;           /* the number times 10^dec */
static int dec;                  /* decimals of the most precise argument */

/* a number: its digits without the point, and how many decimals it had */
static int parse(const char *s, VAL *v, int *d)
{
    int neg = 0, any = 0, k = 0, point = 0;
    VAL x = 0;
    if (*s == '-') neg = 1, s++;
    else if (*s == '+') s++;
    for (; *s; s++) {
        if (*s == '.' && !point) { point = 1; continue; }
        if (*s < '0' || *s > '9') return 0;
        if (x > 99999999999999LL) return 0;
        x = x * 10 + (*s - '0'), any = 1;
        if (point) k++;
    }
    if (!any || k > 9) return 0;
    *v = neg ? -x : x;
    *d = k;
    return 1;
}

static VAL scale(VAL v, int from)
{
    while (from++ < dec) v *= 10;
    return v;
}

/* v (times 10^dec) as text into b: sign, digits, point; returns the length */
static int text(VAL v, int prec, char *b)
{
    char t[40];
    int i = 0, n = 0, d = dec, k;
    unsigned long long u = v < 0 ? -(unsigned long long)v : (unsigned long long)v;
    /* to prec decimals: cut or pad (rounded half away from zero) */
    while (d > prec) { u = (u + (d - prec == 1 ? 5 : 0)) / 10; d--; }
    do { t[i++] = '0' + (int)(u % 10); u /= 10; } while (u || i <= d);
    for (k = 0; k < i && t[k] == '0'; k++) ;
    if (v < 0 && k < i) b[n++] = '-';
    while (i > d) b[n++] = t[--i];
    if (d || prec > d) {
        b[n++] = '.';
        while (i) b[n++] = t[--i];
        while (d < prec) b[n++] = '0', d++;
    }
    b[n] = 0;
    return n;
}

/* the -f format, split: before, the conversion, after */
static const char *fpre, *fpost;
static int fpre_n, fflag0, fminus, fplus, fwidth, fprec = -1, fconv;

static int setformat(const char *f)
{
    const char *p = f, *start = NULL;
    while (*p) {
        if (*p == '%' && p[1] == '%') { p += 2; continue; }
        if (*p == '%') {
            if (start) return 0;
            start = p++;
            for (;; p++) {
                if (*p == '0') fflag0 = 1;
                else if (*p == '-') fminus = 1;
                else if (*p == '+') fplus = 1;
                else if (*p == ' ' || *p == '#') ;
                else break;
            }
            while (*p >= '0' && *p <= '9') fwidth = fwidth * 10 + (*p++ - '0');
            if (*p == '.') { fprec = 0; p++; while (*p >= '0' && *p <= '9') fprec = fprec * 10 + (*p++ - '0'); }
            if (!*p || !strchr("dfgiFG", *p)) return 0;
            fconv = *p++;
            fpost = p;
            continue;
        }
        p++;
    }
    if (!start) return 0;
    fpre = f; fpre_n = start - f;
    return 1;
}

static void putpct(const char *s, int n)   /* text with %% as % */
{
    int i;
    for (i = 0; i < n; i++) {
        if (s[i] == '%' && s[i + 1] == '%') i++;
        cu_putc(s[i]);
    }
}

static void pad(int c, int n) { while (n-- > 0) cu_putc(c); }

static void out(VAL v, int width)
{
    char b[48], *p = b + 1;
    int n, prec = dec;
    if (fconv) {
        int conv = fconv | 0x20, sign = 0;
        putpct(fpre, fpre_n);
        if (conv == 'd' || conv == 'i') prec = 0;
        else if (conv == 'f') prec = fprec >= 0 ? fprec : 6;
        else if (fprec >= 0 && fprec < dec) prec = fprec;   /* %g: as precise as the numbers */
        n = text(v, prec, p);
        if (*p == '-') sign = *p++, n--;
        else if (fplus) sign = '+';
        if (fminus) { if (sign) cu_putc(sign); cu_put(p, n); pad(' ', fwidth - n - !!sign); }
        else if (fflag0) { if (sign) cu_putc(sign); pad('0', fwidth - n - !!sign); cu_put(p, n); }
        else { pad(' ', fwidth - n - !!sign); if (sign) cu_putc(sign); cu_put(p, n); }
        putpct(fpost, strlen(fpost));
        return;
    }
    n = text(v, prec, p);
    if (width) {
        if (*p == '-') { cu_putc('-'); p++; n--; width--; }
        pad('0', width - n);
    }
    cu_put(p, n);
}

static void putsep(const char *s)
{
    for (; *s; s++) {
        if (*s == '\\' && s[1] == 't') cu_putc('\t'), s++;
        else if (*s == '\\' && s[1] == 'n') cu_putc('\n'), s++;
        else if (*s == '\\' && s[1] == '\\') cu_putc('\\'), s++;
        else cu_putc(*s);
    }
}

int main(void)
{
    char *sep = NULL, *fmt = NULL, *num[3];
    int i, k = 0, w = 0, d[3];
    VAL v[3], first, inc, last, x;
    long count = 0;
    (void)version;
    if (cu_args("seq [-w] [-s SEP] [-f FORMAT] [FIRST [INC]] LAST") < 0) return cu_rc;
    /* by hand: -3 is a number, not a flag */
    for (i = 0; i < cu_argc; i++) {
        char *a = cu_argv[i];
        if (a[0] == '-' && a[1] && !(a[1] >= '0' && a[1] <= '9') && a[1] != '.' && k == 0) {
            if (!strcmp(a, "--")) { i++; break; }
            for (a++; *a; a++) {
                if (*a == 'w') w = 1;
                else if (*a == 's' || *a == 'f') {
                    char **to = *a == 's' ? &sep : &fmt;
                    if (a[1]) *to = a + 1;
                    else if (i + 1 < cu_argc) *to = cu_argv[++i];
                    else { char t[3] = { '-', *a, 0 }; cu_err(10, t, "needs a value"); return cu_rc; }
                    break;
                } else { char t[3] = { '-', *a, 0 }; cu_err(10, t, "unknown option"); return cu_rc; }
            }
            continue;
        }
        if (k == 3) { cu_err(10, a, "too many numbers (seq ? for usage)"); return cu_rc; }
        num[k++] = a;
    }
    for (; i < cu_argc; i++) {
        if (k == 3) { cu_err(10, cu_argv[i], "too many numbers (seq ? for usage)"); return cu_rc; }
        num[k++] = cu_argv[i];
    }
    if (!k) { cu_err(10, NULL, "no last number (seq ? for usage)"); return cu_rc; }
    for (i = 0; i < k; i++) {
        if (!parse(num[i], &v[i], &d[i])) { cu_err(10, num[i], "not a number"); return cu_rc; }
        if (d[i] > dec) dec = d[i];
    }
    if (fmt && !setformat(fmt)) { cu_err(10, fmt, "needs one %d, %f or %g"); return cu_rc; }
    first = k > 1 ? scale(v[0], d[0]) : scale(1, 0);
    inc = k > 2 ? scale(v[1], d[1]) : scale(1, 0);
    last = scale(v[k - 1], d[k - 1]);
    if (!inc) { cu_err(10, num[1], "an increment of 0 never ends"); return cu_rc; }
    if (w && !fmt) {                         /* the widest of first and the last one reached */
        char b[48];
        VAL end = first + (last - first) / inc * inc;
        int a = text(first, dec, b), z = text((inc > 0 ? last >= first : last <= first) ? end : first, dec, b);
        w = a > z ? a : z;
    }
    for (x = first; inc > 0 ? x <= last : x >= last; x += inc) {
        if (count++) { if (sep) putsep(sep); else cu_putc('\n'); }
        out(x, w);
        if (!(count & 255) && cu_break()) break;
    }
    if (count) cu_putc('\n');
    cu_flush();
    return cu_rc;
}
