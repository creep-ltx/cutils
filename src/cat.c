/* cat - files one after another to the output.
 *
 *   cat [-bnsEv] [file | pattern | - ...]
 *
 * No file, or -: the input (a pipe). With no flags the bytes go through in
 * big blocks, untouched.
 *   -n number every line   -b number the lines that are not empty
 *   -s one empty line where there were several   -E $ at each line's end
 *   -v control characters as ^X (Tab and the line end kept), M- for 128+
 *
 * Return code 0, 5 when a file could not be read, 10 bad arguments,
 * 20 Ctrl-C. */
#include "cu.h"

const char *cu_name = "cat";
static const char version[] = "$VER: cat 0.1b1 (14.10.2026)";

static int o_num, o_nonblank, o_squeeze, o_ends, o_show;
static LONG lineno;
static int blanks;

static void shown(const char *s, LONG n)
{
    LONG i;
    if (!o_show) { cu_put(s, n); return; }
    for (i = 0; i < n; i++) {
        int c = (unsigned char)s[i];
        if (c >= 128) { cu_puts("M-"); c -= 128; }
        if (c < 32 && c != '\t') { cu_putc('^'); cu_putc(c + 64); }
        else if (c == 127) { cu_putc('^'); cu_putc('?'); }
        else cu_putc(c);
    }
}

static int one(const char *path, void *u)
{
    CuIn in;
    (void)u;
    if (!cu_open(&in, path)) return 0;
    if (!o_num && !o_nonblank && !o_squeeze && !o_ends && !o_show) {
        LONG n;
        cu_flush();
        while ((n = Read(in.fh, in.buf, in.size)) > 0) {
            cu_put(in.buf, n);
            if (cu_break()) break;
        }
        if (n < 0) cu_fault(5, path);
    } else {
        char *line;
        LONG n;
        int nl, k = 0;
        while ((n = cu_line(&in, &line, &nl)) >= 0) {
            if (!n) {
                if (o_squeeze && blanks) continue;
                blanks = 1;
            } else blanks = 0;
            if (o_num && !(o_nonblank && !n)) { cu_putnum(++lineno, 6); cu_putc('\t'); }
            shown(line, n);
            if (o_ends) cu_putc('$');
            if (nl) cu_putc('\n');
            if (!(++k & 255) && cu_break()) break;
        }
        if (in.err) cu_fault(5, path);
    }
    cu_close(&in);
    return cu_rc >= 20;
}

int main(void)
{
    char *v;
    int c, i;
    (void)version;
    if (cu_args("cat [-bnsEv] [file | pattern | - ...]") < 0) return cu_rc;
    while ((c = cu_opt("bnsEv", &v)) >= 0) switch (c) {
        case 'b': o_num = o_nonblank = 1; break;
        case 'n': o_num = 1; break;
        case 's': o_squeeze = 1; break;
        case 'E': o_ends = 1; break;
        case 'v': o_show = 1; break;
        default: return cu_rc;
    }
    if (!cu_argc) one(NULL, NULL);
    for (i = 0; i < cu_argc && cu_rc < 20; i++) cu_each(cu_argv[i], one, NULL);
    cu_flush();
    return cu_rc;
}
