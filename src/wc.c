/* wc - lines, words and bytes of files.
 *
 *   wc [-lwcL] [file | pattern | - ...]
 *
 * All three unless told: lines, words, bytes, then the name; several
 * files: a total last. -L the longest line. One file or the input with one
 * count asked: the number alone (`List | wc -l`).
 *
 * Return code 0, 5 when a file could not be read, 10 bad arguments,
 * 20 Ctrl-C. */
#include "cu.h"

const char *cu_name = "wc";
static const char version[] = "$VER: wc 0.1b1 (14.10.2026)";

static int o_l, o_w, o_c, o_L, nfiles, bare;
static ULONG tl, tw, tc, tL;
static unsigned char space[256];   /* what ends a word */

static void report(ULONG l, ULONG w, ULONG c, ULONG L, const char *name)
{
    int width = bare ? 0 : 7, sp = 0;
    if (o_l) { cu_putnum(l, width); sp = 1; }
    if (o_w) { if (sp) cu_putc(' '); cu_putnum(w, width); sp = 1; }
    if (o_c) { if (sp) cu_putc(' '); cu_putnum(c, width); sp = 1; }
    if (o_L) { if (sp) cu_putc(' '); cu_putnum(L, width); }
    if (name) { cu_putc(' '); cu_puts(name); }
    cu_putc('\n');
}

static int one(const char *path, void *u)
{
    CuIn in;
    ULONG l = 0, w = 0, c = 0, L = 0, col = 0;
    int inword = 0, k = 0;
    LONG n;
    (void)u;
    if (!cu_open(&in, path)) return 0;
    while ((n = Read(in.fh, in.buf, in.size)) > 0) {
        unsigned char *p = (unsigned char *)in.buf, *e = p + n;
        c += n;
        if (o_L) {
            for (; p < e; p++) {
                int ch = *p;
                if (ch == '\n') { l++; if (col > L) L = col; col = 0; }
                else if (ch == '\t') col = (col | 7) + 1;
                else col++;
                if (space[ch]) inword = 0;
                else if (!inword) { inword = 1; w++; }
            }
        } else if (o_w) {
            for (; p < e; p++) {
                if (space[*p]) { inword = 0; if (*p == '\n') l++; }
                else if (!inword) { inword = 1; w++; }
            }
        } else {
            for (; (p = memchr(p, '\n', e - p)); p++) l++;
        }
        if (!(++k & 15) && cu_break()) break;
    }
    if (col > L) L = col;
    if (n < 0) cu_fault(5, path);
    cu_close(&in);
    tl += l; tw += w; tc += c; if (L > tL) tL = L;
    nfiles++;
    report(l, w, c, L, path);
    return cu_rc >= 20;
}

int main(void)
{
    char *v;
    int c, i;
    (void)version;
    if (cu_args("wc [-lwcL] [file | pattern | - ...]") < 0) return cu_rc;
    while ((c = cu_opt("lwcmL", &v)) >= 0) switch (c) {
        case 'l': o_l = 1; break;
        case 'w': o_w = 1; break;
        case 'c': case 'm': o_c = 1; break;
        case 'L': o_L = 1; break;
        default: return cu_rc;
    }
    if (!o_l && !o_w && !o_c && !o_L) o_l = o_w = o_c = 1;
    space[' '] = space['\n'] = space['\t'] = space['\r'] = space['\f'] = space['\v'] = space[0xa0] = 1;
    bare = o_l + o_w + o_c + o_L == 1 && cu_argc <= 1;
    if (!cu_argc) one(NULL, NULL);
    for (i = 0; i < cu_argc && cu_rc < 20; i++) cu_each(cu_argv[i], one, NULL);
    if (nfiles > 1) report(tl, tw, tc, tL, "total");
    cu_flush();
    return cu_rc;
}
