/* cp - copy files and drawers, without destroying anything by default.
 *
 *   cp [-fr] from ... to
 *
 * The last name is TO, the ones before are sources (names or patterns).
 * With several sources or a pattern TO must be an existing drawer and
 * each goes in under its own name; one plain source may name its copy.
 *
 * An existing target file is skipped and listed under "not copied:" at
 * the end (return code 5). -f deletes and replaces it - after SameLock()
 * has made sure it is not the source itself, so `cp -f file file` can
 * never destroy the only copy. A drawer is never replaced by a file.
 * -f is delete-then-copy (the Unix shape): if the copy then fails, the
 * old target is gone.
 *
 * Protection bits, date and filenote go with every copy, as Copy CLONE
 * does (a drawer gets its protection and filenote). There is no -p:
 * keeping them is the Amiga default.
 *
 *   -f     replace an existing target file
 *   -r -R  drawers and everything in them. The tree is walked with a
 *          list on the heap (no recursion), a drawer created before its
 *          contents; an existing target drawer is merged into, the files
 *          in it following the same skip / -f rule. A drawer is never
 *          copied into itself or below itself.
 *
 * Links are not followed into: a soft link to a file is copied as the
 * file, a link to a drawer is skipped (and said). A broken or failed copy
 * deletes the partial target. Ctrl-C is checked between files and between
 * the 128 KB blocks of a copy.
 *
 * Return code 0, 5 when something was skipped (and said), 10 when
 * something failed, 20 Ctrl-C. */
#include "cu.h"

const char *cu_name = "cp";
static const char version[] = "$VER: cp 0.1b2 (16.10.2026)";

static int o_force, o_rec, toisdir;
static const char *to;
static struct FileInfoBlock *fib, *fib2;

/* Is TO (or the drawer it would land in) src itself or below it? By
 * canonical names from NameFromLock(), compared case blind (cp.e C1). */
static int lc(int c)
{
    c &= 0xff;
    return (c >= 'A' && c <= 'Z') || (c >= 0xc0 && c <= 0xde && c != 0xd7) ? c + 32 : c;
}

static int intoitself(const char *src, const char *dst)
{
    static char sp[CU_PATH], dp[CU_PATH];   /* static: the stack is small */
    BPTR l;
    int ok, i;
    if (!(l = Lock((STRPTR)src, SHARED_LOCK))) return 0;
    ok = NameFromLock(l, sp, CU_PATH);
    UnLock(l);
    if (!ok) return 0;
    if (!(l = Lock((STRPTR)dst, SHARED_LOCK))) {
        int n = (const char *)PathPart((STRPTR)dst) - dst;
        if (n >= CU_PATH) return 0;
        memcpy(dp, dst, n);
        dp[n] = 0;
        l = Lock(dp, SHARED_LOCK);
    }
    if (!l) return 0;
    ok = NameFromLock(l, dp, CU_PATH);
    UnLock(l);
    if (!ok) return 0;
    for (i = 0; sp[i]; i++) if (lc(sp[i]) != lc(dp[i])) return 0;
    return !dp[i] || dp[i] == '/' || sp[i - 1] == ':';
}

/* An existing target: 1 go on (none, or -f deleted it), 0 leave it. */
static int prepfile(const char *src, const char *dst)
{
    BPTR t = Lock((STRPTR)dst, SHARED_LOCK), s;
    int same, isdir = 0;
    if (!t) return 1;
    if (!(s = Lock((STRPTR)src, SHARED_LOCK))) {
        LONG e = IoErr();
        UnLock(t);
        SetIoErr(e);
        cu_fault(10, src);
        return 0;
    }
    same = SameLock(s, t) == LOCK_SAME;
    UnLock(s);
    if (Examine(t, fib2) && CU_ISDIR(fib2->fib_DirEntryType)) isdir = 1;
    UnLock(t);
    if (same) { cu_err(10, src, "source and target are the same file"); return 0; }
    if (isdir) { cu_err(10, dst, "is a drawer, not replaced by a file"); return 0; }
    if (!o_force) { cu_skip(src); return 0; }
    if (!DeleteFile((STRPTR)dst)) { cu_fault(10, dst); return 0; }
    return 1;
}

/* a soft link: copied as its file, a link to a drawer is not followed */
static int copylink(const char *src, const char *dst)
{
    BPTR l = Lock((STRPTR)src, SHARED_LOCK);
    int r = 0, isfile = 0;
    if (l) {
        isfile = Examine(l, fib2) && fib2->fib_DirEntryType < 0;
        UnLock(l);
    } else { cu_fault(5, src); return 0; }
    if (!isfile) { cu_err(5, src, "a link to a drawer, not followed"); return 0; }
    if (prepfile(src, dst))
        r = cu_copyfile(src, dst, fib2->fib_Protection, &fib2->fib_Date, (char *)fib2->fib_Comment);
    return r;
}

static int treedir(const char *s, const char *d, void *u)
{
    BPTR l = Lock((STRPTR)d, SHARED_LOCK);
    (void)u;
    if (l) {
        int isdir = Examine(l, fib2) && CU_ISDIR(fib2->fib_DirEntryType);
        UnLock(l);
        if (isdir) return 1;                     /* merged into */
        cu_err(10, d, "exists and is not a drawer");
        return 0;
    }
    if (!cu_clonedir(s, d)) { cu_fault(10, d); return 0; }
    return 1;
}

static int treefile(const char *s, const char *d, CuDirEnt *e, void *u)
{
    (void)u;
    if (CU_LINK(e->type)) {
        if (e->type == ST_LINKDIR) { cu_err(5, s, "a link to a drawer, not followed"); return 0; }
        return copylink(s, d) || cu_rc < 20 ? 1 : -1;
    }
    if (!prepfile(s, d)) return 1;
    if (cu_copyfile(s, d, e->prot, &e->date, e->note)) return 1;
    return cu_rc >= 20 ? -1 : 0;
}

static int one(const char *arg, void *u)
{
    static char src[CU_PATH], dst[CU_PATH];
    LONG type;
    (void)u;
    if (strlen(arg) >= CU_PATH) { cu_err(10, arg, "path too long"); return 0; }
    strcpy(src, arg);
    cu_trimslash(src);
    if (!cu_lstat(src, fib)) { cu_fault(5, arg); return 0; }
    type = fib->fib_DirEntryType;
    if (toisdir) {
        if (!cu_join(dst, to, (char *)FilePart(src))) { cu_err(10, arg, "path too long"); return 0; }
    } else strcpy(dst, to);
    if (type == ST_SOFTLINK) copylink(src, dst);
    else if (type == ST_LINKDIR) cu_err(5, arg, "a link to a drawer, not followed");
    else if (type > 0) {
        if (!o_rec) cu_err(10, arg, "is a drawer (use -r)");
        else if (intoitself(src, dst)) cu_err(10, arg, "cannot copy a drawer into itself");
        else cu_copytree(src, dst, treedir, treefile, NULL);
    } else if (prepfile(src, dst))
        cu_copyfile(src, dst, fib->fib_Protection, &fib->fib_Date, (char *)fib->fib_Comment);
    return cu_rc >= 20;
}

int main(void)
{
    char *v;
    int c, i, multi;
    BPTR l;
    (void)version;
    if (cu_args("cp [-fr] from ... to") < 0) return cu_rc;
    while ((c = cu_opt("frR", &v)) >= 0) switch (c) {
        case 'f': o_force = 1; break;
        case 'r': case 'R': o_rec = 1; break;
        default: return cu_rc;
    }
    if (cu_argc < 2) { cu_err(10, NULL, "needs FROM and TO (cp ? for usage)"); return cu_rc; }
    fib = AllocDosObject(DOS_FIB, NULL);
    fib2 = AllocDosObject(DOS_FIB, NULL);
    if (!fib || !fib2) { cu_err(20, NULL, "out of memory"); return cu_rc; }
    to = cu_argv[--cu_argc];
    multi = cu_argc > 1;
    for (i = 0; i < cu_argc; i++)
        if (cu_haswild(cu_argv[i]) && !(l = Lock(cu_argv[i], SHARED_LOCK))) multi = 1;
        else if (cu_haswild(cu_argv[i])) UnLock(l);
    if ((l = Lock((STRPTR)to, SHARED_LOCK))) {
        toisdir = Examine(l, fib) && fib->fib_DirEntryType > 0;
        UnLock(l);
    }
    if (multi && !toisdir) { cu_err(10, to, "not a drawer (several sources need one)"); return cu_rc; }
    for (i = 0; i < cu_argc && cu_rc < 20; i++) cu_each(cu_argv[i], one, NULL);
    cu_skipped("not copied");
    cu_flush();
    FreeDosObject(DOS_FIB, fib);
    FreeDosObject(DOS_FIB, fib2);
    return cu_rc;
}
