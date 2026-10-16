/* df - how full the volumes are.
 *
 *   df [-h] [path ...]
 *
 * A line for each mounted volume (or for the volume each path is on): its
 * size, used and free space in KB (1024 bytes), how full it is (Use%,
 * rounded up, as GNU df) and its name.
 *   -h sizes as 1.5K, 12K, 3.4M, 2.0G
 * No requester for a volume that is not in a drive: a message instead.
 *
 * Return code 0, 5 when a path or volume could not be asked, 10 bad
 * arguments, 20 Ctrl-C. */
#include "cu.h"

const char *cu_name = "df";
static const char version[] = "$VER: df 0.1b2 (16.10.2026)";

#define MAXVOL 64

static int o_human;

static void putu(ULONG v, int width)
{
    char t[12];
    int i = 0;
    do { t[i++] = '0' + v % 10; v /= 10; } while (v);
    while (width-- > i) cu_putc(' ');
    while (i) cu_putc(t[--i]);
}

/* as du -h: one decimal under 10, rounded up */
static void human(ULONG kb, int width)
{
    static const char suf[] = "KMGT";
    char b[12];
    ULONG unit = 1, whole, tenths;
    int s = 0, n = 0, i;
    if (!kb) b[n++] = '0';
    else {
        char t[12];
        int k = 0;
        while (s < 3 && kb >= unit * 1024UL) { unit *= 1024; s++; }
        whole = kb / unit;
        tenths = whole * 10 + ((kb % unit) * 10 + unit - 1) / unit;
        if (tenths < 100) {
            b[n++] = '0' + tenths / 10; b[n++] = '.'; b[n++] = '0' + tenths % 10;
        } else {
            whole += kb % unit != 0;
            do { t[k++] = '0' + whole % 10; whole /= 10; } while (whole);
            while (k) b[n++] = t[--k];
        }
        b[n++] = suf[s];
    }
    for (i = n; i < width; i++) cu_putc(' ');
    cu_put(b, n);
}

static void size(ULONG kb)
{
    cu_putc(' ');
    if (o_human) human(kb, 6); else putu(kb, 10);
}

static ULONG kbof(ULONG blocks, ULONG bpb)
{
    if (!bpb) return 0;
    return bpb >= 1024 ? blocks * (bpb / 1024) : blocks / (1024 / bpb) + (blocks % (1024 / bpb) != 0);
}

/* the line for the volume the lock is on */
static void one(BPTR lock, const char *what)
{
    struct InfoData *id = cu_alloc(sizeof *id);
    ULONG total, used, pct;
    char name[108];
    if (!id) { cu_err(20, NULL, "out of memory"); return; }
    if (!Info(lock, id)) { cu_fault(5, what); cu_free(id); return; }
    total = kbof(id->id_NumBlocks, id->id_BytesPerBlock);
    used = kbof(id->id_NumBlocksUsed, id->id_BytesPerBlock);
    if (used > total) used = total;
    name[0] = 0;
    if (id->id_VolumeNode) {
        struct DosList *dl = BADDR(id->id_VolumeNode);
        UBYTE *b = BADDR(dl->dol_Name);
        if (b) { memcpy(name, b + 1, b[0]); name[b[0]] = 0; }
    }
    if (!name[0]) {                                  /* no volume node: the name the path has */
        int k = 0;
        while (what[k] && what[k] != ':' && k < 100) { name[k] = what[k]; k++; }
        name[k] = 0;
    }
    size(total);
    size(used);
    size(total - used);
    if (!total) cu_puts("    -");
    else {
        pct = total < 0x2000000UL ? (used * 100 + total - 1) / total : (used + total / 100 - 1) / (total / 100);
        cu_putc(' ');
        putu(pct, 3);
        cu_putc('%');
    }
    cu_putc(' ');
    cu_puts(name);
    cu_puts(":\n");
    cu_free(id);
}

static void bypath(const char *path)
{
    BPTR l = Lock((STRPTR)path, SHARED_LOCK);
    if (!l) { cu_fault(5, path); return; }
    one(l, path);
    UnLock(l);
}

int main(void)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    APTR win = me->pr_WindowPtr;
    char *v, *names[MAXVOL];
    int c, i, n = 0;
    (void)version;
    if (cu_args("df [-h] [path ...]") < 0) return cu_rc;
    while ((c = cu_opt("h", &v)) >= 0) switch (c) {
        case 'h': o_human = 1; break;
        default: return cu_rc;
    }
    me->pr_WindowPtr = (APTR)-1;
    if (o_human) cu_puts("  Size   Used   Free Use% Volume\n");
    else cu_puts("      Size       Used       Free Use% Volume\n");
    if (cu_argc) {
        for (i = 0; i < cu_argc && !cu_break(); i++) bypath(cu_argv[i]);
    } else {
        struct DosList *dl = LockDosList(LDF_VOLUMES | LDF_READ);
        while ((dl = NextDosEntry(dl, LDF_VOLUMES)) && n < MAXVOL) {
            UBYTE *b = BADDR(dl->dol_Name);
            if (!dl->dol_Task || !b) continue;       /* not in a drive */
            if ((names[n] = cu_alloc(b[0] + 2))) {
                memcpy(names[n], b + 1, b[0]);
                names[n][b[0]] = ':';
                names[n][b[0] + 1] = 0;
                n++;
            }
        }
        UnLockDosList(LDF_VOLUMES | LDF_READ);
        for (i = 0; i < n && !cu_break(); i++) bypath(names[i]);
    }
    me->pr_WindowPtr = win;
    cu_flush();
    return cu_rc;
}
