/* touch - make a file's date now, or make an empty file.
 *
 *   touch [-c] [-d date | -r file] file | pattern ...
 *
 * A file or drawer that exists gets the date now (SetFileDate); a name
 * that does not exist becomes an empty file.
 *   -c       never make a file, only date the ones there
 *   -d DATE  that date instead of now: 2026-10-14, "2026-10-14 13:45",
 *            2026-10-14T13:45:10, or the Amiga's own "14-Oct-26 13:45:10",
 *            Today, Yesterday, a weekday name, or a time alone (today)
 *   -r FILE  the date FILE has
 *
 * Return code 0, 5 when a pattern matched nothing (said), 10 when a date
 * could not be set or a file made, or a bad -d / -r, 20 Ctrl-C. */
#include "cu.h"

const char *cu_name = "touch";
static const char version[] = "$VER: touch 0.1b3 (16.10.2026)";

static int o_nocreate;
static struct DateStamp when;

static int one(const char *path, void *u)
{
    BPTR l;
    (void)u;
    if ((l = Lock((STRPTR)path, SHARED_LOCK))) {
        UnLock(l);
        if (!SetFileDate((STRPTR)path, &when)) cu_fault(10, path);
    } else if (IoErr() != ERROR_OBJECT_NOT_FOUND) cu_fault(10, path);
    else if (!o_nocreate) {
        BPTR fh = Open((STRPTR)path, MODE_NEWFILE);
        if (!fh) cu_fault(10, path);
        else {
            Close(fh);
            SetFileDate((STRPTR)path, &when);
        }
    }
    return cu_break();
}

int main(void)
{
    char *v, *d = NULL, *r = NULL;
    int c, i;
    (void)version;
    if (cu_args("touch [-c] [-d date | -r file] file | pattern ...") < 0) return cu_rc;
    while ((c = cu_opt("cd:r:", &v)) >= 0) switch (c) {
        case 'c': o_nocreate = 1; break;
        case 'd': d = v; r = NULL; break;
        case 'r': r = v; d = NULL; break;
        default: return cu_rc;
    }
    if (!cu_argc) { cu_err(10, NULL, "no file named (touch ? for usage)"); return cu_rc; }
    DateStamp(&when);
    if (d && !cu_parsedate(d, &when)) { cu_err(10, d, "not a date"); return cu_rc; }
    if (r) {
        struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
        BPTR l = Lock(r, SHARED_LOCK);
        if (!fib || !l || !Examine(l, fib)) {
            cu_fault(10, r);
            if (l) UnLock(l);
            if (fib) FreeDosObject(DOS_FIB, fib);
            return cu_rc;
        }
        when = fib->fib_Date;
        UnLock(l);
        FreeDosObject(DOS_FIB, fib);
    }
    for (i = 0; i < cu_argc && cu_rc < 20; i++) cu_each(cu_argv[i], one, NULL);
    cu_flush();
    return cu_rc;
}
