/* mv - move or rename files and drawers, without destroying anything by
 * default.
 *
 *   mv [-fb] from ... to
 *
 * The last name is TO, the ones before are sources (names or patterns).
 * With several sources or a pattern TO must be an existing drawer; one
 * plain source is renamed, or moved into TO when TO is a drawer.
 *
 * On one volume a move is Rename(): one cheap call, for a file or a whole
 * drawer. Across volumes a file is copied (protection, date and filenote
 * kept) and the source deleted. A drawer is moved whole or not at all:
 * the tree is copied first, and the source deleted (deepest first) only
 * when every file copied clean; any failure deletes the partial copy and
 * leaves the source untouched. Then the target must not exist yet
 * (merging is cp's job), and a tree with a link in it is refused (a link
 * cannot be made again faithfully on another volume). A Ctrl-C during the
 * copy leaves the partial copy at TO and the source untouched (and says
 * so).
 *
 * An existing target is skipped and listed under "not moved:" at the end
 * (return code 5).
 *   -f  delete and replace an existing target
 *   -b  rename an existing target to NAME.mvbak first; when that name is
 *       taken too the source is not moved (said, rc 10). -bf replaces
 *       the old .mvbak: -f always allows destroying exactly one thing.
 * Neither touches anything when source and target are the same object
 * (SameLock), or when the target is a drawer. A volume root is never
 * moved.
 *
 * Return code 0, 5 when something was skipped or a moved source could
 * not be deleted (and said), 10 when something failed, 20 Ctrl-C. */
#include "cu.h"

const char *cu_name = "mv";
static const char version[] = "$VER: mv 0.1b1 (14.10.2026)";

static int o_force, o_backup, toisdir;
static const char *to;
static struct FileInfoBlock *fib, *fib2;

static int lc(int c)
{
    c &= 0xff;
    return (c >= 'A' && c <= 'Z') || (c >= 0xc0 && c <= 0xde && c != 0xd7) ? c + 32 : c;
}

/* "what: text: why", the why from IoErr() */
static void say(int rc, const char *what, const char *text)
{
    static char t[CU_PATH + 120];
    LONG e = IoErr();
    int n;
    strncpy(t, text, CU_PATH + 10);
    t[CU_PATH + 10] = 0;
    n = strlen(t);
    if (e) {
        strcpy(t + n, ": ");
        if (!Fault(e, NULL, t + n + 2, 100)) t[n] = 0;
    }
    cu_err(rc, what, t);
}

/* the entry's own name, canonical, without following a link: the
 * drawer's name from NameFromLock() and the last part as given */
static int entryname(const char *path, char *buf)
{
    int n = (const char *)PathPart((STRPTR)path) - path, ok = 0;
    BPTR l;
    if (n >= CU_PATH) return 0;
    memcpy(buf, path, n);
    buf[n] = 0;
    if ((l = Lock(buf, SHARED_LOCK))) {
        ok = NameFromLock(l, buf, CU_PATH) && AddPart(buf, FilePart((STRPTR)path), CU_PATH);
        UnLock(l);
    }
    return ok;
}

static int samename(const char *x, const char *y)
{
    static char a[CU_PATH], b[CU_PATH];
    int i;
    if (!entryname(x, a) || !entryname(y, b)) return 0;
    for (i = 0; a[i] && lc(a[i]) == lc(b[i]); i++) ;
    return !a[i] && !b[i];
}

/* ---- a drawer across volumes, whole or not at all ---- */

static int tdir(const char *s, const char *d, void *u)
{
    (void)u;
    if (!cu_clonedir(s, d)) { cu_fault(10, d); return -1; }
    return 1;
}

static int tfile(const char *s, const char *d, CuEnt *e, void *u)
{
    (void)u;
    if (CU_LINK(e->type)) { cu_err(10, s, "a link in the tree, not movable across volumes"); return -1; }
    return cu_copyfile(s, d, e->prot, &e->date, e->note) ? 1 : -1;
}

static int tdel(const char *path, LONG type, void *u)
{
    (void)type; (void)u;
    if (DeleteFile((STRPTR)path)) return 1;
    cu_fault(5, path);
    return 0;
}

static void treemove(const char *src, const char *dst)
{
    BPTR l;
    if ((l = Lock((STRPTR)dst, SHARED_LOCK))) {
        UnLock(l);
        cu_err(10, src, "not moved onto an existing drawer across volumes");
        return;
    }
    if (cu_copytree(src, dst, tdir, tfile, NULL)) {
        if (!cu_deltree(src, tdel, NULL)) cu_err(5, src, "moved, but the source could not be fully deleted");
        return;
    }
    if (cu_rc >= 20) {
        cu_err(20, src, "move broken off: the partial copy is at the target, source untouched");
        return;
    }
    l = Lock((STRPTR)dst, SHARED_LOCK);
    if (l) { UnLock(l); cu_deltree(dst, tdel, NULL); }
    cu_err(10, src, "move abandoned, source untouched");
}

static int one(const char *arg, void *u)
{
    static char src[CU_PATH], dst[CU_PATH], bak[CU_PATH + 8];   /* static: the stack is small */
    BPTR t, s;
    LONG type, e;
    (void)u;
    if (strlen(arg) >= CU_PATH) { cu_err(10, arg, "path too long"); return 0; }
    strcpy(src, arg);
    cu_trimslash(src);
    if (cu_isroot(src)) { cu_err(10, arg, "cannot move a volume root"); return 0; }
    if (!cu_lstat(src, fib)) { cu_fault(5, arg); return 0; }
    type = fib->fib_DirEntryType;
    if (toisdir) {
        if (!cu_join(dst, to, (char *)FilePart(src))) { cu_err(10, arg, "path too long"); return 0; }
    } else strcpy(dst, to);

    if ((t = Lock(dst, SHARED_LOCK))) {
        int same, isdir = 0;
        /* the source must exist and be another object before the target's
         * fate is decided, or -f could delete the target for nothing */
        if (!(same = samename(src, dst)) && type != ST_SOFTLINK) {
            if (!(s = Lock((STRPTR)src, SHARED_LOCK))) {
                e = IoErr();
                UnLock(t);
                SetIoErr(e);
                cu_fault(10, src);
                return 0;
            }
            same = SameLock(s, t) == LOCK_SAME;
            UnLock(s);
        }
        if (Examine(t, fib2) && CU_ISDIR(fib2->fib_DirEntryType)) isdir = 1;
        UnLock(t);
        if (same && strcmp((char *)FilePart(src), (char *)FilePart(dst))) {
            /* the same entry under a new case: a plain Rename */
            if (!Rename(src, dst)) cu_fault(10, src);
            return 0;
        }
        if (same) { cu_err(10, src, "source and target are the same file"); return 0; }
        if (isdir) { cu_err(10, dst, "is a drawer, not replaced"); return 0; }
        if (o_backup) {
            strcpy(bak, dst);
            strcat(bak, ".mvbak");
            if ((t = Lock(bak, SHARED_LOCK))) {
                UnLock(t);
                if (!o_force) {
                    static char t2[CU_PATH + 40];
                    strcpy(t2, "not moved, ");
                    strcat(t2, bak);
                    strcat(t2, " already exists");
                    cu_err(10, src, t2);
                    cu_skip(src);
                    return 0;
                }
                if (!DeleteFile(bak)) { cu_fault(10, bak); return 0; }
            }
            if (!Rename(dst, bak)) { cu_fault(10, dst); return 0; }
        } else if (o_force) {
            if (!DeleteFile(dst)) { cu_fault(10, dst); return 0; }
        } else { cu_skip(src); return 0; }
    }

    if (Rename(src, dst)) return 0;
    if ((e = IoErr()) != ERROR_RENAME_ACROSS_DEVICES) { SetIoErr(e); cu_fault(10, src); return 0; }
    if (CU_LINK(type)) { cu_err(10, src, "a link, not movable across volumes"); return 0; }
    if (type > 0) treemove(src, dst);
    else if (cu_copyfile(src, dst, fib->fib_Protection, &fib->fib_Date, (char *)fib->fib_Comment)
             && !DeleteFile(src))
        say(5, src, "copied, but the source not deleted");
    return cu_rc >= 20;
}

int main(void)
{
    char *v;
    int c, i, multi;
    BPTR l;
    (void)version;
    if (cu_args("mv [-fb] from ... to") < 0) return cu_rc;
    while ((c = cu_opt("fb", &v)) >= 0) switch (c) {
        case 'f': o_force = 1; break;
        case 'b': o_backup = 1; break;
        default: return cu_rc;
    }
    if (cu_argc < 2) { cu_err(10, NULL, "needs FROM and TO (mv ? for usage)"); return cu_rc; }
    fib = AllocDosObject(DOS_FIB, NULL);
    fib2 = AllocDosObject(DOS_FIB, NULL);
    if (!fib || !fib2) { cu_err(20, NULL, "out of memory"); return cu_rc; }
    to = cu_argv[--cu_argc];
    multi = cu_argc > 1;
    for (i = 0; i < cu_argc; i++)
        if (cu_haswild(cu_argv[i])) {
            if ((l = Lock(cu_argv[i], SHARED_LOCK))) UnLock(l); else multi = 1;
        }
    if ((l = Lock((STRPTR)to, SHARED_LOCK))) {
        toisdir = Examine(l, fib) && fib->fib_DirEntryType > 0;
        UnLock(l);
    }
    if (multi && !toisdir) { cu_err(10, to, "not a drawer (several sources need one)"); return cu_rc; }
    for (i = 0; i < cu_argc && cu_rc < 20; i++) cu_each(cu_argv[i], one, NULL);
    cu_skipped("not moved");
    cu_flush();
    FreeDosObject(DOS_FIB, fib);
    FreeDosObject(DOS_FIB, fib2);
    return cu_rc;
}
