/* cu_tree.c - files and trees for cp, mv and rm (cu.h): links never
 * followed, drawers read whole before anything in them changes, work
 * lists on the heap instead of recursion (the Shell's stack is often
 * 4 KB), metadata carried like Copy CLONE. */
#include "cu.h"

int cu_isroot(const char *path)
{
    int l = strlen(path);
    if (l && path[l - 1] == '/') l--;
    return !l || path[l - 1] == ':';
}

void cu_trimslash(char *path)
{
    int l = strlen(path);
    if (l > 1 && path[l - 1] == '/' && path[l - 2] != '/' && path[l - 2] != ':') path[l - 1] = 0;
}

/* A soft link is asked about with ReadLink() on its drawer: Lock() would
 * follow it (dos.library resolves links inside Lock). */
int cu_lstat(const char *path, struct FileInfoBlock *fib)
{
    static char dir[CU_PATH], t[256];     /* static: the stack is often 4 KB */
    const char *name = (const char *)FilePart((STRPTR)path);
    BPTR l;
    LONG r, e;
    if (*name && strlen(path) < CU_PATH) {
        int n = (const char *)PathPart((STRPTR)path) - path;
        memcpy(dir, path, n);
        dir[n] = 0;
        if ((l = Lock(dir, SHARED_LOCK))) {
            struct MsgPort *port = ((struct FileLock *)BADDR(l))->fl_Task;
            SetIoErr(0);
            r = ReadLink(port, l, (STRPTR)name, t, sizeof t);
            e = IoErr();
            UnLock(l);
            /* a link: 3.2 gives its length (-2: buffer too small, -1 and
             * IoErr() for none), older ones DOSTRUE / DOSFALSE */
            if (r >= 1 || r == -2 || (r == -1 && !e)) {
                memset(fib, 0, sizeof *fib);
                fib->fib_DirEntryType = ST_SOFTLINK;
                strncpy((char *)fib->fib_FileName, name, sizeof fib->fib_FileName - 1);
                return 1;
            }
        }
    }
    if (!(l = Lock((STRPTR)path, SHARED_LOCK))) return 0;
    if ((r = Examine(l, fib) != 0)) UnLock(l);
    else {
        e = IoErr();
        UnLock(l);
        SetIoErr(e);
    }
    return r;
}

void cu_freeents(CuEnt *e)
{
    while (e) {
        CuEnt *n = e->next;
        cu_free(e);
        e = n;
    }
}

CuEnt *cu_scan(const char *dir, int *ok)
{
    struct FileInfoBlock *fib;
    CuEnt *head = NULL, *e;
    BPTR l;
    LONG err = 0;
    *ok = 1;
    if (!(fib = AllocDosObject(DOS_FIB, NULL))) { cu_err(20, NULL, "out of memory"); *ok = 0; return NULL; }
    if (!(l = Lock((STRPTR)dir, SHARED_LOCK)) || !Examine(l, fib)) {
        cu_fault(10, dir);
        if (l) UnLock(l);
        FreeDosObject(DOS_FIB, fib);
        *ok = 0;
        return NULL;
    }
    while (ExNext(l, fib)) {
        int nl = strlen((char *)fib->fib_FileName), cl = strlen((char *)fib->fib_Comment);
        if (!nl) continue;                       /* a blank name: never walked into */
        if (!(e = cu_alloc(sizeof *e + nl + (cl ? cl + 1 : 0)))) { err = ERROR_NO_FREE_STORE; break; }
        strcpy(e->name, (char *)fib->fib_FileName);
        e->note = NULL;
        if (cl) { e->note = e->name + nl + 1; strcpy(e->note, (char *)fib->fib_Comment); }
        e->type = fib->fib_DirEntryType;
        e->prot = fib->fib_Protection;
        e->date = fib->fib_Date;
        e->next = head;
        head = e;
    }
    if (!err) err = IoErr();
    UnLock(l);
    FreeDosObject(DOS_FIB, fib);
    if (err != ERROR_NO_MORE_ENTRIES) {
        SetIoErr(err);
        cu_fault(10, dir);
        *ok = 0;
    }
    return head;
}

/* ---- copying ---- */

#define BUFSIZE 131072           /* 64 KB or less goes through exFAT's sector cache (cp.e 0.1.3) */

static char *cbuf;
static LONG csize;

int cu_copyfile(const char *src, const char *dst, LONG prot, const struct DateStamp *ds, const char *note)
{
    BPTR in, out;
    LONG n;
    int ok = 1;
    if (!cbuf) {
        if ((cbuf = cu_alloc(BUFSIZE))) csize = BUFSIZE;
        else if ((cbuf = cu_alloc(8192))) csize = 8192;
        else { cu_err(20, NULL, "out of memory"); return 0; }
    }
    if (!(in = Open((STRPTR)src, MODE_OLDFILE))) { cu_fault(10, src); return 0; }
    if (!(out = Open((STRPTR)dst, MODE_NEWFILE))) { cu_fault(10, dst); Close(in); return 0; }
    for (;;) {
        if (cu_break()) { ok = 0; break; }
        n = Read(in, cbuf, csize);
        if (n < 0) { cu_fault(10, src); ok = 0; break; }
        if (!n) break;
        if (Write(out, cbuf, n) != n) { cu_fault(10, dst); ok = 0; break; }
    }
    Close(in);
    if (!Close(out) && ok) { cu_fault(10, dst); ok = 0; }
    if (!ok) { DeleteFile((STRPTR)dst); return 0; }
    /* best effort: a file system that cannot keep these still has the copy */
    SetProtection((STRPTR)dst, prot);
    if (ds) SetFileDate((STRPTR)dst, (struct DateStamp *)ds);
    if (note && *note) SetComment((STRPTR)dst, (STRPTR)note);
    return 1;
}

int cu_clonedir(const char *src, const char *dst)
{
    struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
    BPTR l;
    int have = 0;
    if (fib && (l = Lock((STRPTR)src, SHARED_LOCK))) {
        have = Examine(l, fib) != 0;
        UnLock(l);
    }
    if (!(l = CreateDir((STRPTR)dst))) {
        LONG e = IoErr();
        if (fib) FreeDosObject(DOS_FIB, fib);
        SetIoErr(e);
        return 0;
    }
    UnLock(l);
    if (have) {
        SetProtection((STRPTR)dst, fib->fib_Protection);
        if (fib->fib_Comment[0]) SetComment((STRPTR)dst, fib->fib_Comment);
    }
    if (fib) FreeDosObject(DOS_FIB, fib);
    return 1;
}

/* ---- work lists ---- */

typedef struct Job {
    struct Job *next;
    char *dst;                   /* NULL in cu_deltree */
    char src[1];
} Job;

static Job *job(const char *s, const char *d)
{
    int sl = strlen(s);
    Job *j = cu_alloc(sizeof *j + sl + (d ? strlen(d) + 1 : 0));
    if (!j) { cu_err(20, NULL, "out of memory"); return NULL; }
    strcpy(j->src, s);
    j->dst = NULL;
    if (d) { j->dst = j->src + sl + 1; strcpy(j->dst, d); }
    j->next = NULL;
    return j;
}

static void freejobs(Job *j)
{
    while (j) {
        Job *n = j->next;
        cu_free(j);
        j = n;
    }
}

int cu_copytree(const char *src, const char *dst,
                int (*dir)(const char *s, const char *d, void *u),
                int (*file)(const char *s, const char *d, CuEnt *e, void *u), void *u)
{
    Job *head, *tail, *j;
    static char cs[CU_PATH], cd[CU_PATH];
    int all = 1, stop = 0;
    if (!(head = tail = job(src, dst))) return 0;
    while ((j = head) && !stop) {
        CuEnt *list, *e;
        int ok, r;
        head = j->next;
        if (!head) tail = NULL;
        if (cu_break()) { stop = 1; all = 0; }
        else if ((r = dir(j->src, j->dst, u)) <= 0) { all = 0; stop = r < 0; }
        else {
            list = cu_scan(j->src, &ok);
            if (!ok) all = 0;
            for (e = list; e && !stop; e = e->next) {
                if (cu_break()) { stop = 1; all = 0; break; }
                if (!cu_join(cs, j->src, e->name) || !cu_join(cd, j->dst, e->name)) {
                    cu_err(10, e->name, "path too long");
                    all = 0;
                    continue;
                }
                if (CU_ISDIR(e->type)) {
                    Job *n = job(cs, cd);
                    if (!n) { stop = 1; all = 0; break; }
                    if (tail) tail->next = n; else head = n;
                    tail = n;
                } else if ((r = file(cs, cd, e, u)) <= 0) { all = 0; stop = r < 0; }
            }
            cu_freeents(list);
        }
        cu_free(j);
    }
    freejobs(head);
    return all;
}

int cu_deltree(const char *top, int (*del)(const char *path, LONG type, void *u), void *u)
{
    Job *head, *tail, *lifo = NULL, *j;
    static char w[CU_PATH];
    int all = 1, stop = 0;
    if (!(head = tail = job(top, NULL))) return 0;
    while ((j = head)) {                         /* scan every drawer, top down */
        CuEnt *list, *e;
        int ok;
        head = j->next;
        if (!head) tail = NULL;
        j->next = lifo;                          /* below its children-to-be */
        lifo = j;
        if (cu_break()) { stop = 1; break; }
        list = cu_scan(j->src, &ok);
        if (!ok) all = 0;
        for (e = list; e; e = e->next) {
            if (cu_break()) { stop = 1; break; }
            if (!cu_join(w, j->src, e->name)) { cu_err(10, e->name, "path too long"); all = 0; continue; }
            if (CU_ISDIR(e->type)) {
                Job *n = job(w, NULL);
                if (!n) { stop = 1; break; }
                if (tail) tail->next = n; else head = n;
                tail = n;
            } else if (!del(w, e->type, u)) all = 0;
        }
        cu_freeents(list);
        if (stop) break;
    }
    freejobs(head);
    for (j = lifo; j && !stop; j = j->next) {    /* then the drawers, deepest first */
        if (cu_break()) { stop = 1; break; }
        if (!del(j->src, ST_USERDIR, u)) all = 0;
    }
    freejobs(lifo);
    return all && !stop;
}

/* ---- the list at the end ---- */

static Job *skips, *skiptail;

void cu_skip(const char *path)
{
    Job *j = job(path, NULL);
    if (cu_rc < 5) cu_rc = 5;
    if (!j) return;
    if (skiptail) skiptail->next = j; else skips = j;
    skiptail = j;
}

static void eput(const char *s)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    Write(me->pr_CES ? me->pr_CES : Output(), (APTR)s, strlen(s));
}

void cu_skipped(const char *title)
{
    Job *j;
    if (!skips) return;
    cu_flush();
    eput(title);
    eput(":\n");
    for (j = skips; j; j = j->next) { eput("  "); eput(j->src); eput("\n"); }
    freejobs(skips);
    skips = skiptail = NULL;
}
