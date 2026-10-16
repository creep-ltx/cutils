/* which - where a command comes from. Takes the place of the Amiga's
 * C:Which, so both forms work:
 *
 *   which [-a] name ...                    (Unix)
 *   which FILE/A,NORES/S,RES/S,ALL/S       (the Amiga's template)
 *
 * Searched as the Shell does: the resident list, the current drawer, the
 * command path (Path), C:. The first place that has it is printed:
 *   RES Assign              made resident (Resident)
 *   INTERNAL cd             built into the Shell (as typed)
 *   Workbench:C/List        a file: its full name (NameFromLock)
 * A file counts only with the e bit (as the Shell runs it); a name with a
 * : in it is looked up as it is (once), and a volume or assign ("C:")
 * gives its drawer. Nothing is printed for a name not found.
 *   -a, ALL  every place that has it, not only the first (the same file
 *            can show twice: through C: and through the path)
 *   NORES    not the resident list     RES  only the resident list
 *   FILE     the keyword before the name may be given (FILE List)
 * Several names: each in turn (the Amiga's takes one).
 *
 * Return code 0 found, 5 not found (WARN, as the Amiga's; with several
 * names, when one was not), 10 bad arguments, 20 Ctrl-C. As the Amiga's
 * Which 45.2: with ALL an internal command alone gives 5 (only a resident
 * or a file counts then). */
#include "cu.h"

const char *cu_name = "which";
static const char version[] = "$VER: which 0.1b2 (16.10.2026)";

struct PathEnt { BPTR next, lock; };   /* cli_CommandDir: BPTRs to these */

static int o_all, o_nores, o_res;
static char buf[CU_PATH];

static int ieq(const char *a, const char *b, int n)   /* case blind, the first n (or all: -1) */
{
    for (; n; n--, a++, b++) {
        int x = *a, y = *b;
        if (x >= 'a' && x <= 'z') x -= 32;
        if (y >= 'a' && y <= 'z') y -= 32;
        if (x != y) return 0;
        if (!x) return 1;
    }
    return 1;
}

static int isexe(BPTR l)
{
    struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
    int ok = fib && Examine(l, fib) && fib->fib_DirEntryType < 0 && !(fib->fib_Protection & FIBF_EXECUTE);
    if (fib) FreeDosObject(DOS_FIB, fib);
    return ok;
}

static void putlock(BPTR l)
{
    if (NameFromLock(l, buf, sizeof buf)) cu_puts(buf);
    cu_putc('\n');
}

/* name in drawer dir (0: the current one): 1 and printed when it is a program */
static int indir(BPTR dir, const char *name)
{
    BPTR old = 0, l;
    int ok = 0;
    if (dir) old = CurrentDir(dir);
    if ((l = Lock((STRPTR)name, SHARED_LOCK))) {
        if ((ok = isexe(l))) putlock(l);
        UnLock(l);
    }
    if (dir) CurrentDir(old);
    return ok;
}

/* the resident list: 1 a resident, 2 internal, 0 none */
static int resident(const char *name)
{
    struct Segment *s;
    int r = 0;
    Forbid();
    if ((s = FindSegment((STRPTR)name, NULL, FALSE)) && s->seg_UC >= 0) r = 1;
    else if ((s = FindSegment((STRPTR)name, NULL, TRUE)) && s->seg_UC == CMD_INTERNAL) r = 2;
    Permit();
    return r;
}

static int which(const char *name)
{
    struct CommandLineInterface *cli = Cli();
    int found = 0, n = strlen(name);
    if (!o_nores && !strchr(name, ':') && !strchr(name, '/')) {
        int r = resident(name);
        if (r) {
            cu_puts(r == 1 ? "RES " : "INTERNAL ");
            cu_puts(name);
            cu_putc('\n');
            if (r == 1 || !o_all) found = 1;
            if (!o_all) return 1;
        }
    }
    if (o_res) return found;
    if (strchr(name, ':')) {                        /* as it is, once */
        BPTR l = Lock((STRPTR)name, SHARED_LOCK);
        if (l) {
            if (name[n - 1] == ':' || isexe(l)) { putlock(l); found = 1; }
            UnLock(l);
        }
        return found;
    }
    if (indir(0, name)) { found = 1; if (!o_all) return 1; }
    if (cli) {
        struct PathEnt *p;
        for (p = BADDR(cli->cli_CommandDir); p; p = BADDR(p->next)) {
            if (p->lock && indir(p->lock, name)) { found = 1; if (!o_all) return 1; }
            if (cu_break()) return found;
        }
    }
    {
        BPTR c = Lock("C:", SHARED_LOCK);
        if (c) {
            if (indir(c, name)) found = 1;
            UnLock(c);
        }
    }
    return found;
}

int main(void)
{
    int i, k = 0;
    char **names;
    (void)version;
    if (cu_args("which [-a] name ...  |  FILE/A,NORES/S,RES/S,ALL/S") < 0) return cu_rc;
    if (!(names = cu_alloc((cu_argc + 1) * sizeof *names))) { cu_err(20, NULL, "out of memory"); return cu_rc; }
    for (i = 0; i < cu_argc; i++) {
        char *a = cu_argv[i];
        if (ieq(a, "ALL", -1) || !strcmp(a, "-a")) o_all = 1;
        else if (ieq(a, "NORES", -1)) o_nores = 1;
        else if (ieq(a, "RES", -1)) o_res = 1;
        else if (ieq(a, "FILE", -1) && i + 1 < cu_argc) names[k++] = cu_argv[++i];
        else if (ieq(a, "FILE=", 5)) names[k++] = a + 5;
        else if (!strcmp(a, "--")) { while (++i < cu_argc) names[k++] = cu_argv[i]; }
        else if (a[0] == '-' && a[1]) { cu_err(10, a, "unknown option"); return cu_rc; }
        else names[k++] = a;
    }
    if (!k) { cu_err(10, NULL, "no command named (which ? for usage)"); return cu_rc; }
    if (o_res && o_nores) { if (cu_rc < 5) cu_rc = 5; return cu_rc; }   /* nowhere to look, as the Amiga's */
    for (i = 0; i < k && cu_rc < 20; i++)
        if (!which(names[i]) && cu_rc < 5) cu_rc = 5;
    cu_flush();
    return cu_rc;
}
