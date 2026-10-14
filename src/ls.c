/* ls - list directory contents, Unix-style, coloured by kind.
 * CShell's ls (docs/design.md "Colours"): CUtils' ls (AmigaTools/cutils,
 * Amiga E) in C, taught the kinds of files.
 *
 *   ls [-1aAdFhlrRSt] [path | pattern ...]
 *
 * Multi-column, natural order (file9 before file10), case blind, drawers and
 * files mixed. A drawer lists its contents, a pattern (#? or *) the matches,
 * a file itself. Columns fit the window: $COLUMNS (csh keeps it), else the
 * console is asked (CSI 0 q). Output that is not the console (a file, a
 * pipe) gets one name a line and no colours.
 *
 * Colours by kind, from ENVARC:FileColours (the family's file: Ahiru and
 * Maki may read it too), written with the defaults when missing:
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
 *   -S by size   -t by date, newest first
 *
 * Return code 0, 5 when a path could not be read, 10 bad arguments,
 * 20 Ctrl-C. */
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/exall.h>
#include <dos/datetime.h>
#include <dos/var.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>

static const char version[] = "$VER: ls 0.2 (13.10.2026)";

#ifndef FIBF_HIDDEN
#define FIBF_HIDDEN (1 << 7)     /* the h bit: not in the NDK's dos.h */
#endif

#define MAXARGS 64

enum { K_TEXT, K_DRAWER, K_LINK, K_PROGRAM, K_SCRIPT, K_ARCHIVE, K_PICTURE, K_SOUND,
       K_SOURCE, K_ICON, K_HIDDEN, K_COUNT };
static const char *const kindnames[K_COUNT] = {
    "text", "drawer", "link", "program", "script", "archive", "picture", "sound",
    "source", "icon", "hidden"
};

typedef struct Ent {
    char *name;
    LONG size, prot, type;
    struct DateStamp date;
    char *note;
    int kind;
} Ent;

static int o_one, o_all, o_dir, o_class, o_human, o_long, o_rev, o_rec, o_size, o_time;
static int colour, cols = 80, cterm, rc;
static BPTR out;

/* ---- output, gathered ---- */

static char obuf[4096];
static int olen;

static void flush_(void) { if (olen) Write(out, obuf, olen); olen = 0; }

static void put(const char *s, int n)
{
    while (n > 0) {
        int k = (int)sizeof obuf - olen;
        if (k > n) k = n;
        memcpy(obuf + olen, s, k);
        olen += k; s += k; n -= k;
        if (olen == (int)sizeof obuf) flush_();
    }
}

static void puts_(const char *s) { put(s, strlen(s)); }

static void putnum(ULONG v, int width)
{
    char t[12], b[12];
    int i = 0, n = 0;
    do { t[i++] = '0' + v % 10; v /= 10; } while (v);
    while (i + n < width && n < 11) b[n++] = ' ';
    while (i) b[n++] = t[--i];
    put(b, n);
}

static int broken(void)
{
    if (SetSignal(0, SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C) {
        flush_();
        PutStr("***Break\n");
        rc = 20;
        return 1;
    }
    return 0;
}

/* ---- names ---- */

static int lc(int c)
{
    c &= 0xff;
    if ((c >= 'A' && c <= 'Z') || (c >= 0xc0 && c <= 0xde && c != 0xd7)) c += 32;
    return c;
}

/* natural, case blind: "file9" before "file10" */
static int natcmp(const char *a, const char *b)
{
    while (*a && *b) {
        if (*a >= '0' && *a <= '9' && *b >= '0' && *b <= '9') {
            const char *x = a, *y = b;
            int lx, ly;
            while (*x == '0') x++;
            while (*y == '0') y++;
            for (lx = 0; x[lx] >= '0' && x[lx] <= '9'; lx++) ;
            for (ly = 0; y[ly] >= '0' && y[ly] <= '9'; ly++) ;
            if (lx != ly) return lx - ly;
            for (; lx; lx--, x++, y++) if (*x != *y) return *x - *y;
            a = x; b = y;
            continue;
        }
        if (lc(*a) != lc(*b)) return lc(*a) - lc(*b);
        a++; b++;
    }
    return lc(*a) - lc(*b);
}

static int ieq(const char *a, const char *b)
{
    while (*a && lc(*a) == lc(*b)) a++, b++;
    return !*a && !*b;
}

/* ---- kinds and colours ---- */

#define MAXEXT 160
static char extname[MAXEXT][12];
static char extkind[MAXEXT];
static int next_;
static char sgr[2][K_COUNT][20];

static const char deffile[] =
    "; File kinds and their colours, for csh's ls (and Ahiru and Maki, later).\n"
    "; KIND CTERM,PLAIN: SGR numbers in CTerm and in a plain Shell window; '-' = none.\n"
    "; In a CTerm Shell tab 30-37 are the Amiga's pens: its theme's colours are 90 grey\n"
    "; 91 red 92 green 93 yellow 94 blue 95 magenta 96 cyan 97 white. A plain window on\n"
    "; a stock Workbench has pens 30 grey 31 black 32 white 33 blue. 1 bold, 3 italic.\n"
    "; .EXT KIND puts a name ending in .ext into a kind (after the built-in list).\n"
    "drawer 94,33\n"
    "link 96,3\n"
    "program 92,1\n"
    "script 92,1\n"
    "archive 91,-\n"
    "picture 93,-\n"
    "sound 95,-\n"
    "source 96,-\n"
    "text -,-\n"
    "icon 90,32\n"
    "hidden 90,32\n";

static const struct { const char *exts; int kind; } builtin[] = {
    { "|lha|lzh|lzx|zip|dms|adf|hdf|iso|gz|tgz|tar|7z|rar|xz|bz2|lz|z|", K_ARCHIVE },
    { "|iff|ilbm|lbm|pic|gif|jpg|jpeg|png|bmp|pcx|tga|ham|brush|br|anim|anm|ico|webp|", K_PICTURE },
    { "|mod|med|mmd0|mmd1|mmd2|xm|s3m|it|8svx|svx|wav|mp3|aiff|aif|sid|ahx|thx|ogg|flac|bp|tfmx|okt|sng|", K_SOUND },
    { "|e|c|h|s|asm|i|py|rexx|rx|cpp|hpp|bas|sh|m|amos|csh|", K_SOURCE },
    { NULL, 0 }
};

static void addext(const char *e, int kind)
{
    int i;
    if (strlen(e) > 11) return;
    for (i = 0; i < next_; i++) if (ieq(extname[i], e)) { extkind[i] = kind; return; }
    if (next_ == MAXEXT) return;
    strcpy(extname[next_], e);
    extkind[next_++] = kind;
}

static void setsgr(char *d, const char *v, int n)
{
    if (n <= 0 || n > 14 || (n == 1 && *v == '-')) { d[0] = 0; return; }
    d[0] = 0x1b; d[1] = '[';
    memcpy(d + 2, v, n);
    d[n + 2] = 'm'; d[n + 3] = 0;
}

static void parsefile(char *t)
{
    char *line = t;
    while (line && *line) {
        char *nl = strchr(line, '\n'), *k, *v, *e;
        int i;
        if (nl) *nl = 0;
        k = line;
        while (*k == ' ' || *k == '\t') k++;
        if (*k && *k != ';' && *k != '#') {
            for (e = k; *e && *e != ' ' && *e != '\t'; e++) ;
            v = e;
            while (*v == ' ' || *v == '\t') v++;
            *e = 0;
            for (e = v; *e && *e != ' ' && *e != '\t' && *e != '\r'; e++) ;
            *e = 0;
            for (i = 0; i < K_COUNT && !ieq(k[0] == '.' ? v : k, kindnames[i]); i++) ;
            if (i < K_COUNT) {
                if (k[0] == '.') addext(k + 1, i);
                else {
                    char *comma = strchr(v, ',');
                    setsgr(sgr[0][i], v, comma ? comma - v : (int)strlen(v));
                    if (comma) setsgr(sgr[1][i], comma + 1, strlen(comma + 1));
                    else strcpy(sgr[1][i], sgr[0][i]);
                }
            }
        }
        line = nl ? nl + 1 : NULL;
    }
}

static char *readall(const char *path)
{
    BPTR fh = Open((STRPTR)path, MODE_OLDFILE);
    char *t;
    LONG n;
    if (!fh) return NULL;
    Seek(fh, 0, OFFSET_END);
    n = Seek(fh, 0, OFFSET_BEGINNING);
    if (n < 0 || n > 32768 || !(t = AllocVec(n + 1, MEMF_ANY))) { Close(fh); return NULL; }
    n = Read(fh, t, n);
    Close(fh);
    t[n > 0 ? n : 0] = 0;
    return t;
}

static void loadcolours(void)
{
    static char d[sizeof deffile];
    char *t;
    int i;
    for (i = 0; builtin[i].exts; i++) {
        const char *p = builtin[i].exts + 1;
        while (*p) {
            char e[12];
            int n = 0;
            while (*p && *p != '|' && n < 11) e[n++] = *p++;
            e[n] = 0;
            if (*p == '|') p++;
            addext(e, builtin[i].kind);
        }
    }
    memcpy(d, deffile, sizeof deffile);
    parsefile(d);
    if (!(t = readall("ENV:FileColours")) && !(t = readall("ENVARC:FileColours"))) {
        /* written with the defaults; no requester on a write-protected disk */
        struct Process *me = (struct Process *)FindTask(NULL);
        APTR win = me->pr_WindowPtr;
        BPTR fh;
        me->pr_WindowPtr = (APTR)-1;
        if ((fh = Open("ENVARC:FileColours", MODE_NEWFILE))) { Write(fh, (APTR)deffile, sizeof deffile - 1); Close(fh); }
        if ((fh = Open("ENV:FileColours", MODE_NEWFILE))) { Write(fh, (APTR)deffile, sizeof deffile - 1); Close(fh); }
        me->pr_WindowPtr = win;
        return;
    }
    parsefile(t);
    FreeVec(t);
}

static int kindof(Ent *e)
{
    const char *dot = NULL, *p;
    int i;
    if (e->type == ST_SOFTLINK || e->type == ST_LINKDIR || e->type == ST_LINKFILE) return K_LINK;
    if (e->type > 0) return K_DRAWER;
    for (p = e->name; *p; p++) if (*p == '.') dot = p;
    if (dot && ieq(dot, ".info")) return K_ICON;
    if (e->prot & FIBF_HIDDEN) return K_HIDDEN;
    if (e->prot & FIBF_SCRIPT) return K_SCRIPT;
    if (!strncmp(e->name, "mod.", 4) || !strncmp(e->name, "MOD.", 4)) return K_SOUND;
    if (dot && dot[1]) {
        for (i = 0; i < next_; i++) if (ieq(extname[i], dot + 1)) return extkind[i];
        return K_TEXT;
    }
    /* no ending and the e bit: a program, Ahiru's rule too. Reading each
     * file's first long word (a hunk header) was exact but tripled the time
     * of a drawer like C: (100 opens), slower than C:Dir */
    if (!(e->prot & FIBF_EXECUTE)) return K_PROGRAM;
    return K_TEXT;
}

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
    Write(out, "\x9b" "0 q", 4);
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

static int iscterm(BPTR fh)
{
    struct FileHandle *f = BADDR(fh);
    struct InfoData *id;
    int yes = 0;
    if (!f || !f->fh_Type) return 0;
    if ((id = AllocVec(sizeof *id, MEMF_PUBLIC | MEMF_CLEAR))) {
        if (DoPkt(f->fh_Type, ACTION_DISK_INFO, MKBADDR(id), 0, 0, 0, 0)) yes = id->id_DiskType == 0x4354524DUL;
        FreeVec(id);
    }
    return yes;
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
        char *b = AllocVec(cap + sizeof(char *), MEMF_ANY);
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
        Ent *ne = AllocVec(cap * sizeof *ne, MEMF_ANY | MEMF_CLEAR);
        if (!ne) return NULL;
        if (l->e) { memcpy(ne, l->e, l->n * sizeof *ne); FreeVec(l->e); }
        l->e = ne; l->cap = cap;
    }
    memset(&l->e[l->n], 0, sizeof *l->e);
    return &l->e[l->n++];
}

static void freelist(List *l)
{
    while (l->pool) { char *n = *(char **)l->pool; FreeVec(l->pool); l->pool = n; }
    if (l->e) FreeVec(l->e);
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
static int readdir_(BPTR lock, const char *dirname, const char *pat, List *l)
{
    struct ExAllControl *eac;
    struct ExAllData *buf, *ed;
    LONG more;
    int ok = 1;
    if (!(eac = AllocDosObject(DOS_EXALLCONTROL, NULL))) return 0;
    if (!(buf = AllocVec(4096, MEMF_ANY))) { FreeDosObject(DOS_EXALLCONTROL, eac); return 0; }
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
    FreeVec(buf);
    FreeDosObject(DOS_EXALLCONTROL, eac);
    return ok;
}

static int cmp(Ent *a, Ent *b)
{
    int r;
    if (o_time) {
        r = CompareDates(&a->date, &b->date);      /* > 0: a is older */
        if (!r) r = natcmp(a->name, b->name);
    } else if (o_size) {
        r = a->size < b->size ? 1 : a->size > b->size ? -1 : natcmp(a->name, b->name);
    } else r = natcmp(a->name, b->name);
    return o_rev ? -r : r;
}

/* A merge sort: stable, and fast enough for a drawer of thousands. */
static void sortents(Ent *e, int n)
{
    Ent *t;
    int w, i;
    if (n < 2 || !(t = AllocVec(n * sizeof *t, MEMF_ANY))) return;
    for (w = 1; w < n; w *= 2) {
        for (i = 0; i < n; i += 2 * w) {
            int a = i, am = i + w < n ? i + w : n, b = am, bm = i + 2 * w < n ? i + 2 * w : n, k = i;
            while (a < am && b < bm) t[k++] = cmp(&e[a], &e[b]) <= 0 ? e[a++] : e[b++];
            while (a < am) t[k++] = e[a++];
            while (b < bm) t[k++] = e[b++];
        }
        memcpy(e, t, n * sizeof *e);
    }
    FreeVec(t);
}

/* ---- printing ---- */

static const char *col(int kind) { return colour ? sgr[cterm ? 0 : 1][kind] : ""; }

static int suffix(Ent *e)
{
    if (!o_class) return 0;
    return e->kind == K_DRAWER ? '/' : e->kind == K_PROGRAM ? '*' : e->kind == K_LINK ? '@' : 0;
}

static void name(Ent *e)
{
    const char *c = col(e->kind);
    int s = suffix(e);
    if (*c) puts_(c);
    puts_(e->name);
    if (*c) puts_("\x1b[0m");
    if (s) { char b = s; put(&b, 1); }
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
        while (l++ < width) put(" ", 1);
        puts_(b);
        return;
    }
    putnum(n, width);
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
    for (i = 0; i < l->n && !broken(); i++) {
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
        puts_(prot);
        put(" ", 1);
        if (e->type > 0) { int k; for (k = 0; k < w - 3; k++) put(" ", 1); puts_("Dir"); }
        else size_(e->size, w);
        memset(&dt, 0, sizeof dt);
        dt.dat_Stamp = e->date;
        dt.dat_Format = FORMAT_DOS;
        dt.dat_StrDate = date;
        dt.dat_StrTime = tm;
        date[0] = tm[0] = 0;
        DateToStr(&dt);
        put(" ", 1); puts_(date); put(" ", 1);
        tm[5] = 0;                       /* hh:mm */
        puts_(tm); put(" ", 1);
        name(e);
        put("\n", 1);
        if (e->note) { puts_(": "); puts_(e->note); put("\n", 1); }
    }
}

static void columns(List *l)
{
    int i, w = 0, ncol, rows, r, c;
    if (o_one || !colour) {
        for (i = 0; i < l->n && !broken(); i++) { name(&l->e[i]); put("\n", 1); }
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
    for (r = 0; r < rows && !broken(); r++) {
        for (c = 0; c < ncol; c++) {
            int k = c * rows + r, n;
            if (k >= l->n) break;
            name(&l->e[k]);
            n = strlen(l->e[k].name) + (suffix(&l->e[k]) != 0);
            if (c + 1 < ncol && (c + 1) * rows + r < l->n) while (n++ < w) put(" ", 1);
        }
        put("\n", 1);
    }
}

static void show(List *l)
{
    sortents(l->e, l->n);
    if (o_long) longlist(l); else columns(l);
}

/* ---- arguments ---- */

static int haswild(const char *s)
{
    for (; *s; s++) if (*s == '#' || *s == '?' || *s == '*' || *s == '|' || *s == '(' || *s == '[' || *s == '~') return 1;
    return 0;
}

static void listdir(const char *path, int header, int depth);

/* Bytes of stack left: ls runs on the Shell's stack (often 4 KB), and
 * no MMU stops it running off the end. */
static long stackleft(void)
{
    char here;
    return &here - (char *)FindTask(NULL)->tc_SPLower;
}

/* Into each drawer; the paths on the heap, a few hundred bytes of stack a
 * level, and a stop with a word when the stack runs short. */
static void recurse(List *l, const char *path, int depth)
{
    int i;
    char *sub;
    if (stackleft() < 1200) { flush_(); PutStr("ls: too deep for the stack (Stack 16000 helps)\n"); if (rc < 5) rc = 5; return; }
    if (!(sub = AllocVec(512, MEMF_ANY))) return;
    for (i = 0; i < l->n && rc < 10; i++) {
        Ent *e = &l->e[i];
        if (e->type <= 0 || e->type == ST_SOFTLINK || e->type == ST_LINKDIR) continue;
        if (strlen(path) + strlen(e->name) + 2 > 512) continue;
        strcpy(sub, path);
        AddPart(sub, e->name, 512);
        put("\n", 1);
        listdir(sub, 1, depth + 1);
    }
    FreeVec(sub);
}

static void listdir(const char *path, int header, int depth)
{
    List l;
    BPTR lock = Lock((STRPTR)path, SHARED_LOCK);
    memset(&l, 0, sizeof l);
    if (!lock) { flush_(); PrintFault(IoErr(), (STRPTR)path); if (rc < 5) rc = 5; return; }
    if (header) { puts_(*path ? path : "\"\""); puts_(":\n"); }   /* as typed, as Unix ls -R */
    if (!readdir_(lock, path, NULL, &l)) { flush_(); PrintFault(IoErr(), (STRPTR)path); if (rc < 5) rc = 5; }
    UnLock(lock);
    show(&l);
    if (o_rec && depth < 32 && rc < 10) recurse(&l, path, depth);   /* on past a drawer it could not read (rc 5) */
    freelist(&l);
}

int main(void)
{
    char *args[MAXARGS], *line, *p;
    int nargs = 0, i, nfiles = 0, ndirs = 0;
    List files;
    (void)version;
    out = Output();
    line = GetArgStr();
    /* flags as Unix bundles: ls -la Work: (ReadArgs would take -la for a name) */
    for (p = line; p && *p && nargs < MAXARGS; ) {
        char *s;
        while (*p == ' ' || *p == '\t') p++;
        if (!*p || *p == '\n') break;
        if (*p == '"') {
            s = ++p;
            while (*p && *p != '"' && *p != '\n') p++;
        } else {
            s = p;
            while (*p && *p != ' ' && *p != '\t' && *p != '\n') p++;
        }
        if (*p) *p++ = 0;
        if (s[0] == '?' && !s[1]) {
            PutStr("ls [-1aAdFhlrRSt] [path | pattern ...]\n");
            return 0;
        }
        if (s[0] == '-' && s[1]) {
            for (s++; *s; s++) switch (*s) {
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
                default: { char b[2] = { *s, 0 }; Printf("ls: unknown option -%s\n", (LONG)b); return 10; }
            }
            continue;
        }
        args[nargs++] = s;
    }
    if (nargs == MAXARGS) { PutStr("ls: too many arguments\n"); return 10; }
    colour = IsInteractive(out);
    if (colour) {
        cterm = iscterm(out);
        cols = termwidth(Input());
        loadcolours();
    }
    if (!nargs) args[nargs++] = "";

    /* files and patterns first, as one list; then the drawers */
    memset(&files, 0, sizeof files);
    for (i = 0; i < nargs && rc < 10; i++) {
        BPTR lock;
        struct FileInfoBlock *fib;
        /* a name that is there as typed is a name, ( [ | ~ in it or not */
        if (haswild(args[i]) && !(lock = Lock(args[i], SHARED_LOCK))) {
            static char dir[512], pat[300], parsed[620];
            char *fp = FilePart(args[i]);
            int k;
            BPTR dl;
            int dlen = fp - args[i];
            if (dlen >= (int)sizeof dir) { args[i] = NULL; continue; }
            memcpy(dir, args[i], dlen); dir[dlen] = 0;
            /* * as well as #? (csh hands #? on; the ROM Shell hands * as typed) */
            for (k = 0, p = fp; *p && k < (int)sizeof pat - 3; p++) {
                if (*p == '*') { pat[k++] = '#'; pat[k++] = '?'; } else pat[k++] = *p;
            }
            pat[k] = 0;
            if (ParsePatternNoCase(pat, parsed, sizeof parsed) < 0) { PrintFault(IoErr(), args[i]); rc = 10; break; }
            if (!(dl = Lock(dir, SHARED_LOCK))) { PrintFault(IoErr(), dir[0] ? dir : (char *)""); if (rc < 5) rc = 5; args[i] = NULL; continue; }
            {
                int before = files.n, all = o_all;
                o_all = 1;                       /* a pattern shows what it matches */
                readdir_(dl, dir, parsed, &files);
                o_all = all;
                for (k = before; k < files.n; k++) {
                    static char full[512];
                    if (dlen + strlen(files.e[k].name) + 1 < sizeof full) {
                        strcpy(full, dir); strcat(full, files.e[k].name);
                        files.e[k].name = keep(&files, full);
                    }
                }
                if (files.n == before) { flush_(); Printf("ls: %s: nothing matches\n", args[i]); if (rc < 5) rc = 5; }
            }
            UnLock(dl);
            nfiles++;
            args[i] = NULL;              /* done: not a drawer to list below */
            continue;
        }
        if (!haswild(args[i]) && !(lock = Lock(args[i], SHARED_LOCK))) { flush_(); PrintFault(IoErr(), args[i]); if (rc < 5) rc = 5; args[i] = NULL; continue; }
        if ((fib = AllocDosObject(DOS_FIB, NULL))) {
            if (Examine(lock, fib) && (fib->fib_DirEntryType < 0 || o_dir)) {
                Ent *e = add(&files);
                if (e) {
                    e->name = keep(&files, args[i][0] ? args[i] : (char *)fib->fib_FileName);
                    e->size = fib->fib_Size; e->prot = fib->fib_Protection; e->type = fib->fib_DirEntryType;
                    e->date = fib->fib_Date;
                    if (o_long && fib->fib_Comment[0]) e->note = keep(&files, fib->fib_Comment);
                    {
                        Ent t = *e;
                        t.name = (char *)fib->fib_FileName;
                        e->kind = kindof(&t);
                    }
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
    for (i = 0; i < nargs && rc != 20 && rc != 10; i++) {
        if (!args[i]) continue;
        if (nfiles + ndirs > 1 || o_rec) {
            if (nfiles || i > 0) put("\n", 1);
        }
        listdir(args[i], nfiles + ndirs > 1 || o_rec, 0);
    }
    flush_();
    return rc;
}
