/* rm - delete files and drawers, carefully.
 *
 *   rm [-rfv] file | pattern ...
 *
 * C:Delete's job with Unix fingers and the E family's safety rules:
 * - A drawer is refused without -r (unlike Delete ALL).
 * - A delete-protected file (no d bit, the Amiga's own safety catch) is
 *   skipped and listed under "not deleted:" at the end. -f takes the
 *   protection off and tries again. A file another program has open
 *   always refuses.
 * - A volume root is refused, always: `rm -rf dh0:` deletes nothing.
 * - Links (soft, and hard links to drawers) go as links: the entry is
 *   deleted, never what it points at, with or without -r.
 * - -r reads each drawer whole before anything in it is deleted, walks
 *   the tree with a list on the heap (no recursion), and deletes the
 *   drawers deepest first. A child that will not go leaves its parents
 *   standing, each saying "directory not empty".
 *
 *   -r -R  drawers and everything in them
 *   -f     force: take off the delete protection; a name or pattern that
 *          is not there is no error
 *   -v     each name as it is deleted
 *
 * Return code 0, 5 when something was skipped or not there (and said),
 * 10 when something failed, 20 Ctrl-C (checked between entries). */
#include "cu.h"

const char *cu_name = "rm";
static const char version[] = "$VER: rm 0.1b1 (14.10.2026)";

static int o_rec, o_force, o_verbose;
static struct FileInfoBlock *fib;

static int delfile(const char *path, LONG type, void *u)
{
    LONG e;
    (void)type; (void)u;
    if (!DeleteFile((STRPTR)path)) {
        e = IoErr();
        if (e != ERROR_DELETE_PROTECTED) { SetIoErr(e); cu_fault(10, path); return 0; }
        if (!o_force) { cu_skip(path); return 0; }
        SetProtection((STRPTR)path, 0);
        if (!DeleteFile((STRPTR)path)) { cu_fault(10, path); return 0; }
    }
    if (o_verbose) { cu_puts(path); cu_putc('\n'); }
    return 1;
}

static int one(const char *arg, void *u)
{
    static char path[CU_PATH];
    (void)u;
    if (strlen(arg) >= CU_PATH) { cu_err(10, arg, "path too long"); return 0; }
    strcpy(path, arg);
    cu_trimslash(path);
    if (cu_isroot(path)) { cu_err(10, arg, "refusing to remove a volume root"); return 0; }
    if (!cu_lstat(path, fib)) {
        if (!(o_force && IoErr() == ERROR_OBJECT_NOT_FOUND)) cu_fault(5, arg);
        return 0;
    }
    if (CU_ISDIR(fib->fib_DirEntryType)) {
        if (!o_rec) { cu_err(10, arg, "is a drawer (use -r)"); return 0; }
        cu_deltree(path, delfile, NULL);
    } else delfile(path, fib->fib_DirEntryType, NULL);
    return cu_rc >= 20;
}

/* -f: a pattern that matches nothing is no error, so no "nothing matches" */
static int anymatch(const char *arg)
{
    struct AnchorPath *ap;
    char pat[300];
    int k = 0, r;
    BPTR l;
    if (!cu_haswild(arg)) return 1;
    if ((l = Lock((STRPTR)arg, SHARED_LOCK))) { UnLock(l); return 1; }
    for (; *arg && k < (int)sizeof pat - 3; arg++)
        if (*arg == '*') { pat[k++] = '#'; pat[k++] = '?'; } else pat[k++] = *arg;
    pat[k] = 0;
    if (!(ap = cu_alloc(sizeof *ap))) return 1;
    memset(ap, 0, sizeof *ap);
    r = MatchFirst(pat, ap) == 0;
    MatchEnd(ap);
    cu_free(ap);
    return r;
}

int main(void)
{
    char *v;
    int c, i;
    (void)version;
    if (cu_args("rm [-rfv] file | pattern ...") < 0) return cu_rc;
    while ((c = cu_opt("rRfv", &v)) >= 0) switch (c) {
        case 'r': case 'R': o_rec = 1; break;
        case 'f': o_force = 1; break;
        case 'v': o_verbose = 1; break;
        default: return cu_rc;
    }
    if (!cu_argc) { cu_err(10, NULL, "nothing to delete (rm ? for usage)"); return cu_rc; }
    if (!(fib = AllocDosObject(DOS_FIB, NULL))) { cu_err(20, NULL, "out of memory"); return cu_rc; }
    for (i = 0; i < cu_argc && cu_rc < 20; i++)
        if (!o_force || anymatch(cu_argv[i])) cu_each(cu_argv[i], one, NULL);
    cu_skipped("not deleted");
    cu_flush();
    FreeDosObject(DOS_FIB, fib);
    return cu_rc;
}
