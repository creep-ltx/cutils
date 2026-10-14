/* cu_in.c - text read fast, a line at a time (cu.h). */
#include <stdlib.h>
#include "cu.h"

/* ---- reading ---- */

#define INBUF 16384

int cu_open(CuIn *in, const char *path)
{
    memset(in, 0, sizeof *in);
    if (!path || (path[0] == '-' && !path[1])) in->fh = Input();
    else {
        BPTR l = Lock((STRPTR)path, SHARED_LOCK);
        if (l) {
            struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
            int dir = fib && Examine(l, fib) && fib->fib_DirEntryType > 0 && fib->fib_DirEntryType != ST_SOFTLINK;
            if (fib) FreeDosObject(DOS_FIB, fib);
            UnLock(l);
            if (dir) { cu_err(5, path, "is a drawer"); return 0; }
        }
        if (!(in->fh = Open((STRPTR)path, MODE_OLDFILE))) { cu_fault(5, path); return 0; }
        in->own = 1;
    }
    if (!(in->buf = cu_alloc(INBUF))) { cu_close(in); cu_err(20, NULL, "out of memory"); return 0; }
    in->size = INBUF;
    return 1;
}

static int fill(CuIn *in)
{
    LONG n;
    if (in->eof) return 0;
    n = Read(in->fh, in->buf, in->size);
    if (n <= 0) { in->eof = 1; if (n < 0) in->err = 1; in->len = in->pos = 0; return 0; }
    in->len = n; in->pos = 0;
    return 1;
}

LONG cu_read(CuIn *in, char *to, LONG n)
{
    LONG got = 0;
    while (got < n) {
        LONG k;
        if (in->pos >= in->len && !fill(in)) break;
        k = in->len - in->pos;
        if (k > n - got) k = n - got;
        memcpy(to + got, in->buf + in->pos, k);
        in->pos += k; got += k;
    }
    return got;
}

LONG cu_line(CuIn *in, char **line, int *nl)
{
    LONG have = 0;
    for (;;) {
        char *s, *e;
        LONG k;
        if (in->pos >= in->len && !fill(in)) {
            if (!have) return -1;
            *line = in->line; *nl = 0;
            in->line[have] = 0;
            return have;
        }
        s = in->buf + in->pos;
        k = in->len - in->pos;
        e = memchr(s, '\n', k);
        if (e && !have) {                  /* the common case: no copy */
            *e = 0;                        /* the \n, so the line is a C string */
            *line = s; *nl = 1;
            in->pos += e - s + 1;
            return e - s;
        }
        if (e) k = e - s;
        if (have + k + 1 > in->lsize) {
            LONG ns = (have + k + 1) * 2;
            char *t = cu_alloc(ns);
            if (!t) { in->err = 1; return -1; }
            if (in->line) { memcpy(t, in->line, have); cu_free(in->line); }
            in->line = t; in->lsize = ns;
        }
        memcpy(in->line + have, s, k);
        have += k;
        in->pos += k;
        if (e) {
            in->pos++;
            in->line[have] = 0;
            *line = in->line; *nl = 1;
            return have;
        }
    }
}

void cu_close(CuIn *in)
{
    if (in->own && in->fh) Close(in->fh);
    cu_free(in->buf);
    cu_free(in->line);
    memset(in, 0, sizeof *in);
}
