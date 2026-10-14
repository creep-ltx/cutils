/* tail - the last lines (or bytes) of files.
 *
 *   tail [-n N | -N | -n +N | +N] [-c N] [-f] [-qv] [file | pattern | - ...]
 *
 * 10 lines unless told. A file is read from its end backwards, so the last
 * lines of a big file come at once; the input (a pipe) is read through,
 * keeping the last N lines. Several files: each under "==> name <==".
 *   -n N, -N  the last N lines   -n +N  from line N on   -c N  the last N
 *   bytes (+N: from byte N)   -f  follow: wait for more (a growing log)
 *   until Ctrl-C   -q never the names   -v always the names
 *
 * Return code 0, 5 when a file could not be read, 10 bad arguments,
 * 20 Ctrl-C (also how -f ends: then 0). */
#include "cu.h"

const char *cu_name = "tail";
static const char version[] = "$VER: tail 0.1b2 (14.10.2026)";

static LONG count = 10;
static int bytes, from, follow, names, first = 1, nfiles;
static char *lastname;

static void header(const char *name)
{
    if (!names) return;
    if (!first) cu_putc('\n');
    cu_puts("==> "); cu_puts(name); cu_puts(" <==\n");
    first = 0;
    lastname = (char *)name;
}

/* bytes from pos to the end, out */
static void copyout(BPTR fh, LONG pos, char *buf, LONG size)
{
    LONG n;
    Seek(fh, pos, OFFSET_BEGINNING);
    while ((n = Read(fh, buf, size)) > 0) { cu_put(buf, n); if (cu_break()) break; }
}

/* a file that can seek: from the end backwards, counting line ends */
static int fromend(BPTR fh, char *buf, LONG size)
{
    LONG end, pos, want = count, start = 0;
    if (Seek(fh, 0, OFFSET_END) < 0) return 0;
    end = Seek(fh, 0, OFFSET_CURRENT);
    if (end < 0) return 0;
    if (bytes) { copyout(fh, end > count ? end - count : 0, buf, size); return 1; }
    pos = end;
    if (pos > 0) {                       /* a last line without \n counts too */
        char c;
        Seek(fh, pos - 1, OFFSET_BEGINNING);
        if (Read(fh, &c, 1) == 1 && c == '\n') want++;
    }
    while (pos > 0 && want > 0) {
        LONG k = pos > size ? size : pos, i;
        pos -= k;
        Seek(fh, pos, OFFSET_BEGINNING);
        if (Read(fh, buf, k) != k) return 0;
        for (i = k - 1; i >= 0; i--)
            if (buf[i] == '\n' && --want == 0) { start = pos + i + 1; break; }
    }
    copyout(fh, want > 0 ? 0 : start, buf, size);
    return 1;
}

/* the input, or -n +N: read through */
static void through(CuIn *in)
{
    if (from) {
        LONG skip = count > 0 ? count - 1 : 0, n;
        if (bytes) {
            char t[512];
            while (skip > 0 && (n = cu_read(in, t, skip < 512 ? skip : 512)) > 0) skip -= n;
        } else {
            char *line;
            int nl;
            while (skip > 0 && cu_line(in, &line, &nl) >= 0) skip--;
        }
        if (in->pos < in->len) cu_put(in->buf + in->pos, in->len - in->pos);
        in->pos = in->len;
        while ((n = Read(in->fh, in->buf, in->size)) > 0) { cu_put(in->buf, n); if (cu_break()) break; }
        return;
    }
    if (bytes) {
        /* a ring of the last count bytes */
        char *ring = count ? cu_alloc(count) : NULL;
        LONG head = 0, have = 0, n, i;
        if (count && !ring) { cu_err(20, NULL, "out of memory"); return; }
        while ((n = cu_read(in, in->line ? in->line : (in->line = cu_alloc(4096)), 4096)) > 0) {
            for (i = 0; i < n && count; i++) { ring[head] = in->line[i]; head = (head + 1) % count; if (have < count) have++; }
            if (cu_break()) break;
        }
        if (have < count) cu_put(ring, have);
        else { cu_put(ring + head, count - head); cu_put(ring, head); }
        cu_free(ring);
    } else {
        /* the last count lines, kept round */
        typedef struct { char *s; LONG n, size; int nl; } L;
        L *ring = count ? cu_alloc(count * sizeof *ring) : NULL;
        LONG head = 0, have = 0, n, i, k = 0;
        char *line;
        int nl;
        if (count && !ring) { cu_err(20, NULL, "out of memory"); return; }
        if (ring) memset(ring, 0, count * sizeof *ring);
        while ((n = cu_line(in, &line, &nl)) >= 0) {
            L *l;
            if (!count) continue;
            l = &ring[head];
            if (l->size < n + 1) {
                cu_free(l->s);
                l->size = n + 32;
                if (!(l->s = cu_alloc(l->size))) { cu_err(20, NULL, "out of memory"); break; }
            }
            memcpy(l->s, line, n); l->n = n; l->nl = nl;
            head = (head + 1) % count;
            if (have < count) have++;
            if (!(++k & 255) && cu_break()) break;
        }
        for (i = 0; i < have; i++) {
            L *l = &ring[(head - have + i + count) % count];
            cu_put(l->s, l->n);
            if (l->nl) cu_putc('\n');
        }
        for (i = 0; i < count; i++) cu_free(ring[i].s);
        cu_free(ring);
    }
}

typedef struct { char *name; BPTR fh; LONG pos; } Follow;
static Follow fl[32];

static int one(const char *path, void *u)
{
    CuIn in;
    (void)u;
    if (!cu_open(&in, path)) return 0;
    nfiles++;
    header(path ? path : "standard input");
    if (in.own && !from && fromend(in.fh, in.buf, in.size)) ;
    else {
        if (in.own) Seek(in.fh, 0, OFFSET_BEGINNING);
        through(&in);
    }
    cu_flush();
    if (follow && in.own && nfiles <= 32) {
        /* keep it open for -f */
        Follow *f = &fl[nfiles - 1];
        f->name = cu_strdup(path);
        f->fh = in.fh;
        f->pos = Seek(in.fh, 0, OFFSET_CURRENT);
        in.own = 0;
    }
    cu_close(&in);
    return cu_rc >= 20;
}

/* -f: look every quarter second for more */
static void dofollow(void)
{
    char buf[4096];
    int i, n = nfiles < 32 ? nfiles : 32;
    for (;;) {
        for (i = 0; i < n; i++) {
            Follow *f = &fl[i];
            LONG k, end;
            if (!f->fh) continue;
            Seek(f->fh, 0, OFFSET_END);
            end = Seek(f->fh, 0, OFFSET_CURRENT);
            if (end < f->pos) f->pos = 0;            /* cut short: from the start */
            if (end == f->pos) continue;
            Seek(f->fh, f->pos, OFFSET_BEGINNING);
            if (names && lastname != f->name) header(f->name);
            while ((k = Read(f->fh, buf, sizeof buf)) > 0) { cu_put(buf, k); f->pos += k; }
            cu_flush();
        }
        if (SetSignal(0, SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C) break;
        Delay(12);
    }
    for (i = 0; i < n; i++) if (fl[i].fh) Close(fl[i].fh);
}

int main(void)
{
    char *v;
    int c, i, ok, quiet = 0, verbose = 0;
    (void)version;
    if (cu_args("tail [-n N | -N | -n +N] [-c N] [-f] [-qv] [file | pattern | - ...]") < 0) return cu_rc;
    /* +N alone, as the old tail: tail +5 file = tail -n +5 file */
    if (cu_argc && cu_argv[0][0] == '+' && cu_argv[0][1] >= '0' && cu_argv[0][1] <= '9') {
        from = 1;
        count = cu_num(cu_argv[0] + 1, &ok);
        if (!ok) { cu_err(10, cu_argv[0], "not a count"); return cu_rc; }
        cu_argv++; cu_argc--;
    }
    while ((c = cu_opt("n:c:fFqv#", &v)) >= 0) switch (c) {
        case 'n': case '#': case 'c':
            from = *v == '+';
            count = cu_num(v + from, &ok);
            if (!ok || count < 0) { cu_err(10, v, "not a count"); return cu_rc; }
            bytes = c == 'c';
            break;
        case 'f': case 'F': follow = 1; break;
        case 'q': quiet = 1; verbose = 0; break;
        case 'v': verbose = 1; quiet = 0; break;
        default: return cu_rc;
    }
    names = verbose || (!quiet && (cu_argc > 1 || (cu_argc == 1 && cu_haswild(cu_argv[0]))));
    if (!cu_argc) one(NULL, NULL);
    for (i = 0; i < cu_argc && cu_rc < 20; i++) cu_each(cu_argv[i], one, NULL);
    if (follow && cu_rc < 20 && nfiles) dofollow();
    cu_flush();
    return cu_rc == 20 && follow ? 0 : cu_rc;
}
