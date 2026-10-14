/* ls - list directory contents, Unix-style, coloured by kind.
 * CShell's ls 0.2 (its docs/design.md "Colours") on the CUtils base.
 *
 *   ls [-1aAdFhlrRSt] [path | pattern ...]
 *
 * Multi-column, natural order (file9 before file10), case blind, drawers and
 * files mixed. A drawer lists its contents, a pattern (#? or *) the matches,
 * a file itself; files and matches first as one list, then each drawer
 * (under "name:" when there are several). Flags may come anywhere before
 * --. Columns fit the window: $COLUMNS (csh keeps it), else the console is
 * asked (CSI 0 q). Output that is not the console (a file, a pipe) gets one
 * name a line and no colours.
 *
 * Colours by kind, from ENV:FileColours (else ENVARC:; the family's file,
 * lib/cu_colours.c, Ahiru and Maki may read it too), written with the
 * defaults when missing:
 *   KIND CTERM,PLAIN   SGR numbers in CTerm and in a plain Shell window
 *   .EXT KIND          a name ending in .ext is of that kind
 * Kinds: drawer link program script archive picture sound source text
 * icon hidden. A program is a name with no ending and the e bit (as Ahiru
 * says), a script has the s bit, hidden the h bit, an icon ends in .info.
 *
 *   -1 one a line   -a also .info and h-bit names   -A the same
 *   -d a drawer itself, not its contents   -F / after drawers, * after
 *   programs, @ after links   -h sizes in K/M/G   -l long: protection,
 *   size, date, name, the filenote below   -r reverse   -R into drawers
 *   (32 levels at most, links not followed)   -S by size, biggest first
 *   -t by date, newest first
 *
 * Return code 0, 5 when a path could not be read, 10 bad arguments,
 * 20 Ctrl-C. */
#include <dos/exall.h>
#include <dos/datetime.h>
#include <dos/var.h>
#include "cu.h"

const char *cu_name = "ls";
static const char version[] = "$VER: ls 0.3b1 (14.10.2026)";

#define MAXARGS 64

typedef struct Ent {
    char *name;
    LONG size, prot, type;
    struct DateStamp date;
    char *note;
    int kind;
} Ent;

static int o_one, o_all, o_dir, o_class, o_human, o_long, o_rev, o_rec, o_size, o_time;
static int colour, cols = 80, cterm;

static int lc(int c)
{
    c &= 0xff;
    if ((c >= 'A' && c <= 'Z') || (c >= 0xc0 && c <= 0xde && c != 0xd7)) c += 32;
    return c;
}

static int ieq(const char *a, const char *b)
{
    while (*a && lc(*a) == lc(*b)) a++, b++;
    return !*a && !*b;
}

static int kindof(Ent *e) { return cu_kind(e->name, e->type, e->prot); }

/* ---- the window's width ---- */

static int termwidth(BPTR in)
{
    UBYTE b[64];
    int n = 0, p[4] = { 0, 0, 0, 0 }, np = 0, i, w = 0;
    char v[16];
    if (GetVar("COLUMNS", v, sizeof v, GVF_LOCAL_ONLY) > 0) {
        LONG c;
        if (StrToLong(v, &c) > 0 && c >= 10 && c < 1000) return c;
    }
    if (!in || !IsInteractive(in)) return 80;
    SetMode(in, 1);
    Write(cu_out, "\x9b" "0 q", 4);
    while (n < (int)sizeof b - 1 && WaitForChar(in, 500000)) {
        if (Read(in, b + n, 1) != 1) break;
        if (b[n++] == 'r') break;
    }
    SetMode(in, 0);
    for (i = 0; i < n; i++) {
        if (b[i] >= '0' && b[i] <= '9') { if (np < 4) p[np] = p[np] * 10 + b[i] - '0'; }
        else if (b[i] == ';') { if (np < 3) np++; }
        else if (b[i] == 'r') { if (np == 3) w = p[3]; break; }
    }
    return w >= 10 ? w : 80;
}

/* ---- reading a drawer ---- */

typedef struct { Ent *e; int n, cap; char *pool; long plen, pcap; } List;

static char *keep(List *l, const char *s)
{
    long n = strlen(s) + 1;
    char *p;
    if (l->plen + n > l->pcap) {
        /* a new block; the old ones stay where they are (names point into them) */
        long cap = n > 8192 ? n : 8192;
        char *b = cu_alloc(cap + sizeof(char *));
        if (!b) return NULL;
        *(char **)b = l->pool;
        l->pool = b;
        l->plen = sizeof(char *);
        l->pcap = cap + sizeof(char *);
    }
    p = l->pool + l->plen;
    memcpy(p, s, n);
    l->plen += n;
    return p;
}

static Ent *add(List *l)
{
    if (l->n == l->cap) {
        int cap = l->cap ? l->cap * 2 : 128;
        Ent *ne = cu_alloc(cap * sizeof *ne);
        if (!ne) return NULL;
        if (l->e) { memcpy(ne, l->e, l->n * sizeof *ne); cu_free(l->e); }
        l->e = ne; l->cap = cap;
    }
    memset(&l->e[l->n], 0, sizeof *l->e);
    return &l->e[l->n++];
}

static void freelist(List *l)
{
    while (l->pool) { char *n = *(char **)l->pool; cu_free(l->pool); l->pool = n; }
    cu_free(l->e);
    memset(l, 0, sizeof *l);
}

static int shown(Ent *e)
{
    if (o_all) return 1;
    if (e->prot & FIBF_HIDDEN) return 0;
    {
        int n = strlen(e->name);
        if (n > 5 && ieq(e->name + n - 5, ".info")) return 0;
    }
    return 1;
}

/* The drawer's entries (those a parsed pattern matches, if one is given). */
static int readdir_(BPTR lock, const char *pat, List *l)
{
    struct ExAllControl *eac;
    struct ExAllData *buf, *ed;
    LONG more;
    int ok = 1;
    if (!(eac = AllocDosObject(DOS_EXALLCONTROL, NULL))) return 0;
    if (!(buf = cu_alloc(4096))) { FreeDosObject(DOS_EXALLCONTROL, eac); return 0; }
    eac->eac_LastKey = 0;
    eac->eac_MatchString = (STRPTR)pat;
    do {
        more = ExAll(lock, buf, 4096, o_long ? ED_COMMENT : ED_DATE, eac);
        if (!more && IoErr() != ERROR_NO_MORE_ENTRIES) { ok = 0; break; }
        if (eac->eac_Entries == 0) continue;
        for (ed = buf; ed; ed = ed->ed_Next) {
            Ent *e;
            Ent tmp;
            memset(&tmp, 0, sizeof tmp);
            tmp.name = (char *)ed->ed_Name;
            tmp.prot = ed->ed_Prot;
            tmp.type = ed->ed_Type;
            if (!shown(&tmp)) continue;
            if (!(e = add(l))) { ok = 0; break; }
            *e = tmp;
            e->name = keep(l, (char *)ed->ed_Name);
            e->size = ed->ed_Size;
            e->date.ds_Days = ed->ed_Days;
            e->date.ds_Minute = ed->ed_Mins;
            e->date.ds_Tick = ed->ed_Ticks;
            if (o_long && ed->ed_Comment && ed->ed_Comment[0]) e->note = keep(l, (char *)ed->ed_Comment);
            if (!e->name) { ok = 0; break; }
            e->kind = kindof(e);
        }
    } while (more);
    if (more) ExAllEnd(lock, buf, 4096, o_long ? ED_COMMENT : ED_DATE, eac);
    cu_free(buf);
    FreeDosObject(DOS_EXALLCONTROL, eac);
    return ok;
}

static int cmp(Ent *a, Ent *b)
{
    int r;
    if (o_time) {
        r = CompareDates(&a->date, &b->date);      /* > 0: a is older */
        if (!r) r = cu_natcmp(a->name, b->name);
    } else if (o_size) {
        r = a->size < b->size ? 1 : a->size > b->size ? -1 : cu_natcmp(a->name, b->name);
    } else r = cu_natcmp(a->name, b->name);
    return o_rev ? -r : r;
}

/* A merge sort: stable, and fast enough for a drawer of thousands. */
static void sortents(Ent *e, int n)
{
    Ent *t;
    int w, i;
    if (n < 2 || !(t = cu_alloc(n * sizeof *t))) return;
    for (w = 1; w < n; w *= 2) {
        for (i = 0; i < n; i += 2 * w) {
            int a = i, am = i + w < n ? i + w : n, b = am, bm = i + 2 * w < n ? i + 2 * w : n, k = i;
            while (a < am && b < bm) t[k++] = cmp(&e[a], &e[b]) <= 0 ? e[a++] : e[b++];
            while (a < am) t[k++] = e[a++];
            while (b < bm) t[k++] = e[b++];
        }
        memcpy(e, t, n * sizeof *e);
    }
    cu_free(t);
}

/* ---- printing ---- */

static const char *col(int kind) { return colour ? cu_sgr(kind, cterm) : ""; }

static int suffix(Ent *e)
{
    if (!o_class) return 0;
    return e->kind == CU_K_DRAWER ? '/' : e->kind == CU_K_PROGRAM ? '*' : e->kind == CU_K_LINK ? '@' : 0;
}

static void name(Ent *e)
{
    const char *c = col(e->kind);
    int s = suffix(e);
    if (*c) cu_puts(c);
    cu_puts(e->name);
    if (*c) cu_puts("\x1b[0m");
    if (s) cu_putc(s);
}

static void size_(LONG n, int width)
{
    if (o_human && n >= 1024) {
        static const char suf[] = "KMG";
        ULONG unit = 1024, v = n;
        int s = 0;
        char b[12];
        int l;
        while (s < 2 && v >= unit * 1024UL) { unit *= 1024; s++; }
        if (v / unit < 10) {
            ULONG whole = v / unit, tenth = (v % unit) * 10 / unit;
            b[0] = '0' + whole; b[1] = '.'; b[2] = '0' + tenth; b[3] = suf[s]; b[4] = 0; l = 4;
        } else {
            ULONG whole = v / unit;
            char t[12];
            int i = 0;
            while (whole) { t[i++] = '0' + whole % 10; whole /= 10; }
            for (l = 0; i; ) b[l++] = t[--i];
            b[l++] = suf[s]; b[l] = 0;
        }
        while (l++ < width) cu_putc(' ');
        cu_puts(b);
        return;
    }
    cu_putnum(n, width);
}

static void longlist(List *l)
{
    int i, w = 3;                /* "Dir" fits */
    for (i = 0; i < l->n; i++) {
        LONG s = l->e[i].size;
        int d = 1;
        while (s >= 10) { s /= 10; d++; }
        if (l->e[i].type <= 0 && d > w) w = d;
    }
    if (o_human && w > 5) w = 5;
    for (i = 0; i < l->n && !cu_break(); i++) {
        Ent *e = &l->e[i];
        char prot[9], date[16], tm[16];
        struct DateTime dt;
        static const char bits[] = "hsparwed";
        int b;
        for (b = 0; b < 8; b++) {
            int bit = 7 - b, set = (e->prot >> bit) & 1;
            /* rwed are set when NOT allowed; hspa when set */
            prot[b] = (bit < 4 ? !set : set) ? bits[b] : '-';
        }
        prot[8] = 0;
        cu_puts(prot);
        cu_putc(' ');
        if (e->type > 0) { int k; for (k = 0; k < w - 3; k++) cu_putc(' '); cu_puts("Dir"); }
        else size_(e->size, w);
        memset(&dt, 0, sizeof dt);
        dt.dat_Stamp = e->date;
        dt.dat_Format = FORMAT_DOS;
        dt.dat_StrDate = date;
        dt.dat_StrTime = tm;
        date[0] = tm[0] = 0;
        DateToStr(&dt);
        cu_putc(' '); cu_puts(date); cu_putc(' ');
        tm[5] = 0;                       /* hh:mm */
        cu_puts(tm); cu_putc(' ');
        name(e);
        cu_putc('\n');
        if (e->note) { cu_puts(": "); cu_puts(e->note); cu_putc('\n'); }
    }
}

static void columns(List *l)
{
    int i, w = 0, ncol, rows, r, c;
    if (o_one || !colour) {
        for (i = 0; i < l->n && !cu_break(); i++) { name(&l->e[i]); cu_putc('\n'); }
        return;
    }
    for (i = 0; i < l->n; i++) {
        int n = strlen(l->e[i].name) + (suffix(&l->e[i]) != 0);
        if (n > w) w = n;
    }
    w += 2;
    ncol = (cols - 1) / w;
    if (ncol < 1) ncol = 1;
    rows = (l->n + ncol - 1) / ncol;
    for (r = 0; r < rows && !cu_break(); r++) {
        for (c = 0; c < ncol; c++) {
            int k = c * rows + r, n;
            if (k >= l->n) break;
            name(&l->e[k]);
            n = strlen(l->e[k].name) + (suffix(&l->e[k]) != 0);
            if (c + 1 < ncol && (c + 1) * rows + r < l->n) while (n++ < w) cu_putc(' ');
        }
        cu_putc('\n');
    }
}

static void show(List *l)
{
    sortents(l->e, l->n);
    if (o_long) longlist(l); else columns(l);
}

/* ---- drawers, and -R without recursion ---- */

/* the drawers still to list (-R), the next one on top */
typedef struct Pend { struct Pend *next; int depth; char path[1]; } Pend;

/* A drawer listed; with -R its drawers put on the pending list, in order. */
static void listdir(const char *path, int header, int depth, Pend **todo)
{
    List l;
    BPTR lock = Lock((STRPTR)path, SHARED_LOCK);
    memset(&l, 0, sizeof l);
    if (!lock) { cu_fault(5, path); return; }
    if (header) { cu_puts(*path ? path : "\"\""); cu_puts(":\n"); }   /* as typed, as Unix ls -R */
    if (!readdir_(lock, NULL, &l)) cu_fault(5, path);
    UnLock(lock);
    show(&l);
    if (o_rec && depth < 32 && cu_rc < 10) {     /* on past a drawer it could not read (rc 5) */
        int i;
        for (i = l.n - 1; i >= 0; i--) {         /* backwards: the first ends on top */
            Ent *e = &l.e[i];
            Pend *p;
            if (!cu_isdir(e->type)) continue;
            if (strlen(path) + strlen(e->name) + 2 > CU_PATH) continue;
            if (!(p = cu_alloc(sizeof *p + CU_PATH))) break;
            cu_join(p->path, path, e->name);
            p->depth = depth + 1;
            p->next = *todo;
            *todo = p;
        }
    }
    freelist(&l);
}

static void listtree(const char *path, int header)
{
    Pend *todo = NULL, *p;
    listdir(path, header, 0, &todo);
    while ((p = todo)) {
        todo = p->next;
        if (cu_rc < 10) { cu_putc('\n'); listdir(p->path, 1, p->depth, &todo); }
        cu_free(p);
    }
}

/* ---- arguments ---- */

int main(void)
{
    char *args[MAXARGS], **first, *v, *p;
    int nargs = 0, i, c, nfiles = 0, ndirs = 0;
    List files;
    (void)version;
    if (cu_args("ls [-1aAdFhlrRSt] [path | pattern ...]") < 0) return cu_rc;
    first = cu_argv;
    /* flags as Unix bundles, before and after the names: ls Work: -la */
    for (;;) {
        while ((c = cu_opt("1aAdFhlrRSt", &v)) >= 0) switch (c) {
            case '1': o_one = 1; break;
            case 'a': case 'A': o_all = 1; break;
            case 'd': o_dir = 1; break;
            case 'F': o_class = 1; break;
            case 'h': o_human = 1; break;
            case 'l': o_long = 1; break;
            case 'r': o_rev = 1; break;
            case 'R': o_rec = 1; break;
            case 'S': o_size = 1; o_time = 0; break;
            case 't': o_time = 1; o_size = 0; break;
            default: return cu_rc;
        }
        if (!cu_argc) break;
        if (cu_argv > first && !strcmp(cu_argv[-1], "--")) {    /* the rest are names */
            while (cu_argc && nargs < MAXARGS) args[nargs++] = *cu_argv++, cu_argc--;
            if (cu_argc) nargs = MAXARGS;
            break;
        }
        if (nargs == MAXARGS) break;
        args[nargs++] = *cu_argv++;
        cu_argc--;
    }
    if (nargs == MAXARGS) { cu_err(10, NULL, "too many arguments"); return cu_rc; }
    colour = IsInteractive(cu_out);
    if (colour) {
        cterm = cu_iscterm(cu_out);
        cols = termwidth(Input());
        cu_colours_load();
    }
    if (!nargs) args[nargs++] = "";

    /* files and patterns first, as one list; then the drawers */
    memset(&files, 0, sizeof files);
    for (i = 0; i < nargs && cu_rc < 10; i++) {
        BPTR lock;
        struct FileInfoBlock *fib;
        /* a name that is there as typed is a name, ( [ | ~ in it or not */
        if (cu_haswild(args[i]) && !(lock = Lock(args[i], SHARED_LOCK))) {
            static char dir[CU_PATH], parsed[620];
            char *fp = FilePart(args[i]);
            int k;
            BPTR dl;
            int dlen = fp - args[i];
            if (dlen >= (int)sizeof dir) { args[i] = NULL; continue; }
            memcpy(dir, args[i], dlen); dir[dlen] = 0;
            /* * as well as #? (csh hands #? on; the ROM Shell hands * as typed) */
            if (!cu_pattern(fp, parsed, sizeof parsed)) { cu_fault(10, args[i]); break; }
            if (!(dl = Lock(dir, SHARED_LOCK))) { cu_fault(5, dir); args[i] = NULL; continue; }
            {
                int before = files.n, all = o_all;
                o_all = 1;                       /* a pattern shows what it matches */
                readdir_(dl, parsed, &files);
                o_all = all;
                for (k = before; k < files.n; k++) {
                    static char full[CU_PATH];
                    if (dlen + strlen(files.e[k].name) + 1 < sizeof full) {
                        strcpy(full, dir); strcat(full, files.e[k].name);
                        if ((p = keep(&files, full))) files.e[k].name = p;
                    }
                }
                if (files.n == before) cu_err(5, args[i], "nothing matches");
            }
            UnLock(dl);
            nfiles++;
            args[i] = NULL;              /* done: not a drawer to list below */
            continue;
        }
        if (!cu_haswild(args[i]) && !(lock = Lock(args[i], SHARED_LOCK))) { cu_fault(5, args[i]); args[i] = NULL; continue; }
        if ((fib = AllocDosObject(DOS_FIB, NULL))) {
            if (Examine(lock, fib) && (fib->fib_DirEntryType < 0 || o_dir)) {
                Ent *e = add(&files);
                if (e) {
                    e->name = keep(&files, args[i][0] ? args[i] : (char *)fib->fib_FileName);
                    e->size = fib->fib_Size; e->prot = fib->fib_Protection; e->type = fib->fib_DirEntryType;
                    e->date = fib->fib_Date;
                    if (o_long && fib->fib_Comment[0]) e->note = keep(&files, fib->fib_Comment);
                    e->kind = cu_kind(fib->fib_FileName, e->type, e->prot);
                }
                args[i] = NULL;
                nfiles++;
            } else ndirs++;
            FreeDosObject(DOS_FIB, fib);
        }
        UnLock(lock);
    }
    if (files.n) show(&files);
    freelist(&files);
    for (i = 0; i < nargs && cu_rc != 20 && cu_rc != 10; i++) {
        if (!args[i]) continue;
        if (nfiles + ndirs > 1 || o_rec) {
            if (nfiles || i > 0) cu_putc('\n');
        }
        listtree(args[i], nfiles + ndirs > 1 || o_rec);
    }
    cu_flush();
    return cu_rc;
}
