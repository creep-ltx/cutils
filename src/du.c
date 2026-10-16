/* du - how much the files in a tree take, drawer by drawer.
 *
 *   du [-achs] [-d N] [path | pattern ...]
 *
 * Each drawer under each path (the current drawer when none is given,
 * shown as "") gets a line: its size and its path, the drawers inside
 * before it. A path that is a file gets its own line.
 * Sizes are in KB (1024 bytes): each file counts its length rounded up to
 * a whole KB (an empty file 0), drawers themselves 0, links 0 and never
 * gone into (a hard link would count its file twice). So the numbers are
 * what the files hold, near what they take on the disk but not its blocks.
 *   -a also a line for each file   -s only one line for each path (-d 0)
 *   -d N  lines only down to N drawers below the path (the sizes still
 *         count everything)   -h sizes as 1.5K, 12K, 3.4M, 2.0G (rounded up)
 *   -c a last line with the grand total: "N  total"
 * The walk keeps its list on the heap: no depth is too deep for the stack.
 *
 * Return code 0, 5 when something could not be read, 10 bad arguments,
 * 20 Ctrl-C. */
#include "cu.h"

const char *cu_name = "du";
static const char version[] = "$VER: du 0.1b2 (16.10.2026)";

static int o_all, o_human, o_total;
static LONG maxd = 0x7fffffff;
static ULONG acc[CU_PATH / 2 + 4];   /* per depth: the drawer being added up */
static ULONG grand;

static void putu(ULONG v)
{
    char t[12];
    int i = 0;
    do { t[i++] = '0' + v % 10; v /= 10; } while (v);
    while (i) cu_putc(t[--i]);
}

static void human(ULONG kb)
{
    static const char suf[] = "KMGT";
    ULONG unit = 1, whole, tenths;
    int s = 0;
    if (!kb) { cu_putc('0'); return; }
    while (s < 3 && kb >= unit * 1024UL) { unit *= 1024; s++; }
    whole = kb / unit;
    tenths = whole * 10 + ((kb % unit) * 10 + unit - 1) / unit;    /* rounded up */
    if (tenths < 100) {
        putu(tenths / 10); cu_putc('.'); cu_putc('0' + tenths % 10);
    } else {
        whole += kb % unit != 0;
        putu(whole);
    }
    cu_putc(suf[s]);
}

static void line(ULONG kb, const char *path)
{
    if (o_human) human(kb); else putu(kb);
    cu_putc('\t');
    cu_puts(*path ? path : "\"\"");
    cu_putc('\n');
}

static ULONG kbof(CuEnt *e)
{
    if (e->type >= 0 || e->type == ST_LINKFILE) return 0;
    return (ULONG)e->size / 1024 + ((ULONG)e->size % 1024 != 0);
}

static int visit(CuEnt *e, void *u)
{
    ULONG kb;
    int d = e->depth < (int)(sizeof acc / sizeof *acc) ? e->depth : (int)(sizeof acc / sizeof *acc) - 1;
    (void)u;
    if (cu_isdir(e->type)) {
        if (!e->post) { acc[d] = 0; return 0; }
        kb = acc[d];
        if (d) acc[d - 1] += kb; else grand += kb;
        if (e->depth <= maxd) line(kb, e->path);
        return 0;
    }
    kb = kbof(e);
    if (!d) { grand += kb; line(kb, e->path); return 0; }
    acc[d - 1] += kb;
    if (o_all && e->depth <= maxd) line(kb, e->path);
    return 0;
}

static int onepath(const char *path, void *u)
{
    (void)u;
    return cu_walk(path, CU_WALK_POST, -1, visit, NULL) == CU_STOP;
}

int main(void)
{
    char *v;
    int c, i, ok;
    (void)version;
    if (cu_args("du [-achs] [-d N] [path | pattern ...]") < 0) return cu_rc;
    while ((c = cu_opt("achsd:", &v)) >= 0) switch (c) {
        case 'a': o_all = 1; break;
        case 'c': o_total = 1; break;
        case 'h': o_human = 1; break;
        case 's': maxd = 0; break;
        case 'd':
            maxd = cu_num(v, &ok);
            if (!ok || maxd < 0) { cu_err(10, v, "not a depth"); return cu_rc; }
            break;
        default: return cu_rc;
    }
    if (!cu_argc) onepath("", NULL);
    for (i = 0; i < cu_argc && cu_rc < 20; i++) cu_each(cu_argv[i], onepath, NULL);
    if (o_total && cu_rc < 20) line(grand, "total");
    cu_flush();
    return cu_rc;
}
