/* cu_walk.c - a tree walked without recursion (cu.h): find, du.
 * Each drawer on the way down is a frame on the heap with its entries,
 * read whole and sorted before any of them is given out, so no lock is
 * held between calls and the stack use is the same at any depth. */
#include <stdlib.h>
#include <dos/exall.h>
#include "cu.h"

typedef struct {
    LONG name;                   /* offset into the frame's names */
    LONG type, size, prot;
    struct DateStamp date;
} WEnt;

typedef struct Frame {
    struct Frame *up;
    CuEnt self;                  /* the drawer, for its post call */
    char *path;
    WEnt *e;
    char *names;
    int n, i;
} Frame;

int cu_isdir(LONG type)
{
    return type > 0 && type != ST_SOFTLINK && type != ST_LINKDIR;
}

static char *sortnames;

static int cmpent(const void *a, const void *b)
{
    return cu_natcmp(sortnames + ((WEnt *)a)->name, sortnames + ((WEnt *)b)->name);
}

/* the drawer's entries into f (none, and a message, when it cannot be read) */
static void readdrawer(Frame *f)
{
    struct ExAllControl *eac = NULL;
    struct ExAllData *buf = NULL, *ed;
    LONG more = 0, ncap = 0, nlen = 0;
    int cap = 0;
    BPTR lock = Lock((STRPTR)f->path, SHARED_LOCK);
    if (!lock) { cu_fault(5, f->path); return; }
    if (!(eac = AllocDosObject(DOS_EXALLCONTROL, NULL)) || !(buf = cu_alloc(4096))) {
        cu_err(20, NULL, "out of memory");
        goto done;
    }
    eac->eac_LastKey = 0;
    eac->eac_MatchString = NULL;
    do {
        more = ExAll(lock, buf, 4096, ED_DATE, eac);
        if (!more && IoErr() != ERROR_NO_MORE_ENTRIES) { cu_fault(5, f->path); break; }
        if (!eac->eac_Entries) continue;
        for (ed = buf; ed; ed = ed->ed_Next) {
            LONG l = strlen((char *)ed->ed_Name) + 1;
            WEnt *w;
            if (f->n == cap) {
                WEnt *t = cu_alloc((cap = cap ? cap * 2 : 64) * sizeof *t);
                if (!t) { cu_err(20, NULL, "out of memory"); goto end; }
                if (f->e) { memcpy(t, f->e, f->n * sizeof *t); cu_free(f->e); }
                f->e = t;
            }
            if (nlen + l > ncap) {
                char *t = cu_alloc(ncap = (nlen + l) * 2 + 512);
                if (!t) { cu_err(20, NULL, "out of memory"); goto end; }
                if (f->names) { memcpy(t, f->names, nlen); cu_free(f->names); }
                f->names = t;
            }
            w = &f->e[f->n++];
            w->name = nlen;
            memcpy(f->names + nlen, ed->ed_Name, l);
            nlen += l;
            w->type = ed->ed_Type;
            w->size = ed->ed_Size;
            w->prot = ed->ed_Prot;
            w->date.ds_Days = ed->ed_Days;
            w->date.ds_Minute = ed->ed_Mins;
            w->date.ds_Tick = ed->ed_Ticks;
        }
    } while (more);
end:
    if (more) ExAllEnd(lock, buf, 4096, ED_DATE, eac);
    if (f->n > 1) { sortnames = f->names; qsort(f->e, f->n, sizeof *f->e, cmpent); }
done:
    cu_free(buf);
    if (eac) FreeDosObject(DOS_EXALLCONTROL, eac);
    UnLock(lock);
}

/* a frame for the drawer e (path and name copied) */
static Frame *push(Frame *up, CuEnt *e)
{
    int lp = strlen(e->path) + 1, ln = strlen(e->name) + 1;
    Frame *f = cu_alloc(sizeof *f + lp + ln);
    if (!f) { cu_err(20, NULL, "out of memory"); return NULL; }
    memset(f, 0, sizeof *f);
    f->up = up;
    f->self = *e;
    f->path = (char *)(f + 1);
    memcpy(f->path, e->path, lp);
    f->self.path = f->path;
    f->self.name = f->path + lp;
    memcpy((char *)f->self.name, e->name, ln);
    readdrawer(f);
    return f;
}

static Frame *pop(Frame *f)
{
    Frame *up = f->up;
    cu_free(f->e);
    cu_free(f->names);
    cu_free(f);
    return up;
}

int cu_walk(const char *start, int flags, int maxdepth, int (*fn)(CuEnt *e, void *u), void *u)
{
    static char cur[CU_PATH];
    struct FileInfoBlock *fib;
    CuEnt e;
    Frame *top = NULL;
    BPTR lock;
    int r;
    char *fp;

    if (!(lock = Lock((STRPTR)start, SHARED_LOCK))) { cu_fault(5, start); return 0; }
    if (!(fib = AllocDosObject(DOS_FIB, NULL))) { UnLock(lock); cu_err(20, NULL, "out of memory"); return CU_STOP; }
    if (!Examine(lock, fib)) { cu_fault(5, start); FreeDosObject(DOS_FIB, fib); UnLock(lock); return 0; }
    UnLock(lock);
    memset(&e, 0, sizeof e);
    e.path = start;
    fp = FilePart((STRPTR)start);
    e.name = *fp ? fp : (char *)fib->fib_FileName;    /* "RAM:", "a/", "": its own name */
    e.type = fib->fib_DirEntryType;
    e.size = fib->fib_Size;
    e.prot = fib->fib_Protection;
    e.date = fib->fib_Date;
    r = fn(&e, u);
    if (r == CU_STOP || !cu_isdir(e.type)) { FreeDosObject(DOS_FIB, fib); return r == CU_STOP ? CU_STOP : 0; }
    if (r == CU_SKIP || maxdepth == 0) {
        r = 0;
        if (flags & CU_WALK_POST) { e.post = 1; r = fn(&e, u); }
        FreeDosObject(DOS_FIB, fib);
        return r == CU_STOP ? CU_STOP : 0;
    }
    top = push(NULL, &e);
    FreeDosObject(DOS_FIB, fib);
    if (!top) return CU_STOP;

    while (top) {
        if (cu_break() || cu_rc >= 20) goto stop;
        if (top->i < top->n) {
            WEnt *w = &top->e[top->i++];
            memset(&e, 0, sizeof e);
            e.name = top->names + w->name;
            if (!cu_join(cur, top->path, e.name)) { cu_err(5, e.name, "path too long"); continue; }
            e.path = cur;
            e.type = w->type;
            e.size = w->size;
            e.prot = w->prot;
            e.date = w->date;
            e.depth = top->self.depth + 1;
            if ((r = fn(&e, u)) == CU_STOP) goto stop;
            if (cu_isdir(e.type)) {
                if (r != CU_SKIP && (maxdepth < 0 || e.depth < maxdepth)) {
                    Frame *f = push(top, &e);
                    if (!f) goto stop;
                    top = f;
                } else if (flags & CU_WALK_POST) {
                    e.post = 1;
                    if (fn(&e, u) == CU_STOP) goto stop;
                }
            }
        } else {
            if (flags & CU_WALK_POST) {
                top->self.post = 1;
                if (fn(&top->self, u) == CU_STOP) goto stop;
            }
            top = pop(top);
        }
    }
    return 0;
stop:
    while (top) top = pop(top);
    return CU_STOP;
}
