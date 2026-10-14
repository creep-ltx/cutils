/* stat - all a file system says about a file.
 *
 *   stat [-L] [-t] [-c FORMAT] file | pattern ...
 *
 *     File: s:Startup-Sequence
 *     Type: file
 *     Size: 51 bytes, 2 blocks
 *  Protect: ----rw-d
 *     Date: 2026-08-15 10:53:32 (Saturday)
 *  Comment: a filenote, when there is one
 *
 * Type is file, drawer, volume (a root such as "Work:"), soft link (and
 * where it points) or hard link. A soft link is described itself, not
 * what it points to, unless -L. Protect is the Amiga's eight bits as List
 * shows them: h (hold), s (script), p (pure), a (archived), r w e d
 * (read, write, execute, delete; a - means not allowed).
 *   -L         follow a soft link
 *   -t         one line each: name size blocks protect date
 *   -c FORMAT  only FORMAT, with %n the name, %s the size, %b the blocks,
 *              %A the protection bits, %F the type, %y the date
 *              (2026-08-15 10:53:32), %Y it in seconds since 1970, %C the
 *              comment, %N the name and a soft link's target, %% a %;
 *              \n and \t are a new line and a tab, a new line at the end
 *
 * Return code 0, 5 when a file was not found or a pattern matched nothing
 * (said), 10 bad arguments, 20 Ctrl-C. */
#include "cu.h"

const char *cu_name = "stat";
static const char version[] = "$VER: stat 0.1b1 (14.10.2026)";

static int o_follow, o_terse;
static const char *o_fmt;
static struct FileInfoBlock *fib;
static char target[CU_PATH];

static const char *typename(LONG t)
{
    if (t == ST_ROOT) return "volume";
    if (t == ST_SOFTLINK) return "soft link";
    if (t == ST_LINKDIR) return "hard link to a drawer";
    if (t == ST_LINKFILE) return "hard link to a file";
    if (t > 0) return "drawer";
    if (t == ST_PIPEFILE) return "pipe";
    return "file";
}

static void linktarget(const char *path)
{
    static char dir[CU_PATH];
    const char *name = (const char *)FilePart((STRPTR)path);
    int n = (const char *)PathPart((STRPTR)path) - path;
    BPTR l;
    target[0] = 0;
    if (n >= CU_PATH) return;
    memcpy(dir, path, n);
    dir[n] = 0;
    if ((l = Lock(dir, SHARED_LOCK))) {
        struct MsgPort *port = ((struct FileLock *)BADDR(l))->fl_Task;
        LONG r = ReadLink(port, l, (STRPTR)name, target, sizeof target);
        if (r == 0 || r == -1 || r == -2) target[0] = 0;   /* 3.2: the length, older: DOSTRUE */
        UnLock(l);
    }
}

static void putdate(const struct DateStamp *ds)
{
    char d[20];
    cu_isodate(ds, d);
    cu_puts(d);
}

static void putname(const char *path)
{
    cu_puts(path);
    if (fib->fib_DirEntryType == ST_SOFTLINK && target[0]) { cu_puts(" -> "); cu_puts(target); }
}

static void format(const char *path)
{
    const char *f;
    char p[9];
    for (f = o_fmt; *f; f++) {
        if (*f == '\\' && (f[1] == 'n' || f[1] == 't' || f[1] == '\\')) { f++; cu_putc(*f == 'n' ? '\n' : *f == 't' ? '\t' : '\\'); continue; }
        if (*f != '%' || !f[1]) { cu_putc(*f); continue; }
        switch (*++f) {
            case 'n': cu_puts(path); break;
            case 'N': putname(path); break;
            case 's': cu_putnum(fib->fib_Size, 0); break;
            case 'b': cu_putnum(fib->fib_NumBlocks, 0); break;
            case 'A': cu_protstr(fib->fib_Protection, p); cu_puts(p); break;
            case 'F': cu_puts(typename(fib->fib_DirEntryType)); break;
            case 'y': putdate(&fib->fib_Date); break;
            case 'Y': {
                char t[12];
                int i = 0;
                ULONG u = cu_unixtime(&fib->fib_Date);
                do t[i++] = '0' + u % 10, u /= 10; while (u);
                while (i) cu_putc(t[--i]);
                break;
            }
            case 'C': cu_puts((char *)fib->fib_Comment); break;
            case '%': cu_putc('%'); break;
            default: cu_putc('%'); cu_putc(*f); break;
        }
    }
    cu_putc('\n');
}

static int one(const char *path, void *u)
{
    char p[9];
    int ok;
    (void)u;
    target[0] = 0;
    if (o_follow) {
        BPTR l = Lock((STRPTR)path, SHARED_LOCK);
        ok = l && Examine(l, fib);
        if (!ok) { cu_fault(5, path); if (l) UnLock(l); return cu_break(); }
        UnLock(l);
    } else if (!cu_lstat(path, fib)) { cu_fault(5, path); return cu_break(); }
    if (fib->fib_DirEntryType == ST_SOFTLINK) linktarget(path);
    if (o_fmt) format(path);
    else if (o_terse) {
        cu_puts(path); cu_putc(' ');
        cu_putnum(fib->fib_Size, 0); cu_putc(' ');
        cu_putnum(fib->fib_NumBlocks, 0); cu_putc(' ');
        cu_protstr(fib->fib_Protection, p); cu_puts(p); cu_putc(' ');
        putdate(&fib->fib_Date); cu_putc('\n');
    } else {
        cu_puts("   File: "); putname(path); cu_putc('\n');
        cu_puts("   Type: "); cu_puts(typename(fib->fib_DirEntryType)); cu_putc('\n');
        if (fib->fib_DirEntryType != ST_SOFTLINK) {
            cu_puts("   Size: "); cu_putnum(fib->fib_Size, 0); cu_puts(fib->fib_Size == 1 ? " byte, " : " bytes, ");
            cu_putnum(fib->fib_NumBlocks, 0); cu_puts(fib->fib_NumBlocks == 1 ? " block\n" : " blocks\n");
            cu_puts("Protect: "); cu_protstr(fib->fib_Protection, p); cu_puts(p); cu_putc('\n');
            cu_puts("   Date: "); putdate(&fib->fib_Date);
            cu_puts(" ("); cu_puts(cu_wdays[fib->fib_Date.ds_Days % 7]); cu_puts(")\n");
            if (fib->fib_Comment[0]) { cu_puts("Comment: "); cu_puts((char *)fib->fib_Comment); cu_putc('\n'); }
        }
    }
    return cu_break();
}

int main(void)
{
    char *v;
    int c, i;
    (void)version;
    if (cu_args("stat [-L] [-t] [-c FORMAT] file | pattern ...") < 0) return cu_rc;
    while ((c = cu_opt("Ltc:", &v)) >= 0) switch (c) {
        case 'L': o_follow = 1; break;
        case 't': o_terse = 1; break;
        case 'c': o_fmt = v; break;
        default: return cu_rc;
    }
    if (!cu_argc) { cu_err(10, NULL, "no file named (stat ? for usage)"); return cu_rc; }
    if (!(fib = AllocDosObject(DOS_FIB, NULL))) { cu_err(20, NULL, "out of memory"); return cu_rc; }
    for (i = 0; i < cu_argc && cu_rc < 20; i++) cu_each(cu_argv[i], one, NULL);
    FreeDosObject(DOS_FIB, fib);
    cu_flush();
    return cu_rc;
}
