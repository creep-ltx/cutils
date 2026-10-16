/* head - the first lines (or bytes) of files.
 *
 *   head [-n N | -N] [-c N] [-qv] [file | pattern | - ...]
 *
 * 10 lines unless told. Several files: each under "==> name <==".
 *   -n N, -N  the first N lines   -c N  the first N bytes (k, M: 1024s)
 *   -q never the names   -v always the names
 * It stops reading as soon as it has them, so `big | head` ends at once.
 *
 * Return code 0, 5 when a file could not be read, 10 bad arguments,
 * 20 Ctrl-C. */
#include "cu.h"

const char *cu_name = "head";
static const char version[] = "$VER: head 0.1b2 (16.10.2026)";

static LONG count = 10;
static int bytes, names, first = 1;

static int one(const char *path, void *u)
{
    CuIn in;
    (void)u;
    if (!cu_open(&in, path)) return 0;
    if (names) {
        if (!first) cu_putc('\n');
        cu_puts("==> "); cu_puts(path ? path : "standard input"); cu_puts(" <==\n");
    }
    first = 0;
    if (bytes) {
        LONG left = count, n;
        while (left > 0 && (n = cu_read(&in, in.line ? in.line : (in.line = cu_alloc(4096)), left < 4096 ? left : 4096)) > 0) {
            cu_put(in.line, n);
            left -= n;
            if (cu_break()) break;
        }
    } else {
        char *line;
        LONG n, k = 0;
        int nl;
        while (k < count && (n = cu_line(&in, &line, &nl)) >= 0) {
            cu_put(line, n);
            if (nl) cu_putc('\n');
            if (!(++k & 255) && cu_break()) break;
        }
    }
    if (in.err) cu_fault(5, path);
    cu_close(&in);
    return cu_rc >= 20;
}

int main(void)
{
    char *v;
    int c, i, ok, quiet = 0, verbose = 0;
    (void)version;
    if (cu_args("head [-n N | -N] [-c N] [-qv] [file | pattern | - ...]") < 0) return cu_rc;
    while ((c = cu_opt("n:c:qv#", &v)) >= 0) switch (c) {
        case 'n': case '#': case 'c':
            count = cu_num(v, &ok);
            if (!ok || count < 0) { cu_err(10, v, "not a count"); return cu_rc; }
            bytes = c == 'c';
            break;
        case 'q': quiet = 1; verbose = 0; break;
        case 'v': verbose = 1; quiet = 0; break;
        default: return cu_rc;
    }
    names = verbose || (!quiet && cu_argc > 1);
    if (!cu_argc) one(NULL, NULL);
    for (i = 0; i < cu_argc && cu_rc < 20; i++) cu_each(cu_argv[i], one, NULL);
    cu_flush();
    return cu_rc;
}
