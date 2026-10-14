/* ln - make links (MakeLink).
 *
 *   ln [-sfv] TARGET [LINK]
 *   ln [-sfv] TARGET ... DRAWER
 *
 * A hard link by default, as Unix: a second name for the same file, on
 * the same volume. -s makes a soft link instead: a name that holds the
 * path TARGET, which may be on any volume and need not exist yet (it is
 * kept as typed, so a relative TARGET is taken from the link's drawer).
 * No LINK: a link of the same name in the current drawer. LINK an
 * existing drawer, or several targets: links of their names in it.
 *   -s  soft links   -f  replace a LINK that is there (never a drawer)
 *   -v  say each link made: "link -> target"
 * Hard links across volumes are refused (use -s), and so are hard links
 * to a volume or assign root ("RAM:": a loop no tool could walk). A hard link to a drawer works on the same volume
 * (the Amiga allows it); CUtils never walks into one.
 *
 * Return code 0, 10 when a link could not be made or bad arguments,
 * 20 Ctrl-C. */
#include "cu.h"

const char *cu_name = "ln";
static const char version[] = "$VER: ln 0.1b1 (14.10.2026)";

static int o_soft, o_force, o_verbose;

static int isdrawer(const char *p)   /* an existing drawer (not a link to one) */
{
    struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
    int r = fib && cu_lstat(p, fib) && CU_ISDIR(fib->fib_DirEntryType);
    if (fib) FreeDosObject(DOS_FIB, fib);
    return r;
}

static void link1(const char *target, const char *link)
{
    struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
    BPTR t = 0, dir = 0;
    LONG ok;
    char parent[CU_PATH];
    int n;
    if (!fib) { cu_err(20, NULL, "out of memory"); return; }
    if (cu_lstat(link, fib)) {                         /* something is there */
        if (!o_force) { cu_err(10, link, "is already there (-f replaces it)"); goto done; }
        if (CU_ISDIR(fib->fib_DirEntryType)) { cu_err(10, link, "is a drawer: not replaced"); goto done; }
        if (fib->fib_DirEntryType != ST_SOFTLINK) {        /* never delete the target itself */
            BPTR a = Lock((STRPTR)link, SHARED_LOCK), b = Lock((STRPTR)target, SHARED_LOCK);
            int same = a && b && SameLock(a, b) == LOCK_SAME;
            if (a) UnLock(a);
            if (b) UnLock(b);
            if (same) { cu_err(10, link, "is the same file as the target"); goto done; }
        }
        if (!DeleteFile((STRPTR)link)) { cu_fault(10, link); goto done; }
    }
    if (o_soft) ok = MakeLink((STRPTR)link, (LONG)target, LINK_SOFT);
    else {
        if (!(t = Lock((STRPTR)target, SHARED_LOCK))) { cu_fault(10, target); goto done; }
        if (!Examine(t, fib)) { cu_fault(10, target); goto done; }
        if (fib->fib_DirEntryType > 0) {                   /* a root (no parent): never */
            BPTR up = ParentDir(t);
            if (!up) { cu_err(10, target, "a volume or assign root cannot be hard linked"); goto done; }
            UnLock(up);
        }
        n = (const char *)PathPart((STRPTR)link) - link;
        memcpy(parent, link, n);
        parent[n] = 0;
        if (!(dir = Lock(parent, SHARED_LOCK))) { cu_fault(10, n ? parent : link); goto done; }
        if (SameLock(t, dir) == LOCK_DIFFERENT) {
            cu_err(10, link, "a hard link cannot go to another volume (ln -s makes a soft one)");
            goto done;
        }
        ok = MakeLink((STRPTR)link, (LONG)t, LINK_HARD);
    }
    if (!ok) cu_fault(10, link);
    else if (o_verbose) { cu_puts(link); cu_puts(" -> "); cu_puts(target); cu_putc('\n'); }
done:
    if (t) UnLock(t);
    if (dir) UnLock(dir);
    FreeDosObject(DOS_FIB, fib);
}

int main(void)
{
    char *v, path[CU_PATH];
    int c, i, last;
    (void)version;
    if (cu_args("ln [-sfv] TARGET [LINK]  |  ln [-sfv] TARGET ... DRAWER") < 0) return cu_rc;
    while ((c = cu_opt("sfv", &v)) >= 0) switch (c) {
        case 's': o_soft = 1; break;
        case 'f': o_force = 1; break;
        case 'v': o_verbose = 1; break;
        default: return cu_rc;
    }
    if (!cu_argc) { cu_err(10, NULL, "no target (ln ? for usage)"); return cu_rc; }
    if (cu_argc == 1) {
        link1(cu_argv[0], (const char *)FilePart((STRPTR)cu_argv[0]));
    } else if (cu_argc == 2 && !isdrawer(cu_argv[1])) {
        link1(cu_argv[0], cu_argv[1]);
    } else {
        last = cu_argc - 1;
        if (!isdrawer(cu_argv[last])) { cu_err(10, cu_argv[last], "is not a drawer"); return cu_rc; }
        for (i = 0; i < last && cu_rc < 20; i++) {
            char t[CU_PATH];
            strncpy(t, cu_argv[i], sizeof t - 1);
            t[sizeof t - 1] = 0;
            cu_trimslash(t);
            if (!cu_join(path, cu_argv[last], (const char *)FilePart((STRPTR)t))) { cu_err(10, cu_argv[i], "path too long"); continue; }
            link1(cu_argv[i], path);
            if (cu_break()) break;
        }
    }
    cu_flush();
    return cu_rc;
}
