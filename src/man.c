/* man - manual pages.
 *
 *   man [-a] [-w] [-L LANG] NAME ...
 *   man -k WORD
 *   man -l FILE
 *
 * Shows the page for a command or program, in its own viewer when the
 * output is a window (scroll, search, q), as plain text when it is a file
 * or a pipe (`man ls | grep -i sort`). With ENV:PAGER set (e.g. Maki) the
 * page goes to that viewer instead, in colour.
 *
 * Where pages are found (docs/man.md has the why), the first one wins:
 *   MAN:LANG/PROGRAM/NAME.man         our pages (MAN: is an assign, often
 *                                     HELP:Man; Assign ADD for your own)
 *   HELP:LANG/Sys/DRAWER/NAME.help    the system's own (3.2: 110 commands)
 *   the same two in english
 *   NAME.guide, NAME.doc, NAME.readme next to the program (found as the
 *   Shell finds commands), or at the top of HELP:
 * LANG is the one set in Locale prefs (english without locale.library).
 * Pages may be .man (marked-up text), plain text, or AmigaGuide.
 *
 *   -a every page found, one after another   -w where the page is, not
 *   the page   -L LANG another language   -k WORD the pages whose name
 *   or one-line summary has WORD in it   -l FILE show that file as a page
 *
 * In the viewer: Down/j/Return a line, Up/k back, Space/PgDn/f a page,
 * b/PgUp back, g/Home the start, G/End the end, / search, n/N the next
 * and the one before, the mouse wheel, h help, q/Esc quit.
 *
 * Return code 0, 5 no page found, 10 bad arguments, 20 Ctrl-C. */
#include "cu.h"
#include "tui.h"
#include <stdlib.h>
#include <proto/locale.h>
#include <libraries/locale.h>
#include <dos/var.h>
#include <dos/dostags.h>
#include <utility/tagitem.h>

const char *cu_name = "man";
static const char version[] = "$VER: man 0.1b1 (14.10.2026)";

struct LocaleBase *LocaleBase;

/* styles inside a rendered line: zero-width marks */
#define S_BOLD  '\x01'
#define S_UNDER '\x02'
#define S_HEAD  '\x03'
#define S_OFF   '\x04'
#define ISMARK(c) ((unsigned char)(c) >= 1 && (unsigned char)(c) <= 4)

static char lang[40] = "english";
static int o_all, o_where;

/* ---- finding pages ---------------------------------------------------- */

static int exists(const char *path)
{
    APTR win = ((struct Process *)FindTask(NULL))->pr_WindowPtr;
    BPTR l;
    ((struct Process *)FindTask(NULL))->pr_WindowPtr = (APTR)-1;   /* no "insert volume" */
    l = Lock((STRPTR)path, SHARED_LOCK);
    ((struct Process *)FindTask(NULL))->pr_WindowPtr = win;
    if (l) UnLock(l);
    return l != 0;
}

/* Every drawer an assign stands for (a multi-assign has several): fn gets
 * each one's path. */
static int eachdir(const char *assign, int (*fn)(const char *dir, void *u), void *u)
{
    struct DevProc *dp = NULL;
    char dir[CU_PATH];
    int r = 0;
    struct MsgPort *fsport;
    APTR win = ((struct Process *)FindTask(NULL))->pr_WindowPtr;
    ((struct Process *)FindTask(NULL))->pr_WindowPtr = (APTR)-1;
    while (!r && (dp = GetDeviceProc((STRPTR)assign, dp))) {
        fsport = dp->dvp_Port;
        (void)fsport;
        if (dp->dvp_Lock && NameFromLock(dp->dvp_Lock, dir, sizeof dir)) r = fn(dir, u);
        else if (!dp->dvp_Lock) r = fn(assign, u);
        if (!(dp->dvp_Flags & DVPF_ASSIGN)) break;
    }
    if (dp) FreeDeviceProc(dp);
    ((struct Process *)FindTask(NULL))->pr_WindowPtr = win;
    return r;
}

/* The drawers inside dir (one level), each given to fn with its path. */
static int eachsub(const char *dir, int (*fn)(const char *sub, void *u), void *u)
{
    BPTR l;
    struct FileInfoBlock *fib;
    char **subs = NULL;
    int n = 0, max = 0, i, r = 0;
    APTR win = ((struct Process *)FindTask(NULL))->pr_WindowPtr;
    ((struct Process *)FindTask(NULL))->pr_WindowPtr = (APTR)-1;
    l = Lock((STRPTR)dir, SHARED_LOCK);
    ((struct Process *)FindTask(NULL))->pr_WindowPtr = win;
    if (!l) return 0;
    if ((fib = AllocDosObject(DOS_FIB, NULL))) {
        if (Examine(l, fib)) while (ExNext(l, fib)) {
            char p[CU_PATH];
            if (fib->fib_DirEntryType <= 0 || fib->fib_DirEntryType == ST_SOFTLINK) continue;
            if (!cu_join(p, dir, fib->fib_FileName)) continue;
            if (n == max) {
                char **t = cu_alloc((max = max ? max * 2 : 16) * sizeof *t);
                if (!t) break;
                if (subs) { memcpy(t, subs, n * sizeof *t); cu_free(subs); }
                subs = t;
            }
            if (!(subs[n] = cu_strdup(p))) break;
            n++;
        }
        FreeDosObject(DOS_FIB, fib);
    }
    UnLock(l);
    for (i = 0; i < n; i++) {
        if (!r) r = fn(subs[i], u);
        cu_free(subs[i]);
    }
    cu_free(subs);
    return r;
}

typedef struct {
    const char *name, *ext, *lang, *mid;   /* mid: "" or "Sys" between LANG and PROGRAM */
    char found[CU_PATH];
    int count;                              /* -a: pages shown so far */
    int (*show)(const char *path);
} Find;

static int trysub(const char *sub, void *u)
{
    Find *f = u;
    char p[CU_PATH], file[64];
    if (strlen(f->name) + strlen(f->ext) + 1 > sizeof file) return 0;
    strcpy(file, f->name); strcat(file, f->ext);
    if (!cu_join(p, sub, file) || !exists(p)) return 0;
    strcpy(f->found, p);
    f->count++;
    if (o_all && f->show) { f->show(p); return 0; }
    return 1;
}

static int trylang(const char *dir, void *u)
{
    Find *f = u;
    char p[CU_PATH];
    if (!cu_join(p, dir, f->lang)) return 0;
    if (*f->mid && !cu_join(p, p, f->mid)) return 0;
    return eachsub(p, trysub, f);
}

/* The program's own documents, next to the program */
static int nearprogram(Find *f)
{
    static const char *const exts[] = { ".guide", ".doc", ".readme", NULL };
    struct CommandLineInterface *cli = Cli();
    BPTR *pl;
    char dir[CU_PATH], p[CU_PATH];
    int i;
    /* the command path, then C: */
    for (pl = cli ? BADDR(cli->cli_CommandDir) : NULL; ; pl = BADDR(pl[0])) {
        if (pl) {
            if (!NameFromLock(pl[1], dir, sizeof dir)) continue;
        } else strcpy(dir, "C:");
        if (cu_join(p, dir, f->name) && exists(p)) {
            /* found the program: its documents, then a Docs drawer beside it */
            for (i = 0; exts[i]; i++) {
                char d[CU_PATH], file[64];
                if (strlen(f->name) + 8 > sizeof file) break;
                strcpy(file, f->name); strcat(file, exts[i]);
                if (cu_join(d, dir, file) && exists(d)) { strcpy(f->found, d); return 1; }
                if (cu_join(d, dir, "Docs") && cu_join(d, d, file) && exists(d)) { strcpy(f->found, d); return 1; }
            }
        }
        if (!pl) break;
    }
    return 0;
}

static int tophelp(const char *dir, void *u)
{
    static const char *const exts[] = { ".guide", ".doc", NULL };
    Find *f = u;
    char p[CU_PATH], file[64];
    int i;
    for (i = 0; exts[i]; i++) {
        if (strlen(f->name) + 8 > sizeof file) return 0;
        strcpy(file, f->name); strcat(file, exts[i]);
        if (cu_join(p, dir, file) && exists(p)) { strcpy(f->found, p); return 1; }
    }
    return 0;
}

static int findpage(const char *name, char *out, int (*show)(const char *path), int *count)
{
    Find f;
    int pass;
    memset(&f, 0, sizeof f);
    f.name = name;
    f.show = show;
    for (pass = 0; pass < 2; pass++) {
        f.lang = pass ? "english" : lang;
        if (pass && !strcmp(lang, "english")) break;
        f.ext = ".man"; f.mid = "";
        if (eachdir("MAN:", trylang, &f)) goto found;
        if (!exists("MAN:")) {               /* no MAN: yet: where it would point */
            if (trylang("HELP:Man", &f)) goto found;
            if (trylang("SYS:Man", &f)) goto found;
        }
        f.ext = ".help"; f.mid = "Sys";
        if (eachdir("HELP:", trylang, &f)) goto found;
    }
    if (o_all && f.count) { *count = f.count; return 1; }
    if (nearprogram(&f)) goto found;
    if (eachdir("HELP:", tophelp, &f)) goto found;
    *count = f.count;
    return f.count > 0;
found:
    strcpy(out, f.found);
    *count = f.count;
    return 1;
}

/* ---- the text, in lines --------------------------------------------- */

typedef struct { char **l; int n, max; } Lines;

static int addline(Lines *ls, const char *s, int n)
{
    char *c;
    if (ls->n == ls->max) {
        char **t = cu_alloc((ls->max = ls->max ? ls->max * 2 : 256) * sizeof *t);
        if (!t) return 0;
        if (ls->l) { memcpy(t, ls->l, ls->n * sizeof *t); cu_free(ls->l); }
        ls->l = t;
    }
    if (!(c = cu_alloc(n + 1))) return 0;
    memcpy(c, s, n); c[n] = 0;
    ls->l[ls->n++] = c;
    return 1;
}

static void freelines(Lines *ls)
{
    int i;
    for (i = 0; i < ls->n; i++) cu_free(ls->l[i]);
    cu_free(ls->l);
    memset(ls, 0, sizeof *ls);
}

/* the file, every line (tabs out to 8) */
static int readlines(const char *path, Lines *src)
{
    CuIn in;
    char *line, buf[1024];
    LONG n;
    int nl;
    if (!cu_open(&in, path)) return 0;
    while ((n = cu_line(&in, &line, &nl)) >= 0) {
        int i, k = 0;
        for (i = 0; i < n && k < (int)sizeof buf - 9; i++) {
            if (line[i] == '\t') { do buf[k++] = ' '; while (k & 7); }
            else if (line[i] == '\r' && i == n - 1) ;
            else buf[k++] = line[i];
        }
        if (!addline(src, buf, k)) break;
        if (cu_break()) break;
    }
    cu_close(&in);
    return 1;
}

/* ---- rendering: source lines to screen lines of the given width ------- */

static int W;                          /* the width to fill */
static Lines out;
static char ob[2048];
static int olen, ocol;                 /* ocol: visible columns so far */

static void emit(void) { addline(&out, ob, olen); olen = ocol = 0; }
static void obyte(int c) { if (olen < (int)sizeof ob - 1) ob[olen++] = c; if (!ISMARK(c)) ocol++; }
static void ospaces(int n) { while (n-- > 0) obyte(' '); }

/* a paragraph of text with marks, wrapped at W, first line at col0, the
 * rest at col: words never split unless longer than a line */
static void wrap(const char *t, int col0, int col)
{
    const char *p = t;
    ospaces(col0 - ocol);
    while (*p) {
        const char *w = p;
        int vis = 0;
        while (*p == ' ') p++;
        w = p;
        while (*p && *p != ' ') { if (!ISMARK(*p)) vis++; p++; }
        if (p == w) break;
        if (ocol > col && ocol + 1 + vis > W) { emit(); ospaces(col); }
        else if (ocol > col0 && ocol > col) obyte(' ');
        else if (ocol > col0 && w > t) obyte(' ');
        while (w < p) obyte(*w++);
    }
}

/* inline marks of a .man line: **bold** `code` *under* \x */
static void inline_(const char *s, char *d, int size)
{
    int k = 0, bold = 0, code = 0, under = 0;
    for (; *s && k < size - 3; s++) {
        if (*s == '\\' && (s[1] == '*' || s[1] == '`' || s[1] == '\\')) { d[k++] = *++s; continue; }
        if (s[0] == '*' && s[1] == '*' && !code) { bold = !bold; d[k++] = bold ? S_BOLD : S_OFF; s++; continue; }
        if (*s == '`') { code = !code; d[k++] = code ? S_BOLD : S_OFF; continue; }
        if (*s == '*' && !code && (under || (s[1] && s[1] != ' '))) { under = !under; d[k++] = under ? S_UNDER : S_OFF; continue; }
        if (ISMARK(*s)) continue;
        d[k++] = *s;
    }
    if (bold || code || under) d[k++] = S_OFF;
    d[k] = 0;
}

static int indentof(const char *s) { int i = 0; while (s[i] == ' ') i++; return s[i] ? i : -1; }

#define BODY 4                         /* where the text of a section starts */
#define TERMW 12                       /* an option's term column width */

static void render_man(Lines *src)
{
    static char para[4096], t[4096];
    int i = 0, plen;
    while (i < src->n) {
        char *s = src->l[i];
        int ind = indentof(s);
        if (ind < 0) { i++; if (out.n && out.l[out.n - 1][0]) addline(&out, "", 0); continue; }
        if (s[0] == '#' && s[1] == ' ') {                     /* the title */
            char *dash = strstr(s + 2, " - ");
            olen = ocol = 0;
            obyte(S_HEAD); { const char *h = "NAME"; while (*h) obyte(*h++); } obyte(S_OFF); emit();
            ospaces(BODY);
            if (dash) { obyte(S_BOLD); { char *c = s + 2; while (c < dash) obyte(*c++); } obyte(S_OFF); obyte(' '); inline_(dash + 1, t, sizeof t); wrap(t, ocol, BODY + 2); }
            else { inline_(s + 2, t, sizeof t); wrap(t, BODY, BODY); }
            emit();
            i++;
            continue;
        }
        if (s[0] == '#' && s[1] == '#' && s[2] == ' ') {      /* a section */
            if (out.n && out.l[out.n - 1][0]) addline(&out, "", 0);
            olen = ocol = 0;
            obyte(S_HEAD);
            { char *c = s + 3; while (*c) obyte(*c++); }
            obyte(S_OFF);
            emit();
            i++;
            continue;
        }
        if (ind >= 4) {                                        /* as it is */
            olen = ocol = 0;
            ospaces(BODY + 2 + ind - 4);
            { char *c = s + ind; while (*c) obyte(*c++); }
            emit();
            i++;
            continue;
        }
        if (s[0] == '-' && s[1] == ' ') {                      /* a list item and its lines */
            plen = 0;
            plen += strlen(strcpy(para, s + 2));
            for (i++; i < src->n && indentof(src->l[i]) == 2 && src->l[i][2] != '-'; i++) {
                if (plen + strlen(src->l[i]) + 2 >= sizeof para) break;
                para[plen++] = ' ';
                strcpy(para + plen, src->l[i] + 2);
                plen += strlen(src->l[i] + 2);
            }
            olen = ocol = 0;
            if (para[0] == '*' && para[1] == '*' && strstr(para + 2, "**")) {
                char *e = strstr(para + 2, "**");
                int tw;
                *e = 0;
                inline_(para + 2, t, sizeof t);
                ospaces(BODY);
                obyte(S_BOLD);
                { char *c = t; while (*c) obyte(*c++); }
                obyte(S_OFF);
                /* the option's arguments belong to the term: words in capitals
                 * right after it (-e PAT, -m N, -k POS1[,POS2], --colour=WHEN) */
                e += 2;
                for (tw = 0; tw < 1; tw++) {         /* one argument word at most */
                    char *w = e, *x;
                    int caps = 0, ok = 1;
                    while (*w == ' ') w++;
                    for (x = w; *x && *x != ' '; x++) {
                        if (*x >= 'A' && *x <= 'Z') caps = 1;
                        else if (!((*x >= '0' && *x <= '9') || strchr("[],=|+-./:<>_", *x))) { ok = 0; break; }
                    }
                    if (!ok || !caps || x == w) break;
                    if (w > e) obyte(' ');
                    obyte(S_UNDER);
                    while (w < x) obyte(*w++);
                    obyte(S_OFF);
                    e = x;
                }
                inline_(e, t, sizeof t);
                tw = ocol - BODY;
                if (tw + 1 >= TERMW) { emit(); wrap(t, BODY + TERMW, BODY + TERMW); }
                else wrap(t, BODY + TERMW, BODY + TERMW);
            } else {
                inline_(para, t, sizeof t);
                ospaces(BODY);
                obyte('-');
                obyte(' ');
                wrap(t, BODY + 2, BODY + 2);
            }
            emit();
            continue;
        }
        /* a paragraph: lines joined until a blank, an indented line or a mark */
        plen = 0;
        para[0] = 0;
        while (i < src->n) {
            char *l = src->l[i];
            int k = indentof(l);
            if (k < 0 || k >= 4 || (l[0] == '#') || (l[0] == '-' && l[1] == ' ')) break;
            if (plen && plen < (int)sizeof para - 1) para[plen++] = ' ';
            if (plen + strlen(l + k) >= sizeof para) break;
            strcpy(para + plen, l + k);
            plen += strlen(l + k);
            i++;
        }
        inline_(para, t, sizeof t);
        olen = ocol = 0;
        wrap(t, BODY, BODY);
        emit();
    }
}

/* a word in capitals alone on its line: a heading (NAME, FORMAT, SEE ALSO) */
static int capsheading(const char *s)
{
    int any = 0, ind = indentof(s);
    if (ind < 0 || ind > 3) return 0;
    for (s += ind; *s; s++) {
        if (*s >= 'a' && *s <= 'z') return 0;
        if (*s >= 'A' && *s <= 'Z') any = 1;
        else if (*s != ' ' && *s != '/' && *s != '-') return 0;
    }
    return any;
}

/* a long line cut into pieces of W, the rest indented like it */
static void plainline(const char *s)
{
    int ind = indentof(s), n = strlen(s);
    if (ind < 0) { addline(&out, "", 0); return; }
    while (n > W && W > 10) {
        int cut = W;
        while (cut > ind + 10 && s[cut] != ' ') cut--;
        if (cut <= ind + 10) cut = W;
        addline(&out, s, cut);
        s += cut;
        while (*s == ' ') s++;
        n = strlen(s);
        olen = ocol = 0;
        ospaces(ind);
        { const char *c = s; while (*c) obyte(*c++); }
        if (ind + n <= W) { emit(); return; }
        olen = ocol = 0;
    }
    addline(&out, s, n);
}

static void render_plain(Lines *src)
{
    int i;
    for (i = 0; i < src->n; i++) {
        if (capsheading(src->l[i])) {
            olen = ocol = 0;
            obyte(S_HEAD);
            { char *c = src->l[i] + indentof(src->l[i]); while (*c) obyte(*c++); }
            obyte(S_OFF);
            emit();
        } else plainline(src->l[i]);
    }
}

/* AmigaGuide: @ commands out, @{b} @{u} @{i} and links as styles */
static void render_guide(Lines *src)
{
    int i;
    char t[2048];
    for (i = 0; i < src->n; i++) {
        char *s = src->l[i];
        int k = 0;
        if (s[0] == '@' && s[1] != '{') {
            if (!strncmp(s, "@node", 5) || !strncmp(s, "@NODE", 5)) {
                char *q = strchr(s, '"'), *e;
                if (out.n && out.l[out.n - 1][0]) addline(&out, "", 0);
                if (q && (e = strchr(q + 1, '"'))) {
                    olen = ocol = 0;
                    obyte(S_HEAD);
                    while (++q < e) obyte(*q);
                    obyte(S_OFF);
                    emit();
                }
            }
            continue;
        }
        while (*s && k < (int)sizeof t - 4) {
            if (s[0] == '\\' && (s[1] == '@' || s[1] == '\\')) { t[k++] = s[1]; s += 2; continue; }
            if (s[0] == '@' && s[1] == '{') {
                char *e = strchr(s, '}');
                if (!e) { t[k++] = *s++; continue; }
                if (s[2] == '"') {                         /* a link: its label */
                    char *q = strchr(s + 3, '"');
                    if (q && q < e) {
                        t[k++] = S_UNDER;
                        for (s += 3; s < q && k < (int)sizeof t - 2; s++) t[k++] = *s;
                        t[k++] = S_OFF;
                    }
                } else if (!strncmp(s + 2, "b}", 2) || !strncmp(s + 2, "B}", 2)) t[k++] = S_BOLD;
                else if (!strncmp(s + 2, "u}", 2) || !strncmp(s + 2, "i}", 2)) t[k++] = S_UNDER;
                else if (!strncmp(s + 2, "ub}", 3) || !strncmp(s + 2, "uu}", 3) || !strncmp(s + 2, "ui}", 3) ||
                         !strncmp(s + 2, "plain}", 6)) t[k++] = S_OFF;
                s = e + 1;
                continue;
            }
            t[k++] = *s++;
        }
        t[k] = 0;
        plainline(t);
    }
}

static Lines src;
static int kind;                        /* 0 .man, 1 plain, 2 AmigaGuide */

static void render(int width)
{
    freelines(&out);
    W = width > 20 ? width : 20;
    if (kind == 0) render_man(&src);
    else if (kind == 2) render_guide(&src);
    else render_plain(&src);
    while (out.n && !out.l[out.n - 1][0]) cu_free(out.l[--out.n]);
}

static int load(const char *path)
{
    const char *dot = strrchr(path, '.');
    freelines(&src);
    if (!readlines(path, &src)) return 0;
    if (dot && !stricmp(dot, ".man")) kind = 0;
    else if (src.n && !strnicmp(src.l[0], "@database", 9)) kind = 2;
    else kind = 1;
    return 1;
}

/* ---- out, plain or with escape codes ------------------------------- */

static void printout(int ansi)
{
    int i;
    for (i = 0; i < out.n && !cu_break(); i++) {
        char *s = out.l[i];
        for (; *s; s++) {
            if (!ISMARK(*s)) { cu_putc(*s); continue; }
            if (!ansi) continue;
            if (*s == S_BOLD) cu_puts("\x1b[1m");
            else if (*s == S_UNDER) cu_puts("\x1b[4m");
            else if (*s == S_HEAD) cu_puts("\x1b[1;33m");
            else cu_puts("\x1b[0m");
        }
        cu_putc('\n');
    }
}

/* ---- the viewer -------------------------------------------------- */

static int top;
static char pat[80];
static char title[60];

static int lc(int c) { c &= 0xff; return (c >= 'A' && c <= 'Z') || (c >= 0xc0 && c <= 0xde && c != 0xd7) ? c + 32 : c; }

/* the visible text of line i, marks out (pos[] maps back) */
static int plaintext(int i, char *t, int size)
{
    char *s = out.l[i];
    int k = 0;
    for (; *s && k < size - 1; s++) if (!ISMARK(*s)) t[k++] = *s;
    t[k] = 0;
    return k;
}

static int hasmatch(int i)
{
    char t[1024];
    int n = plaintext(i, t, sizeof t), m = strlen(pat), j, q;
    if (!m) return 0;
    for (j = 0; j + m <= n; j++) {
        for (q = 0; q < m && lc(t[j + q]) == lc(pat[q]); q++) ;
        if (q == m) return 1;
    }
    return 0;
}

static void drawline(int row, int i)
{
    char *s = out.l[i], t[1024];
    int col = 0, fg = TC_DEFAULT, at = 0, m = strlen(pat), n, j;
    unsigned char mark[1024];
    n = plaintext(i, t, sizeof t);
    memset(mark, 0, n);
    if (m) for (j = 0; j + m <= n; j++) {
        int q;
        for (q = 0; q < m && lc(t[j + q]) == lc(pat[q]); q++) ;
        if (q == m) { memset(mark + j, 1, m); j += m - 1; }
    }
    for (j = 0; *s && col < tui_cols; s++) {
        if (*s == S_BOLD) { at = TA_BOLD; fg = TC_DEFAULT; continue; }
        if (*s == S_UNDER) { at = TA_UNDER; fg = TC_DEFAULT; continue; }
        if (*s == S_HEAD) { at = TA_BOLD; fg = 3; continue; }
        if (*s == S_OFF) { at = 0; fg = TC_DEFAULT; continue; }
        tui_putc(col++, row, (unsigned char)*s, fg, TC_DEFAULT, at | (mark[j] ? TA_REV : 0));
        j++;
    }
}

static void draw(const char *msg)
{
    int r, rows = tui_rows - 1;
    char st[200];
    int pct;
    tui_clear();
    for (r = 0; r < rows && top + r < out.n; r++) drawline(r, top + r);
    pct = out.n <= rows ? 100 : (int)((long)(top + rows) * 100 / out.n);
    if (pct > 100) pct = 100;
    if (msg) strcpy(st, msg);
    else {
        char num[8];
        int k = 0, v = pct;
        do num[k++] = '0' + v % 10; while ((v /= 10));
        strcpy(st, " ");
        strcat(st, title);
        strcat(st, "  ");
        { int a = strlen(st); while (k) st[a++] = num[--k]; st[a] = 0; }
        strcat(st, "%   q quit  / search  h help ");
    }
    tui_fill(0, rows, tui_cols, ' ', TC_DEFAULT, TC_DEFAULT, TA_REV);
    tui_put(0, rows, st, strlen(st) < (unsigned)tui_cols ? (int)strlen(st) : tui_cols, TC_DEFAULT, TC_DEFAULT, TA_REV);
    tui_cursor(0, rows, 0);
    tui_flush();
}

static void clamp(void)
{
    int rows = tui_rows - 1;
    if (top > out.n - rows) top = out.n - rows;
    if (top < 0) top = 0;
}

/* the next (dir 1) or earlier (-1) line with a match, from top */
static int seek(int dir)
{
    int i;
    for (i = top + dir; i >= 0 && i < out.n; i += dir) if (hasmatch(i)) { top = i; clamp(); return 1; }
    return 0;
}

/* a line typed on the status row */
static int ask(const char *prompt, char *buf, int size)
{
    TuiEv e;
    int n = 0, rows = tui_rows - 1;
    buf[0] = 0;
    for (;;) {
        char st[200];
        strcpy(st, prompt);
        strcat(st, buf);
        tui_fill(0, rows, tui_cols, ' ', TC_DEFAULT, TC_DEFAULT, 0);
        tui_put(0, rows, st, (int)strlen(st) < tui_cols ? (int)strlen(st) : tui_cols, TC_DEFAULT, TC_DEFAULT, 0);
        tui_cursor(strlen(st) < (unsigned)tui_cols ? strlen(st) : tui_cols - 1, rows, 1);
        tui_flush();
        if (!tui_wait(&e, -1, 0, NULL)) continue;
        if (e.type == TE_RESIZE) { tui_resized(); render(tui_cols - 1); clamp(); draw(NULL); continue; }
        if (e.type != TE_KEY) continue;
        if (e.key == K_ENTER) return n > 0;
        if (e.key == K_ESC || e.key == (TM_CTRL | 'c')) return 0;
        if (e.key == K_BS) { if (n) buf[--n] = 0; continue; }
        if (e.key >= 32 && e.key < 256 && n < size - 1) { buf[n++] = e.key; buf[n] = 0; }
    }
}

static void viewer(void)
{
    TuiEv e;
    int rows;
    const char *msg = NULL;
    render(tui_cols - 1);
    top = 0;
    tui_title(title);
    draw(NULL);
    for (;;) {
        if (!tui_wait(&e, -1, 0, NULL)) continue;
        rows = tui_rows - 1;
        msg = NULL;
        if (e.type == TE_RESIZE) {
            /* keep the same text at the top: by its share of the whole */
            long share = out.n ? (long)top * 1000 / out.n : 0;
            tui_resized();
            render(tui_cols - 1);
            top = (int)(share * out.n / 1000);
        } else if (e.type == TE_MOUSE) {
            if (e.mact == TB_WHEELUP) top -= 3;
            else if (e.mact == TB_WHEELDOWN) top += 3;
        } else if (e.type == TE_KEY) {
            int k = e.key;
            if (k == 'q' || k == 'Q' || k == K_ESC || k == (TM_CTRL | 'c')) break;
            else if (k == K_DOWN || k == 'j' || k == K_ENTER || k == 'e') top++;
            else if (k == K_UP || k == 'k' || k == 'y') top--;
            else if (k == ' ' || k == K_PGDN || k == 'f' || k == (TM_CTRL | 'f') || k == (TM_SHIFT | K_DOWN)) top += rows - 1;
            else if (k == 'b' || k == K_PGUP || k == (TM_CTRL | 'b') || k == (TM_SHIFT | K_UP)) top -= rows - 1;
            else if (k == 'd' || k == (TM_CTRL | 'd')) top += rows / 2;
            else if (k == 'u' || k == (TM_CTRL | 'u')) top -= rows / 2;
            else if (k == 'g' || k == '<' || k == K_HOME) top = 0;
            else if (k == 'G' || k == '>' || k == K_END) top = out.n;
            else if (k == '/' || k == '?') {
                char b[80];
                if (ask(k == '/' ? "/" : "?", b, sizeof b)) {
                    strcpy(pat, b);
                    if (!seek(k == '/' ? 1 : -1) && !hasmatch(top)) msg = " not found ";
                }
            } else if (k == 'n') { if (!seek(1)) msg = " no more "; }
            else if (k == 'N') { if (!seek(-1)) msg = " no more "; }
            else if (k == 'h' || k == 'H' || k == K_HELP) msg = " j/k line  space/b page  g/G start/end  / search  n/N next  q quit ";
            else continue;
        } else continue;
        clamp();
        draw(msg);
    }
}

/* ---- showing a page ------------------------------------------------ */

static int interactive;

static int showpage(const char *path)
{
    char pager[100];
    const char *base = FilePart((STRPTR)path);
    int k;
    if (o_where) { cu_puts(path); cu_putc('\n'); return 1; }
    if (!load(path)) return 0;
    for (k = 0; base[k] && base[k] != '.' && k < (int)sizeof title - 1; k++) title[k] = base[k];
    title[k] = 0;
    if (!interactive) {
        render(78);
        printout(0);
        cu_flush();
        return 1;
    }
    if (GetVar("PAGER", pager, sizeof pager, 0) > 0 && pager[0]) {
        /* to the viewer named, in colour, through a T: file */
        char tmp[60], cmd[300];
        BPTR fh, keep = cu_out;
        struct Process *me = (struct Process *)FindTask(NULL);
        strcpy(tmp, "T:man-");
        { ULONG v = (ULONG)me; int d = strlen(tmp); do tmp[d++] = "0123456789abcdef"[v & 15]; while ((v >>= 4)); tmp[d] = 0; }
        if ((fh = Open(tmp, MODE_NEWFILE))) {
            cu_flush();
            cu_out = fh;
            render(78);
            printout(1);
            cu_flush();
            cu_out = keep;
            Close(fh);
            if (strlen(pager) + strlen(tmp) + 3 < sizeof cmd) {
                strcpy(cmd, pager); strcat(cmd, " "); strcat(cmd, tmp);
                SystemTags(cmd, SYS_Input, 0, SYS_Output, 0, SYS_UserShell, TRUE, TAG_END);
            }
            DeleteFile(tmp);
            return 1;
        }
    }
    if (tui_open()) {
        viewer();
        tui_close();
    } else {
        render(78);
        printout(1);
    }
    cu_flush();
    return 1;
}

/* ---- man -k ----------------------------------------------------- */

typedef struct { char *name, *sum; } Hit;
static Hit *hits;
static int nhits, maxhits;
static const char *kword;

static int contains(const char *s, const char *w)
{
    int m = strlen(w), j, q, n = strlen(s);
    for (j = 0; j + m <= n; j++) {
        for (q = 0; q < m && lc(s[j + q]) == lc(w[q]); q++) ;
        if (q == m) return 1;
    }
    return 0;
}

/* the name and one-line summary of a page */
static int summary(const char *path, char *name, int nsize, char *sum, int ssize)
{
    CuIn in;
    char *line;
    LONG n;
    int nl, k = 0, after = 0;
    const char *base = FilePart((STRPTR)path);
    int i;
    for (i = 0; base[i] && base[i] != '.' && i < nsize - 1; i++) name[i] = base[i];
    name[i] = 0;
    sum[0] = 0;
    cu_silent = 1;
    if (!cu_open(&in, path)) { cu_silent = 0; return 0; }
    cu_silent = 0;
    while (k++ < 30 && (n = cu_line(&in, &line, &nl)) >= 0) {
        char *dash;
        if (line[0] == '#' && line[1] == ' ' && (dash = strstr(line, " - "))) {
            strncpy(sum, dash + 3, ssize - 1); sum[ssize - 1] = 0;
            break;
        }
        if (after && (dash = strstr(line, " - "))) {    /* NAME / SORT - Alphabetically ... */
            strncpy(sum, dash + 3, ssize - 1); sum[ssize - 1] = 0;
            break;
        }
        { int j = indentof(line); if (j >= 0 && !strncmp(line + j, "NAME", 4) && !line[j + 4 + strspn(line + j + 4, " ")]) after = 1; }
    }
    cu_close(&in);
    return 1;
}

static int kfile(const char *path)
{
    char name[40], sum[120];
    int i;
    if (!summary(path, name, sizeof name, sum, sizeof sum)) return 0;
    if (!contains(name, kword) && !contains(sum, kword)) return 0;
    for (i = 0; i < nhits; i++) if (!stricmp(hits[i].name, name)) return 0;   /* the first one wins */
    if (nhits == maxhits) {
        Hit *t = cu_alloc((maxhits = maxhits ? maxhits * 2 : 64) * sizeof *t);
        if (!t) return 1;
        if (hits) { memcpy(t, hits, nhits * sizeof *t); cu_free(hits); }
        hits = t;
    }
    hits[nhits].name = cu_strdup(name);
    hits[nhits].sum = cu_strdup(sum);
    nhits++;
    return 0;
}

static const char *kext;

static int kdrawer(const char *dir, void *u)
{
    BPTR l;
    struct FileInfoBlock *fib;
    (void)u;
    if (!(l = Lock((STRPTR)dir, SHARED_LOCK))) return 0;
    if ((fib = AllocDosObject(DOS_FIB, NULL))) {
        if (Examine(l, fib)) while (ExNext(l, fib)) {
            char p[CU_PATH];
            int n = strlen(fib->fib_FileName), e = strlen(kext);
            if (fib->fib_DirEntryType > 0 || n <= e || stricmp(fib->fib_FileName + n - e, kext)) continue;
            if (cu_join(p, dir, fib->fib_FileName)) kfile(p);
            if (cu_break()) break;
        }
        FreeDosObject(DOS_FIB, fib);
    }
    UnLock(l);
    return cu_rc >= 20;
}

typedef struct { const char *lang, *mid; } KL;

static int klang(const char *dir, void *u)
{
    KL *k = u;
    char p[CU_PATH];
    if (!cu_join(p, dir, k->lang)) return 0;
    if (*k->mid && !cu_join(p, p, k->mid)) return 0;
    return eachsub(p, kdrawer, NULL);
}

static int cmphit(const void *a, const void *b) { return stricmp(((Hit *)a)->name, ((Hit *)b)->name); }

static void apropos(const char *word)
{
    KL k;
    int pass, i;
    kword = word;
    for (pass = 0; pass < 2; pass++) {
        k.lang = pass ? "english" : lang;
        if (pass && !strcmp(lang, "english")) break;
        kext = ".man"; k.mid = "";
        eachdir("MAN:", klang, &k);
        kext = ".help"; k.mid = "Sys";
        eachdir("HELP:", klang, &k);
    }
    if (nhits > 1) qsort(hits, nhits, sizeof *hits, cmphit);
    for (i = 0; i < nhits; i++) {
        int n = strlen(hits[i].name);
        cu_puts(hits[i].name);
        cu_put("                ", n < 14 ? 14 - n : 1);
        cu_puts("- ");
        cu_puts(hits[i].sum);
        cu_putc('\n');
    }
    if (!nhits) cu_err(5, word, "nothing appropriate");
}

/* ---- main ---------------------------------------------------------- */

static void getlang(void)
{
    struct Locale *loc;
    if (!(LocaleBase = (struct LocaleBase *)OpenLibrary("locale.library", 38))) return;
    if ((loc = OpenLocale(NULL))) {
        const char *s = (const char *)loc->loc_LanguageName;
        int k;
        if (s) {
            for (k = 0; s[k] && s[k] != '.' && k < (int)sizeof lang - 1; k++) lang[k] = s[k];
            lang[k] = 0;
        }
        CloseLocale(loc);
    }
    CloseLibrary((struct Library *)LocaleBase);
    LocaleBase = NULL;
    if (!lang[0]) strcpy(lang, "english");
}

int main(void)
{
    char *v, *kw = NULL, *file = NULL, path[CU_PATH];
    int c, i, count;
    (void)version;
    if (cu_args("man [-a] [-w] [-L LANG] NAME ... | man -k WORD | man -l FILE") < 0) return cu_rc;
    getlang();
    while ((c = cu_opt("awL:k:l:", &v)) >= 0) switch (c) {
        case 'a': o_all = 1; break;
        case 'w': o_where = 1; break;
        case 'L': strncpy(lang, v, sizeof lang - 1); break;
        case 'k': kw = v; break;
        case 'l': file = v; break;
        default: return cu_rc;
    }
    interactive = IsInteractive(Output()) && IsInteractive(Input());
    if (kw) { apropos(kw); cu_flush(); return cu_rc; }
    if (file) { showpage(file); cu_flush(); return cu_rc; }
    if (!cu_argc) { cu_err(10, NULL, "which page? (man ? for help)"); return cu_rc; }
    for (i = 0; i < cu_argc && cu_rc < 20; i++) {
        if (findpage(cu_argv[i], path, o_all ? showpage : NULL, &count)) {
            if (!o_all) showpage(path);
        } else cu_err(5, cu_argv[i], "no page (man -k word searches)");
    }
    cu_flush();
    freelines(&src);
    freelines(&out);
    return cu_rc;
}
