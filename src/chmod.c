/* chmod - change protection bits, in Unix words or the Amiga's letters.
 *
 *   chmod [-Rvc] MODE file | pattern ...
 *
 * MODE is one or more of [who]+|-|=letters, joined with commas, or octal:
 *   chmod +x prog      the e bit on          chmod -w file   write and delete off
 *   chmod u+rwx f      owner: r w e d on     chmod go-r f    group/other read off
 *   chmod +s script    the script bit        chmod +p prog   pure (Resident)
 *   chmod -a f         archive bit off       chmod =rwed f   exactly ----rwed
 *   chmod 755 prog     owner rwed, group and other r and e
 * Letters: r read, w write (and delete: on AmigaOS deleting is the
 * file's own bit, so Unix's w means both), x or e execute, d delete
 * alone, and the Amiga's own s script, p pure, a archived, h hold (bit 7).
 * Who: u (the owner: the bits AmigaOS itself uses, also when no who is
 * given), g group, o other (kept by the file system for networks and
 * MultiUser), a all three. = sets the owner's r w e d and the h s p a
 * bits to exactly the letters given (as Protect does); with g or o only
 * those. Octal is the owner digit (4 r, 2 w and d, 1 e), then group and
 * other; h s p a are kept as they were.
 *   -R  into drawers, all the way down (links are left as they are)
 *   -v  say each file: "file: ----rwed -> ----rw-d"   -c  only the changed
 * A MODE starting with - is a mode, not a flag (chmod -w file).
 *
 * Return code 0, 5 when a pattern matched nothing (said), 10 when a bit
 * could not be set or a bad MODE, 20 Ctrl-C. */
#include "cu.h"

const char *cu_name = "chmod";
static const char version[] = "$VER: chmod 0.1b1 (14.10.2026)";

static int o_rec, o_verbose, o_changes;
static const char *mode;

/* the owner's bits r w e d are kept inverted (set: not allowed); group
 * and other, and h s p a, are set when on. Work with "on" bits: */
#define ON_MASK (FIBF_READ | FIBF_WRITE | FIBF_EXECUTE | FIBF_DELETE)

static LONG letters(const char *s, int who, int *ok, const char **end)
{
    LONG b = 0;
    *ok = 1;
    for (; *s && *s != ','; s++) {
        switch (*s) {
            case 'r': if (who & 1) b |= FIBF_READ; if (who & 2) b |= FIBF_GRP_READ; if (who & 4) b |= FIBF_OTR_READ; break;
            case 'w':
                if (who & 1) b |= FIBF_WRITE | FIBF_DELETE;
                if (who & 2) b |= FIBF_GRP_WRITE | FIBF_GRP_DELETE;
                if (who & 4) b |= FIBF_OTR_WRITE | FIBF_OTR_DELETE;
                break;
            case 'x': case 'e': if (who & 1) b |= FIBF_EXECUTE; if (who & 2) b |= FIBF_GRP_EXECUTE; if (who & 4) b |= FIBF_OTR_EXECUTE; break;
            case 'X': if (who & 1) b |= FIBF_EXECUTE; if (who & 2) b |= FIBF_GRP_EXECUTE; if (who & 4) b |= FIBF_OTR_EXECUTE; break;
            case 'd': if (who & 1) b |= FIBF_DELETE; if (who & 2) b |= FIBF_GRP_DELETE; if (who & 4) b |= FIBF_OTR_DELETE; break;
            case 's': b |= FIBF_SCRIPT; break;
            case 'p': b |= FIBF_PURE; break;
            case 'a': b |= FIBF_ARCHIVE; break;
            case 'h': b |= FIBF_HIDDEN; break;
            case '+': case '-': case '=': *end = s; return b;
            default: *ok = 0; return 0;
        }
    }
    *end = s;
    return b;
}

/* the new bits ("on" form) from the old, or -1 for a bad MODE */
static LONG apply(LONG on, int isdir)
{
    const char *s = mode;
    (void)isdir;
    if (*s >= '0' && *s <= '7') {                       /* octal */
        int n = strlen(s), i, d[3] = { 0, 0, 0 };
        if (n > 4) return -1;
        for (i = 0; i < n; i++) if (s[i] < '0' || s[i] > '7') return -1;
        for (i = 0; i < 3 && i < n; i++) d[2 - i] = s[n - 1 - i] - '0';
        on &= ~(ON_MASK | 0xff00);
        if (d[0] & 4) on |= FIBF_READ;
        if (d[0] & 2) on |= FIBF_WRITE | FIBF_DELETE;
        if (d[0] & 1) on |= FIBF_EXECUTE;
        if (d[1] & 4) on |= FIBF_GRP_READ;
        if (d[1] & 2) on |= FIBF_GRP_WRITE | FIBF_GRP_DELETE;
        if (d[1] & 1) on |= FIBF_GRP_EXECUTE;
        if (d[2] & 4) on |= FIBF_OTR_READ;
        if (d[2] & 2) on |= FIBF_OTR_WRITE | FIBF_OTR_DELETE;
        if (d[2] & 1) on |= FIBF_OTR_EXECUTE;
        return on;
    }
    while (*s) {
        int who = 0, ok;
        for (; *s == 'u' || *s == 'g' || *s == 'o' || (*s == 'a' && (s[1] == 'u' || s[1] == 'g' || s[1] == 'o' || s[1] == 'a' || s[1] == '+' || s[1] == '-' || s[1] == '=')); s++)
            who |= *s == 'u' ? 1 : *s == 'g' ? 2 : *s == 'o' ? 4 : 7;
        if (!who) who = 1;
        if (*s != '+' && *s != '-' && *s != '=') return -1;
        while (*s == '+' || *s == '-' || *s == '=') {
            int op = *s++;
            const char *end;
            LONG b = letters(s, who, &ok, &end), clear = 0;
            if (!ok) return -1;
            if (op == '+') on |= b;
            else if (op == '-') on &= ~b;
            else {
                if (who & 1) clear |= ON_MASK | FIBF_HIDDEN | FIBF_SCRIPT | FIBF_PURE | FIBF_ARCHIVE;
                if (who & 2) clear |= FIBF_GRP_READ | FIBF_GRP_WRITE | FIBF_GRP_EXECUTE | FIBF_GRP_DELETE;
                if (who & 4) clear |= FIBF_OTR_READ | FIBF_OTR_WRITE | FIBF_OTR_EXECUTE | FIBF_OTR_DELETE;
                on = (on & ~clear) | b;
            }
            s = end;
        }
        if (*s == ',') s++;
        else if (*s) return -1;
    }
    return on;
}

static int set(const char *path, LONG prot, int isdir)
{
    LONG on = prot ^ ON_MASK, nw = apply(on, isdir), newprot;
    char a[9], b[9];
    if (nw < 0) { cu_err(10, mode, "not a mode (chmod ? for usage)"); return 1; }
    newprot = nw ^ ON_MASK;
    if (newprot != prot && !SetProtection((STRPTR)path, newprot)) { cu_fault(10, path); return 0; }
    if (o_verbose || (o_changes && newprot != prot)) {
        cu_protstr(prot, a); cu_protstr(newprot, b);
        cu_puts(path); cu_puts(": "); cu_puts(a);
        if (newprot != prot) { cu_puts(" -> "); cu_puts(b); } else cu_puts(" (unchanged)");
        cu_putc('\n');
    }
    return 0;
}

static int walked(CuEnt *e, void *u)
{
    (void)u;
    if (e->post || CU_LINK(e->type)) return 0;
    if (set(e->path, e->prot, cu_isdir(e->type))) return CU_STOP;
    return cu_rc >= 20 ? CU_STOP : 0;
}

static int one(const char *path, void *u)
{
    struct FileInfoBlock *fib = u;
    if (o_rec) { cu_walk(path, 0, -1, walked, NULL); return cu_rc >= 20; }
    if (!cu_lstat(path, fib)) { cu_fault(10, path); return cu_break(); }
    if (fib->fib_DirEntryType == ST_SOFTLINK) { cu_err(5, path, "a soft link: left as it is"); return cu_break(); }
    if (set(path, fib->fib_Protection, CU_ISDIR(fib->fib_DirEntryType))) return 1;
    return cu_break();
}

int main(void)
{
    struct FileInfoBlock *fib;
    int i;
    (void)version;
    if (cu_args("chmod [-Rvc] MODE file | pattern ...") < 0) return cu_rc;
    /* flags by hand: -w is a mode, -R -v -c (and bundles of them) are flags */
    for (i = 0; i < cu_argc; i++) {
        char *a = cu_argv[i];
        if (!strcmp(a, "--")) { i++; break; }
        if (a[0] == '-' && a[1] && strspn(a + 1, "Rvc") == strlen(a + 1)) {
            for (a++; *a; a++) { if (*a == 'R') o_rec = 1; else if (*a == 'v') o_verbose = 1; else o_changes = 1; }
            continue;
        }
        break;
    }
    if (i >= cu_argc) { cu_err(10, NULL, "no mode (chmod ? for usage)"); return cu_rc; }
    mode = cu_argv[i++];
    if (apply(0, 0) < 0) { cu_err(10, mode, "not a mode (chmod ? for usage)"); return cu_rc; }
    if (i >= cu_argc) { cu_err(10, NULL, "no file named (chmod ? for usage)"); return cu_rc; }
    if (!(fib = AllocDosObject(DOS_FIB, NULL))) { cu_err(20, NULL, "out of memory"); return cu_rc; }
    for (; i < cu_argc && cu_rc < 20; i++) cu_each(cu_argv[i], one, fib);
    FreeDosObject(DOS_FIB, fib);
    cu_flush();
    return cu_rc;
}
