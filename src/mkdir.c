/* mkdir - make drawers.
 *
 *   mkdir [-p] drawer ...
 *
 * Without -p each drawer is made directly: its parent must exist, and one
 * that exists already is an error, as in Unix.
 *   -p  make every missing drawer along the path (`mkdir -p a/b/c` makes
 *       a, a/b, a/b/c); one that exists already is fine. A part of the
 *       path that is a file is an error, named at that part. The volume
 *       (the part before ':') is never made, only gone into.
 * Names are taken as typed, never as patterns: `mkdir "#?"` makes a
 * drawer called #?.
 *
 * Return code 0, 10 when a drawer could not be made (the others still
 * are), 20 Ctrl-C. */
#include "cu.h"

const char *cu_name = "mkdir";
static const char version[] = "$VER: mkdir 0.1b2 (16.10.2026)";

static struct FileInfoBlock *fib;

static int isdir(const char *path)
{
    BPTR l = Lock((STRPTR)path, SHARED_LOCK);
    int r = 0;
    if (l) {
        r = Examine(l, fib) && fib->fib_DirEntryType > 0;
        UnLock(l);
    }
    return r;
}

/* one drawer; exist: an existing drawer is fine (-p). 0: failed, said. */
static int make(const char *path, int exist)
{
    BPTR l = CreateDir((STRPTR)path);
    LONG e;
    if (l) { UnLock(l); return 1; }
    e = IoErr();
    if (exist && e == ERROR_OBJECT_EXISTS) {
        if (isdir(path)) return 1;
        cu_err(10, path, "exists and is not a drawer");
        return 0;
    }
    SetIoErr(e);
    cu_fault(10, path);
    return 0;
}

/* -p: each part in turn; empty parts (a leading or doubled /, the
 * parent) are stepped over, not made */
static void parents(const char *path)
{
    static char w[CU_PATH];
    int i, start = 0, len = strlen(path);
    if (len >= CU_PATH) { cu_err(10, path, "path too long"); return; }
    strcpy(w, path);
    for (i = 0; i < len; i++) {
        if (w[i] == ':') start = i + 1;
        else if (w[i] == '/') {
            if (i > start) {
                w[i] = 0;
                if (!make(w, 1)) return;
                w[i] = '/';
            }
            start = i + 1;
        }
    }
    if (len > start) make(w, 1);
}

int main(void)
{
    char *v;
    int c, i, o_p = 0;
    (void)version;
    if (cu_args("mkdir [-p] drawer ...") < 0) return cu_rc;
    while ((c = cu_opt("p", &v)) >= 0) switch (c) {
        case 'p': o_p = 1; break;
        default: return cu_rc;
    }
    if (!cu_argc) { cu_err(10, NULL, "no drawer named (mkdir ? for usage)"); return cu_rc; }
    if (!(fib = AllocDosObject(DOS_FIB, NULL))) { cu_err(20, NULL, "out of memory"); return cu_rc; }
    for (i = 0; i < cu_argc && !cu_break(); i++) {
        if (o_p) parents(cu_argv[i]);
        else make(cu_argv[i], 0);
    }
    FreeDosObject(DOS_FIB, fib);
    return cu_rc;
}
