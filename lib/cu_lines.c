/* cu_lines.c - many lines kept in memory (cu.h): the text in big chunks,
 * one pointer per line, no malloc per line; and a stable merge sort over
 * the pointers, bottom-up (no recursion, so any count on a 4 KB stack). */
#include <stdlib.h>
#include "cu.h"

#define CHUNK 65536

typedef struct Chunk { struct Chunk *next; } Chunk;

char *cu_lines_add(CuLines *l, const char *s, LONG n, LONG pre)
{
    char *d;
    LONG need;
    pre = (pre + 3) & ~3;
    need = (pre + n + 1 + 3) & ~3;            /* each record on a long word */
    if (l->n == l->max) {
        LONG nm = l->max ? l->max * 2 : 1024;
        char **t = cu_alloc(nm * sizeof *t);
        if (!t) return NULL;
        if (l->v) { memcpy(t, l->v, l->n * sizeof *t); cu_free(l->v); }
        l->v = t; l->max = nm;
    }
    if (l->used + need > l->size) {           /* a new chunk (a long line: its own) */
        LONG sz = need > CHUNK - (LONG)sizeof(Chunk) ? need + sizeof(Chunk) : CHUNK;
        Chunk *c = cu_alloc(sz);
        if (!c) return NULL;
        c->next = l->chunks; l->chunks = c;
        l->mem = (char *)(c + 1); l->used = 0; l->size = sz - sizeof(Chunk);
    }
    d = l->mem + l->used + pre;
    memcpy(d, s, n);
    d[n] = 0;
    l->used += need;
    l->v[l->n++] = d;
    return d;
}

void cu_lines_free(CuLines *l)
{
    Chunk *c = (Chunk *)l->chunks, *nx;
    for (; c; c = nx) { nx = c->next; cu_free(c); }
    cu_free(l->v);
    memset(l, 0, sizeof *l);
}

/* runs of 8 by insertion, then merged in passes of doubling width */
int cu_msort(char **v, LONG n, int (*cmp)(const char *a, const char *b))
{
    char **t, **src = v, **dst, **sw;
    LONG w, i, j, lo, mid, hi, k;
    for (lo = 0; lo < n; lo += 8) {
        hi = lo + 8 < n ? lo + 8 : n;
        for (i = lo + 1; i < hi; i++) {
            char *x = v[i];
            for (j = i; j > lo && cmp(v[j - 1], x) > 0; j--) v[j] = v[j - 1];
            v[j] = x;
        }
    }
    if (n <= 8) return 1;
    if (!(t = cu_alloc(n * sizeof *t))) return 0;
    dst = t;
    for (w = 8; w < n; w *= 2) {
        for (lo = 0; lo < n; lo += 2 * w) {
            mid = lo + w < n ? lo + w : n;
            hi = lo + 2 * w < n ? lo + 2 * w : n;
            i = lo; j = mid; k = lo;
            if (mid < hi && cmp(src[mid - 1], src[mid]) <= 0) {   /* already in order */
                memcpy(dst + lo, src + lo, (hi - lo) * sizeof *t);
                continue;
            }
            while (i < mid && j < hi) dst[k++] = cmp(src[i], src[j]) <= 0 ? src[i++] : src[j++];
            while (i < mid) dst[k++] = src[i++];
            while (j < hi) dst[k++] = src[j++];
        }
        sw = src; src = dst; dst = sw;
        if (cu_break()) break;
    }
    if (src != v) memcpy(v, src, n * sizeof *v);
    cu_free(t);
    return 1;
}
