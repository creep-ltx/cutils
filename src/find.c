/* find - walk trees and do things with what matches, as GNU find.
 *
 *   find [path | pattern ...] [expression]
 *
 * Each path is walked down to the bottom, each drawer's entries in natural
 * order (file9 before file10), and the expression is asked about every
 * entry; with no action in it, those for which it is true are printed. No
 * path: the current drawer; then the drawer itself is left out and the
 * names are relative ("x", "sub/x"). A path is printed as given, the names
 * under it joined with / (after : for a volume: "RAM:x"). Soft links (and
 * hard links to drawers) are never gone into. The walk keeps its list on
 * the heap, so the Shell's 4 KB stack is no limit to the depth.
 *
 * Tests:
 *   -name PAT   the name matches an Amiga pattern; * works as #?. Always
 *               case blind (as the Amiga's names are)   -iname PAT  the same
 *   -type T     f a file, d a drawer, l a link (soft, or a hard link);
 *               several at once: -type fl or -type f,l
 *   -size [+-]N[c|b|k|M|G]  the size in those units, rounded up (as GNU):
 *               plain N and c are bytes, b 512-byte blocks, k M G 1024s.
 *               +N more than N, -N less than N. Rounded up means -size -1k
 *               is only the empty files: say -size -1024 for "under 1 KB"
 *   -mtime [+-]N  changed N days ago (in whole 24 hours: 0 is the last day)
 *   -mmin [+-]N   changed N minutes ago
 *   -newer FILE changed after FILE was   -empty  an empty file or drawer
 *   -true -false
 * Actions:
 *   -print      the path and a new line (the default)   -print0  and a 0 byte
 *   -prune      do not go into this drawer (true)
 *   -delete     delete it; true when it went. Turns on -depth, so a
 *               drawer's contents go before it. A volume or assign root
 *               ("Work:") and the current drawer are never deleted; a
 *               drawer with something left in it fails as Delete would
 *   -exec CMD ARGS ;   run the command with every {} as the path; true when
 *               it gives return code 0. The Shell takes an unquoted ; for
 *               a comment: write ";" or \; or simply end the line there.
 *               -exec CMD {} "+"  runs it with many paths at once (true);
 *               quote the +: at the end of a line the Shell takes it for
 *               "more lines follow", and > < anywhere for its own: quote a
 *               ">NIL:" meant for the command. Words with spaces are
 *               quoted for it; its input is the console, else NIL:.
 * Operators, strongest first: ( EXPR ), ! EXPR or -not EXPR,
 *   EXPR EXPR or EXPR -a EXPR (-and), EXPR -o EXPR (-or).
 * Options (anywhere, for the whole walk): -maxdepth N (0 is only the paths
 *   themselves), -mindepth N (1 leaves out the paths), -depth (drawers
 *   after their contents).
 *
 * Return code 0 (also when nothing matched), 5 when something could not be
 * read or deleted, 10 a bad expression, 20 Ctrl-C. */
#include <dos/dostags.h>
#include <utility/tagitem.h>
#include "cu.h"

const char *cu_name = "find";
static const char version[] = "$VER: find 0.1b1 (14.10.2026)";

enum { N_AND, N_OR, N_NOT, N_TRUE, N_FALSE, N_NAME, N_TYPE, N_SIZE, N_NEWER, N_MTIME,
       N_MMIN, N_EMPTY, N_PRINT, N_PRINT0, N_PRUNE, N_DELETE, N_EXEC };

typedef struct Expr {
    int op;
    struct Expr *a, *b;
    char *pat;                   /* -name: parsed */
    int types;                   /* -type: 1 f, 2 d, 4 l */
    int cmp;                     /* -1 less, 0 equal, 1 more */
    LONG n, unit;
    struct DateStamp date;       /* -newer */
    char **argv;                 /* -exec */
    int argc, plus;
    char *batch;                 /* -exec +: the paths gathered */
    int blen;
} Expr;

static char **tok;
static int ntok, pos, bad, actions;
static int o_depth, maxdepth = -1, mindepth;
static int pruned;
static struct DateStamp now;
static Expr *execs[16];
static int nexecs;

static Expr *mk(int op, Expr *a, Expr *b)
{
    Expr *n = cu_alloc(sizeof *n);
    if (!n) { cu_err(20, NULL, "out of memory"); bad = 1; return NULL; }
    memset(n, 0, sizeof *n);
    n->op = op; n->a = a; n->b = b;
    return n;
}

static int is(const char *t, const char *w) { return t && !strcmp(t, w); }
static char *peek(void) { return pos < ntok ? tok[pos] : NULL; }

static char *value(const char *what)
{
    if (pos >= ntok) { cu_err(10, what, "needs a value"); bad = 1; return NULL; }
    return tok[pos++];
}

/* [+-]N, the number; with units letters after it (or none) */
static int number(Expr *n, const char *what, const char *units)
{
    char *v = value(what), *p;
    LONG x = 0;
    int any = 0;
    if (!v) return 0;
    p = v;
    n->cmp = 0;
    if (*p == '+') n->cmp = 1, p++;
    else if (*p == '-') n->cmp = -1, p++;
    while (*p >= '0' && *p <= '9') x = x * 10 + (*p++ - '0'), any = 1;
    n->n = x;
    n->unit = 1;
    if (*p && units && strchr(units, *p)) {
        switch (*p++) {
            case 'b': n->unit = 512; break;
            case 'k': n->unit = 1024; break;
            case 'M': n->unit = 1024L * 1024; break;
            case 'G': n->unit = 1024L * 1024 * 1024; break;
        }
    }
    if (!any || *p) { cu_err(10, v, "not a number"); bad = 1; return 0; }
    return 1;
}

static Expr *parse_or(void);

static Expr *primary(void)
{
    char *t = tok[pos++], *v;
    Expr *n;
    if (is(t, "-name") || is(t, "-iname")) {
        if (!(v = value(t)) || !(n = mk(N_NAME, NULL, NULL))) return NULL;
        if (!(n->pat = cu_alloc(620)) || !cu_pattern(v, n->pat, 620)) {
            cu_err(10, v, "not a pattern"); bad = 1; return NULL;
        }
        return n;
    }
    if (is(t, "-type")) {
        if (!(v = value(t)) || !(n = mk(N_TYPE, NULL, NULL))) return NULL;
        for (; *v; v++) {
            if (*v == 'f') n->types |= 1;
            else if (*v == 'd') n->types |= 2;
            else if (*v == 'l') n->types |= 4;
            else if (*v != ',') { cu_err(10, t, "is f, d or l"); bad = 1; return NULL; }
        }
        return n;
    }
    if (is(t, "-size")) {
        if (!(n = mk(N_SIZE, NULL, NULL)) || !number(n, t, "cbkMG")) return NULL;
        return n;
    }
    if (is(t, "-mtime") || is(t, "-mmin")) {
        if (!(n = mk(t[2] == 't' ? N_MTIME : N_MMIN, NULL, NULL)) || !number(n, t, NULL)) return NULL;
        return n;
    }
    if (is(t, "-maxdepth") || is(t, "-mindepth")) {
        Expr x;
        if (!number(&x, t, NULL)) return NULL;
        if (x.cmp) { cu_err(10, t, "needs a plain number"); bad = 1; return NULL; }
        if (t[2] == 'a') maxdepth = x.n; else mindepth = x.n;
        return mk(N_TRUE, NULL, NULL);
    }
    if (is(t, "-newer")) {
        BPTR l;
        struct FileInfoBlock *fib;
        if (!(v = value(t)) || !(n = mk(N_NEWER, NULL, NULL))) return NULL;
        if (!(l = Lock(v, SHARED_LOCK))) { cu_fault(10, v); bad = 1; return NULL; }
        if ((fib = AllocDosObject(DOS_FIB, NULL))) {
            if (Examine(l, fib)) n->date = fib->fib_Date;
            FreeDosObject(DOS_FIB, fib);
        }
        UnLock(l);
        return n;
    }
    if (is(t, "-exec")) {
        int start = pos;
        if (!(n = mk(N_EXEC, NULL, NULL))) return NULL;
        while (pos < ntok && !is(tok[pos], ";") && !is(tok[pos], "\\;")
               && !(is(tok[pos], "+") && pos > start && is(tok[pos - 1], "{}"))) pos++;
        n->argv = tok + start;
        n->argc = pos - start;
        if (pos < ntok) n->plus = is(tok[pos++], "+");
        if (!n->argc) { cu_err(10, t, "needs a command"); bad = 1; return NULL; }
        if (n->plus) {
            n->argc--;                       /* the {} is where the paths go */
            if (nexecs == 16 || !(n->batch = cu_alloc(CU_PATH * 2))) { cu_err(10, t, "too many"); bad = 1; return NULL; }
            execs[nexecs++] = n;
        }
        actions = 1;
        return n;
    }
    if (is(t, "-print")) { actions = 1; return mk(N_PRINT, NULL, NULL); }
    if (is(t, "-print0")) { actions = 1; return mk(N_PRINT0, NULL, NULL); }
    if (is(t, "-delete")) { actions = 1; o_depth = 1; return mk(N_DELETE, NULL, NULL); }
    if (is(t, "-prune")) return mk(N_PRUNE, NULL, NULL);
    if (is(t, "-empty")) return mk(N_EMPTY, NULL, NULL);
    if (is(t, "-true")) return mk(N_TRUE, NULL, NULL);
    if (is(t, "-false")) return mk(N_FALSE, NULL, NULL);
    if (is(t, "-depth")) { o_depth = 1; return mk(N_TRUE, NULL, NULL); }
    cu_err(10, t, t[0] == '-' ? "unknown test" : "paths go before the tests");
    bad = 1;
    return NULL;
}

static Expr *unary(void)
{
    char *t = peek();
    Expr *n;
    if (bad) return NULL;
    if (cu_stackleft() < 1000) { cu_err(10, NULL, "expression too deep"); bad = 1; return NULL; }
    if (!t || is(t, ")") || is(t, "-o") || is(t, "-or") || is(t, "-a") || is(t, "-and")) {
        cu_err(10, t ? t : (pos ? tok[pos - 1] : NULL), t ? "not expected here" : "something should follow");
        bad = 1;
        return NULL;
    }
    if (is(t, "!") || is(t, "-not")) {
        pos++;
        return (n = unary()) ? mk(N_NOT, n, NULL) : NULL;
    }
    if (is(t, "(")) {
        pos++;
        if (!(n = parse_or())) return NULL;
        if (!is(peek(), ")")) { cu_err(10, "(", "no ) to end it"); bad = 1; return NULL; }
        pos++;
        return n;
    }
    return primary();
}

static Expr *parse_and(void)
{
    Expr *a = unary(), *b;
    while (a && !bad && pos < ntok && !is(tok[pos], ")") && !is(tok[pos], "-o") && !is(tok[pos], "-or")) {
        if (is(tok[pos], "-a") || is(tok[pos], "-and")) pos++;
        if (!(b = unary())) return NULL;
        a = mk(N_AND, a, b);
    }
    return bad ? NULL : a;
}

static Expr *parse_or(void)
{
    Expr *a = parse_and(), *b;
    while (a && !bad && (is(peek(), "-o") || is(peek(), "-or"))) {
        pos++;
        if (!(b = parse_and())) return NULL;
        a = mk(N_OR, a, b);
    }
    return bad ? NULL : a;
}

/* ---- running a command ---- */

/* w, quoted for the Shell when it has to be ("a b" -> "a b", * as **) */
static int addword(char *line, int len, const char *w, int max)
{
    int q = !*w || strpbrk(w, " \t;\"=") != NULL;
    if (len && len < max) line[len++] = ' ';
    if (q && len < max) line[len++] = '"';
    for (; *w && len < max - 2; w++) {
        if (q && (*w == '"' || *w == '*')) line[len++] = '*';
        line[len++] = *w;
    }
    if (q && len < max) line[len++] = '"';
    return len;
}

static APTR winptr;              /* the requesters' window, as find found it */

static int run(char *line)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    APTR mine = me->pr_WindowPtr;
    LONG rc;
    cu_flush();
    me->pr_WindowPtr = winptr;       /* the command's own requesters as usual */
    /* the input only when it is a console: from a script or a pipe the
     * command's Shell would read on in it and eat the lines after find */
    if (IsInteractive(Input()))
        rc = SystemTags((STRPTR)line, SYS_Input, Input(), SYS_Output, Output(), TAG_END);
    else {
        BPTR nil = Open("NIL:", MODE_OLDFILE);
        rc = SystemTags((STRPTR)line, SYS_Input, nil, SYS_Output, Output(), TAG_END);
        if (nil) Close(nil);
    }
    me->pr_WindowPtr = mine;
    if (rc == -1) cu_fault(5, line);
    return rc;
}

#define LINE 1024

static int execone(Expr *n, const char *path)
{
    static char line[LINE], w[CU_PATH * 2];
    int i, len = 0;
    for (i = 0; i < n->argc; i++) {
        const char *s = n->argv[i], *b;
        int k = 0;
        while ((b = strstr(s, "{}")) && k + (b - s) + strlen(path) < sizeof w) {
            memcpy(w + k, s, b - s); k += b - s;
            strcpy(w + k, path); k += strlen(path);
            s = b + 2;
        }
        if (k + strlen(s) >= sizeof w) { cu_err(5, path, "command too long"); return 0; }
        strcpy(w + k, s);
        len = addword(line, len, w, LINE - 2);
    }
    if (len >= LINE - 4) { cu_err(5, path, "command too long"); return 0; }
    line[len] = 0;
    return run(line) == 0;
}

/* -exec ... {} +: the paths gathered run with the command */
static void execbatch(Expr *n)
{
    static char line[LINE];
    int i, len = 0;
    if (!n->blen) return;
    for (i = 0; i < n->argc; i++) len = addword(line, len, n->argv[i], LINE - 2);
    if (len + n->blen + 2 >= LINE) { cu_err(5, n->argv[0], "command too long"); n->blen = 0; return; }
    line[len++] = ' ';
    memcpy(line + len, n->batch, n->blen);
    line[len + n->blen] = 0;
    n->blen = 0;
    run(line);
}

static void addbatch(Expr *n, const char *path)
{
    char w[CU_PATH + 8];
    int k = addword(w, 0, path, sizeof w - 1);
    if (n->blen + k + 1 > 400) execbatch(n);     /* a line the Shell takes */
    if (n->blen) n->batch[n->blen++] = ' ';
    memcpy(n->batch + n->blen, w, k);
    n->blen += k;
}

/* ---- the tests ---- */

static int isempty(CuEnt *e)
{
    BPTR l;
    struct FileInfoBlock *fib;
    int empty = 0;
    if (e->type < 0) return e->size == 0;
    if (!cu_isdir(e->type)) return 0;
    if (!(l = Lock((STRPTR)e->path, SHARED_LOCK))) return 0;
    if ((fib = AllocDosObject(DOS_FIB, NULL))) {
        if (Examine(l, fib) && !ExNext(l, fib)) empty = IoErr() == ERROR_NO_MORE_ENTRIES;
        FreeDosObject(DOS_FIB, fib);
    }
    UnLock(l);
    return empty;
}

static int compare(LONG v, Expr *n)
{
    return n->cmp > 0 ? v > n->n : n->cmp < 0 ? v < n->n : v == n->n;
}

static int eval(Expr *n, CuEnt *e)
{
    switch (n->op) {
    case N_AND: return eval(n->a, e) && eval(n->b, e);
    case N_OR: return eval(n->a, e) || eval(n->b, e);
    case N_NOT: return !eval(n->a, e);
    case N_TRUE: return 1;
    case N_FALSE: return 0;
    case N_NAME: return cu_match(n->pat, e->name);
    case N_TYPE: {
        int t = e->type == ST_SOFTLINK || e->type == ST_LINKDIR || e->type == ST_LINKFILE ? 4 : e->type > 0 ? 2 : 1;
        return (n->types & t) != 0;
    }
    case N_SIZE: {
        ULONG s = (ULONG)e->size, u = n->unit;
        return compare((LONG)(s / u + (s % u != 0)), n);
    }
    case N_NEWER: return CompareDates(&e->date, &n->date) < 0;
    case N_MTIME: case N_MMIN: {
        LONG min = (now.ds_Days - e->date.ds_Days) * 1440 + now.ds_Minute - e->date.ds_Minute;
        if (n->op == N_MTIME) min = min >= 0 ? min / 1440 : -1 - (-1 - min) / 1440;
        return compare(min, n);
    }
    case N_EMPTY: return isempty(e);
    case N_PRINT: cu_puts(e->path); cu_putc('\n'); return 1;
    case N_PRINT0: cu_puts(e->path); cu_putc(0); return 1;
    case N_PRUNE: pruned = 1; return 1;
    case N_DELETE: {
        int n = strlen(e->path);
        if (!n || e->path[n - 1] == ':') { cu_err(5, e->path, "a root, not deleted"); return 0; }
        if (!DeleteFile((STRPTR)e->path)) { cu_fault(5, e->path); return 0; }
        return 1;
    }
    case N_EXEC:
        if (n->plus) { addbatch(n, e->path); return 1; }
        return execone(n, e->path);
    }
    return 0;
}

static Expr *root;

static int visit(CuEnt *e, void *u)
{
    (void)u;
    if (o_depth && cu_isdir(e->type) && !e->post) return 0;   /* asked after its contents */
    if (e->depth < mindepth || (!e->depth && !*e->path)) return 0;
    pruned = 0;
    eval(root, e);
    if (cu_rc >= 20 || cu_break()) return CU_STOP;
    return pruned && !o_depth ? CU_SKIP : 0;
}

static int onepath(const char *path, void *u)
{
    (void)u;
    return cu_walk(path, o_depth ? CU_WALK_POST : 0, maxdepth, visit, NULL) == CU_STOP;
}

int main(void)
{
    int npaths = 0, i;
    struct Process *me = (struct Process *)FindTask(NULL);
    (void)version;
    winptr = me->pr_WindowPtr;
    if (cu_args("find [path | pattern ...] [-name PAT] [-type f|d|l] [-size [+-]N[ckM]] [-mtime [+-]N] [-newer FILE] [-empty] [-maxdepth N] [-mindepth N] [! -o -a ( )] [-print] [-prune] [-delete] [-exec CMD {} ;]") < 0) return cu_rc;
    while (npaths < cu_argc) {
        char *w = cu_argv[npaths];
        if ((w[0] == '-' && w[1]) || is(w, "!") || is(w, "(") || is(w, ")")) break;
        npaths++;
    }
    tok = cu_argv + npaths;
    ntok = cu_argc - npaths;
    if (ntok) {
        root = parse_or();
        if (!bad && pos < ntok) { cu_err(10, tok[pos], "not expected here"); bad = 1; }
        if (bad) return cu_rc > 10 ? cu_rc : 10;
    }
    if (!actions) root = root ? mk(N_AND, root, mk(N_PRINT, NULL, NULL)) : mk(N_PRINT, NULL, NULL);
    if (!root || cu_rc >= 20) return cu_rc;
    DateStamp(&now);
    me->pr_WindowPtr = (APTR)-1;                  /* "please insert" as a message, not a requester */
    if (!npaths) onepath("", NULL);
    for (i = 0; i < npaths && cu_rc < 20; i++) cu_each(cu_argv[i], onepath, NULL);
    for (i = 0; i < nexecs && cu_rc < 20; i++) execbatch(execs[i]);
    me->pr_WindowPtr = winptr;
    cu_flush();
    return cu_rc;
}
