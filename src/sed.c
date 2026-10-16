/* sed - the stream editor, GNU sed for everyday use.
 *
 *   sed [-nEsuz] [-i[SUFFIX]] [-l N] [-e SCRIPT]... [-f FILE]... [SCRIPT] [file ...]
 *
 * Reads the files (or the input) a line at a time, runs the script on each
 * line and prints the result. Without -e or -f the first word is the script.
 *   -n  no automatic printing (only p and friends print)   -E, -r  extended
 *   regular expressions   -i[SUFFIX]  edit the files in place (with SUFFIX:
 *   keep the old one as name+SUFFIX; a * in SUFFIX stands for the name)
 *   -s  each file on its own ($ and line numbers per file)   -u  output at
 *   once, line by line   -z  lines end with a 0 byte, not a newline
 *   -l N  where l wraps its lines (70)   -e SCRIPT  more script   -f FILE
 *   script from a file (- the input)   --expression= --file= --quiet
 *   --silent --in-place[=SUFFIX] --regexp-extended --separate --null-data
 *   --line-length=N --unbuffered --posix --debug --version
 *
 * Commands: s y d D p P n N g G h H x b t T : = l q Q a i c r R w W z F
 * { } and ! (v and #n as GNU). Addresses: N, $, /re/, \cREc, /re/I, /re/M,
 * first~step, a,b, a,+N, a,~N, 0,/re/. s flags: g p N i I m M w FILE; in
 * the replacement & \0-\9 \n \t \L \U \l \u \E. Escapes \t \n \a \f \v \r
 * \cX \dNNN \oNNN \xHH everywhere, as GNU's.
 * The regular expressions are basic by default (\( \) \{ \} \| \+ \?),
 * POSIX's leftmost-longest; \b \B \< \> \w \W \s \S \` \' work. Not done:
 * back-references inside a pattern (\1 in the search part: the matcher has
 * none, said so), the e command and the e flag (no shell to run).
 *
 * Return code 0 fine, q's or Q's exit code, 10 a bad script, a file that
 * could not be read or written (GNU's 1, 2 and 4: sed goes on with the next
 * file, as GNU does), 20 Ctrl-C or out of memory. */
#include <stdlib.h>
#include "cu.h"

const char *cu_name = "sed";
static const char version[] = "$VER: sed 0.1b2 (16.10.2026)";

/* ---- growing buffers ---- */

typedef struct { char *s; LONG n, size; int nl; } Buf;   /* nl: ended with a newline (GNU's "chomped") */

static void die(int rc);

static void bneed(Buf *b, LONG n)
{
    char *t;
    LONG ns;
    if (n + 1 <= b->size) return;
    ns = b->size ? b->size : 128;
    while (ns < n + 1) ns *= 2;
    if (!(t = cu_alloc(ns))) { cu_err(20, NULL, "out of memory"); die(20); }
    if (b->n) memcpy(t, b->s, b->n);
    cu_free(b->s);
    b->s = t; b->size = ns;
}
static void bset(Buf *b, const char *s, LONG n) { bneed(b, n); if (n) memmove(b->s, s, n); b->n = n; }
static void bcat(Buf *b, const char *s, LONG n) { bneed(b, b->n + n); if (n) memcpy(b->s + b->n, s, n); b->n += n; }
static void bputc(Buf *b, int c) { bneed(b, b->n + 1); b->s[b->n++] = c; }
static void bswap(Buf *a, Buf *b)
{
    char *s = a->s; LONG n = a->n, size = a->size; int nl = a->nl;
    a->s = b->s; a->n = b->n; a->size = b->size; a->nl = b->nl;
    b->s = s; b->n = n; b->size = size; b->nl = nl;
}

static int lower(int c) { c &= 0xff; return (c >= 'A' && c <= 'Z') || (c >= 0xc0 && c <= 0xde && c != 0xd7) ? c + 32 : c; }
static int upper(int c) { c &= 0xff; return (c >= 'a' && c <= 'z') || (c >= 0xe0 && c <= 0xfe && c != 0xf7) ? c - 32 : c; }

/* ---- options ---- */

static int o_quiet, o_ext, o_sep, o_unbuf, o_inplace, o_posix;
static char sep = '\n';            /* -z: 0 */
static char *o_suffix;             /* -i's backup suffix, NULL: none */
static LONG o_width = 70;          /* l */

/* ---- output: stdout, the -i file, w files ---- */

typedef struct Out {
    struct Out *next;
    char *name;
    BPTR fh;                       /* 0: cu_put (the standard output) */
    int own, missing;              /* missing: the last line written had no newline */
    char *buf;
    LONG len;
} Out;

static Out mainout, errout_, *outs;
static Out *cur = &mainout;        /* where the pattern space goes */

static void oflush(Out *o)
{
    if (!o->fh) { cu_flush(); return; }
    if (o->len > 0 && Write(o->fh, o->buf, o->len) != o->len) {
        o->len = 0;
        cu_fault(10, o->name ? o->name : "output");
        die(10);
    }
    o->len = 0;
}

static void owrite(Out *o, const char *s, LONG n)
{
    if (!o->fh) { cu_put(s, n); return; }
    while (n > 0) {
        LONG k = 4096 - o->len;
        if (k > n) k = n;
        memcpy(o->buf + o->len, s, k);
        o->len += k; s += k; n -= k;
        if (o->len == 4096) oflush(o);
    }
}

static void ofix(Out *o)
{
    if (o->missing) { owrite(o, &sep, 1); o->missing = 0; }
}

static void otext(Out *o, const char *s, LONG n) { ofix(o); owrite(o, s, n); }

static void oline(Out *o, const char *s, LONG n, int nl)
{
    ofix(o);
    owrite(o, s, n);
    if (nl) owrite(o, &sep, 1); else o->missing = 1;
}

static BPTR errfh(void)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    return me->pr_CES ? me->pr_CES : Output();
}

/* a w file: opened (emptied) once when the script is read */
static Out *wfile(const char *name)
{
    Out *o;
    if (!strcmp(name, "/dev/stdout")) return &mainout;
    if (!strcmp(name, "/dev/stderr")) {
        if (!errout_.fh) { errout_.fh = errfh(); errout_.buf = cu_alloc(4096); errout_.name = "/dev/stderr"; if (!errout_.buf) die(20); }
        return &errout_;
    }
    for (o = outs; o; o = o->next) if (!strcmp(o->name, name)) return o;
    if (!(o = cu_alloc(sizeof *o))) die(20);
    memset(o, 0, sizeof *o);
    if (!(o->name = cu_strdup(name)) || !(o->buf = cu_alloc(4096))) die(20);
    if (!(o->fh = Open((STRPTR)name, MODE_NEWFILE))) { cu_fault(10, name); die(10); }
    o->own = 1;
    o->next = outs; outs = o;
    return o;
}

/* ---- reading lines (any length, a newline or with -z a 0 at the end) ---- */

typedef struct {
    BPTR fh;
    int own, eof, err;
    char *buf;
    LONG pos, len;
} Src;

#define INBUF 16384

static int src_open(Src *s, const char *name)
{
    memset(s, 0, sizeof *s);
    if (!strcmp(name, "-") || !strcmp(name, "/dev/stdin")) s->fh = Input();
    else {
        if (!(s->fh = Open((STRPTR)name, MODE_OLDFILE))) return 0;
        s->own = 1;
    }
    if (!(s->buf = cu_alloc(INBUF))) { cu_err(20, NULL, "out of memory"); die(20); }
    return 1;
}

static void src_close(Src *s)
{
    if (s->own && s->fh) Close(s->fh);
    cu_free(s->buf);
    memset(s, 0, sizeof *s);
}

static int src_fill(Src *s)
{
    LONG n;
    if (s->eof) return 0;
    n = Read(s->fh, s->buf, INBUF);
    if (n <= 0) { s->eof = 1; if (n < 0) s->err = 1; s->pos = s->len = 0; return 0; }
    s->pos = 0; s->len = n;
    return 1;
}

/* the next line into b (its nl set): 0 at the end */
static int src_line(Src *s, Buf *b, int keep)
{
    int any = 0;
    if (!keep) b->n = 0;
    for (;;) {
        char *p, *e;
        LONG k;
        if (s->pos >= s->len && !src_fill(s)) { b->nl = 0; return any; }
        p = s->buf + s->pos;
        k = s->len - s->pos;
        any = 1;
        if ((e = memchr(p, sep, k))) {
            bcat(b, p, e - p);
            s->pos += e - p + 1;
            b->nl = 1;
            return 1;
        }
        bcat(b, p, k);
        s->pos = s->len;
    }
}

/* ---- the script ---- */

typedef struct { void *prog; int ngroup; } Re;

enum { A_NONE, A_LINE, A_LAST, A_RE, A_STEP, A_ZERO, A_PLUS, A_MULT };
typedef struct { int type; LONG n, step; Re *re; } Addr;    /* re NULL with A_RE: the last one used */

enum { R_INACTIVE, R_ACTIVE, R_CLOSED };

enum { P_LIT, P_GROUP, P_UPPER, P_LOWER, P_UONE, P_LONE, P_END };
typedef struct { int kind, group; LONG off, len; } Piece;

typedef struct RFile { struct RFile *next; char *name; Src src; int open, done; } RFile;

typedef struct {
    Addr a1, a2;
    int cmd, neg, range, range0;
    LONG rend;
    char *text;                    /* a i c: the text; r R: the file name */
    LONG tlen;
    Re *re;                        /* s: NULL = the last regex */
    Piece *rep;
    char *rtext;
    int nrep, gflag, pflag;
    LONG nth;
    Out *wout;
    LONG num;                      /* q Q: exit code; l: width (-1 the default); b t T {: where to go */
    char *label;
    unsigned char *ymap;
    RFile *rf;
} Cmd;

static Cmd *cmds;
static int ncmd, maxcmd;

/* the whole script, the -e and -f parts joined by newlines */
static char *S;
static LONG slen, smax;
typedef struct { LONG at; int e; char *file; } Part;    /* e: -e number, else the -f file */
static Part *parts;
static int nparts, nexpr;

static LONG pos;                   /* where the parser is in S */

static void numstr(char *to, LONG v)
{
    char t[12];
    int j = 0;
    do t[j++] = '0' + v % 10; while ((v /= 10));
    while (j) *to++ = t[--j];
    *to = 0;
}

static void bad(const char *why)
{
    char what[300];
    int k = 0;
    LONG i, p = pos;
    while (k + 1 < nparts && parts[k + 1].at <= p) k++;
    if (!nparts) strcpy(what, "-e expression #1");
    else if (parts[k].file) {
        LONG line = 1;
        for (i = parts[k].at; i < p && i < slen; i++) if (S[i] == '\n') line++;
        strcpy(what, "file ");
        strncat(what, parts[k].file, 200);
        strcat(what, " line ");
        numstr(what + strlen(what), line);
    } else {
        strcpy(what, "-e expression #");
        numstr(what + strlen(what), parts[k].e);
        strcat(what, ", char ");
        numstr(what + strlen(what), p - parts[k].at < 0 ? 0 : p - parts[k].at);
    }
    cu_err(10, what, why);
    die(10);
}

static void bad2(const char *a, int c, const char *b)
{
    char t[120];
    int n = strlen(a);
    strcpy(t, a);
    t[n] = c; t[n + 1] = 0;
    strcat(t, b);
    bad(t);
}

/* a part ends here (the newline joining two -e's): a/i/c one-line text stops */
static int partend(LONG at)
{
    int k;
    for (k = 1; k < nparts; k++) if (parts[k].at == at + 1) return 1;
    return 0;
}

static int peek(void) { return pos < slen ? (unsigned char)S[pos] : -1; }
static int next(void) { return pos < slen ? (unsigned char)S[pos++] : -1; }
static int blank(int c) { return c == ' ' || c == '\t' || c == '\r' || c == '\f' || c == '\v'; }
static void skipblank(void) { while (blank(peek())) pos++; }

static LONG number(void)
{
    LONG v = 0;
    while (peek() >= '0' && peek() <= '9') v = v * 10 + (next() - '0');
    return v;
}

/* GNU's escapes: \a \f \v \r \t \n \cX \dNNN \oNNN \xHH at s (after the \):
 * the byte, its length in *used; -1 when it is none of them */
static int escape(const char *s, LONG n, LONG *used)
{
    int v = 0, k = 0, base, max;
    if (n < 1) return -1;
    *used = 1;
    switch (s[0]) {
    case 'a': return 7;
    case 'f': return 12;
    case 'v': return 11;
    case 'r': return 13;
    case 't': return 9;
    case 'n': return 10;
    case 'c':
        if (n < 2) return -1;
        *used = 2;
        return upper((unsigned char)s[1]) ^ 0x40;
    case 'd': base = 10; max = 3; break;
    case 'o': base = 8; max = 3; break;
    case 'x': base = 16; max = 2; break;
    default: return -1;
    }
    while (k < max && k + 1 < n) {
        int c = (unsigned char)s[k + 1], d;
        if (c >= '0' && c <= '9') d = c - '0';
        else if ((c | 32) >= 'a' && (c | 32) <= 'f') d = (c | 32) - 'a' + 10;
        else break;
        if (d >= base) break;
        v = v * base + d;
        k++;
    }
    if (!k) return -1;
    *used = k + 1;
    return v & 0xff;
}

/* text up to the delimiter d: \d gives d, \n a newline (regex), \ newline a
 * newline; other \x kept as they are */
static char *delimited(int d, int regex, LONG *len, const char *what)
{
    Buf b;
    int c;
    memset(&b, 0, sizeof b);
    for (;;) {
        c = next();
        if (c < 0) bad(what);
        if (c == '\n' && regex) { pos--; bad(what); }
        if (c == d) break;
        if (c == '[' && regex) {           /* a bracket: the delimiter is plain inside (GNU) */
            LONG j = pos;
            if (j < slen && S[j] == '^') j++;
            if (j < slen && S[j] == ']') j++;
            while (j < slen && S[j] != ']' && S[j] != '\n') {
                if (S[j] == '[' && j + 1 < slen && (S[j + 1] == ':' || S[j + 1] == '.' || S[j + 1] == '=')) {
                    char k = S[j + 1];
                    LONG e = j + 2;
                    while (e + 1 < slen && S[e] != '\n' && !(S[e] == k && S[e + 1] == ']')) e++;
                    if (e + 1 < slen && S[e] == k) { j = e + 2; continue; }
                }
                j++;
            }
            if (j < slen && S[j] == ']') {
                bputc(&b, '[');
                bcat(&b, S + pos, j + 1 - pos);
                pos = j + 1;
                continue;
            }
        }
        if (c == '\\') {
            c = next();
            if (c < 0) bad(what);
            if (c == d) { if (!regex && c == '&') bputc(&b, '\\'); }
            else if (c == 'n' && regex) c = '\n';
            else if (c != '\n') bputc(&b, '\\');
        }
        bputc(&b, c);
    }
    bputc(&b, 0);
    *len = b.n - 1;
    return b.s;
}

/* a basic or extended regular expression made into lib/regex.c's own
 * (extended, \x a plain x): escapes done, BRE's specials swapped */
static int special(int c) { return c && strchr(".[]()|*+?{}^$\\", c) != NULL; }

static char *convert(const char *s, LONG n, int *groups)
{
    Buf o;
    LONG i = 0, used;
    int start = 1, ng = 0;
    memset(&o, 0, sizeof o);
    while (i < n) {
        int c = (unsigned char)s[i];
        if (c == '[') {                          /* a bracket, whole */
            LONG j = i + 1;
            if (j < n && s[j] == '^') j++;
            if (j < n && s[j] == ']') j++;
            while (j < n && s[j] != ']') {
                if (s[j] == '[' && j + 1 < n && (s[j + 1] == ':' || s[j + 1] == '.' || s[j + 1] == '=')) {
                    char k = s[j + 1];
                    LONG e = j + 2;
                    while (e + 1 < n && !(s[e] == k && s[e + 1] == ']')) e++;
                    if (e + 1 < n) { j = e + 2; continue; }
                }
                j++;
            }
            if (j >= n) bad("unterminated address regex");
            for (; i <= j; i++) {
                int e;
                if (s[i] == '\\' && i + 1 < j && (e = escape(s + i + 1, j - i - 1, &used)) > 0) { bputc(&o, e); i += used; continue; }
                bputc(&o, s[i]);
            }
            start = 0;
            continue;
        }
        if (c == '\\') {
            int d;
            if (i + 1 >= n) bad("trailing backslash (\\)");
            d = (unsigned char)s[i + 1];
            if (d >= '1' && d <= '9') bad("back-references in a pattern are not supported");
            if ((c = escape(s + i + 1, n - i - 1, &used)) >= 0) {
                if (!c) bad("a 0 byte in a pattern is not supported");
                bputc(&o, c);                    /* as GNU: the byte keeps any special meaning */
                i += 1 + used;
                start = 0;
                continue;
            }
            i += 2;
            if (!o_ext && strchr("(){}|+?", d)) {
                if (d == '{' && i < n && s[i] == ',') { bcat(&o, "{0", 2); start = 0; continue; }
                bputc(&o, d);
                if (d == '(') ng++;
                start = d == '(' || d == '|';
                continue;
            }
            if (strchr("wWsSbB<>`'", d)) { bputc(&o, '\\'); bputc(&o, d); start = 0; continue; }
            if (special(d) || d == '/') { bputc(&o, '\\'); bputc(&o, d); start = 0; continue; }
            bputc(&o, d);                        /* \y: y */
            start = 0;
            continue;
        }
        if (!o_ext) {
            if (strchr("(){}|+?", c)) { bputc(&o, '\\'); bputc(&o, c); start = 0; i++; continue; }
            if (c == '*' && start) { bcat(&o, "\\*", 2); start = 0; i++; continue; }
            if (c == '^') {
                if (start) { bputc(&o, '^'); i++; continue; }   /* still at the start: ^* is a plain * */
                bcat(&o, "\\^", 2); i++; continue;
            }
            if (c == '$') {
                if (i + 1 == n || (s[i + 1] == '\\' && i + 2 < n && (s[i + 2] == ')' || s[i + 2] == '|'))) bputc(&o, '$');
                else bcat(&o, "\\$", 2);
                i++; start = 0;
                continue;
            }
        } else {
            if (c == '(') ng++;
            if (c == '{' && i + 1 < n && s[i + 1] == ',') { bcat(&o, "{0", 2); i++; start = 0; continue; }
        }
        bputc(&o, c);
        i++;
        start = 0;
    }
    bputc(&o, 0);
    *groups = ng;
    return o.s;
}

static Re *compile(const char *s, LONG n, int icase, int multi)
{
    Re *re;
    int ng;
    char *e = convert(s, n, &ng);
    if (!(re = cu_alloc(sizeof *re))) die(20);
    re->ngroup = ng;
    if (!(re->prog = re_comp(e, (icase ? RE_ICASE : 0) | RE_LONGEST | (multi ? RE_MULTILINE : 0)))) bad("bad regular expression (or too big)");
    cu_free(e);
    return re;
}

static Re *lastre;

/* /re/ or \cREc after its first character, with I and M after it */
static void regaddr(Addr *a, int d)
{
    LONG n;
    char *s;
    int icase = 0, multi = 0;
    if (d < 0 || d == '\n' || d == '\\') bad("unterminated address regex");
    s = delimited(d, 1, &n, "unterminated address regex");
    for (;;) {
        if (peek() == 'I') { icase = 1; pos++; }
        else if (peek() == 'M') { multi = 1; pos++; }
        else break;
    }
    a->type = A_RE;
    if (!n) {
        if (icase || multi) bad("no previous regular expression");
        a->re = NULL;
    } else a->re = compile(s, n, icase, multi);
    cu_free(s);
}

static int address(Addr *a, int second)
{
    int c = peek();
    memset(a, 0, sizeof *a);
    if (c == '/') { pos++; regaddr(a, '/'); return 1; }
    if (c == '\\') { pos++; regaddr(a, next()); return 1; }
    if (c == '$') { pos++; a->type = A_LAST; return 1; }
    if (c >= '0' && c <= '9') {
        a->n = number();
        a->type = A_LINE;
        skipblank();
        if (peek() == '~' && !second) {
            pos++;
            skipblank();
            a->step = number();
            a->type = a->step > 0 ? A_STEP : A_LINE;
        }
        return 1;
    }
    if (second && (c == '+' || c == '~')) {
        pos++;
        skipblank();
        if (peek() < '0' || peek() > '9') bad("expected a number after `+' or `~'");
        a->n = number();
        a->type = c == '+' ? A_PLUS : A_MULT;
        return 1;
    }
    return 0;
}

/* after a command: blanks, then ; newline } # or the end */
static void endcmd(void)
{
    int c;
    skipblank();
    c = peek();
    if (c == -1 || c == '\n' || c == ';') { if (c >= 0) pos++; return; }
    if (c == '}' || c == '#') return;
    bad("extra characters after command");
}

/* r w R W and s///w: the rest of the line */
static char *filename(void)
{
    LONG s;
    char *f;
    skipblank();
    s = pos;
    while (pos < slen && S[pos] != '\n') pos++;
    if (pos == s) bad("missing filename in r/R/w/W commands");
    if (!(f = cu_alloc(pos - s + 1))) die(20);
    memcpy(f, S + s, pos - s);
    f[pos - s] = 0;
    if (pos < slen) pos++;
    return f;
}

static char *label(int colon)
{
    LONG s;
    char *l;
    skipblank();
    s = pos;
    while (pos < slen && S[pos] != '\n' && S[pos] != ';' && !blank(S[pos]) && (colon || S[pos] != '}')) pos++;
    if (!(l = cu_alloc(pos - s + 1))) die(20);
    memcpy(l, S + s, pos - s);
    l[pos - s] = 0;
    if (colon && !*l) bad("\":\" lacks a label");
    return l;
}

static void text(Cmd *c)
{
    Buf b;
    int ch;
    memset(&b, 0, sizeof b);
    skipblank();
    ch = next();
    if (ch < 0 || ch == '\n') bad("expected \\ after `a', `c' or `i'");
    if (ch == '\\') {
        ch = next();
        if (ch < 0) { c->text = ""; c->tlen = 0; return; }   /* a\ and nothing: nothing */
        if (ch != '\n') pos--;               /* a\text (GNU): the text starts here */
    } else pos--;
    for (;;) {
        ch = next();
        if (ch < 0 || ch == '\n') break;
        if (ch == '\\') {
            LONG used;
            int e;
            if (peek() < 0) break;
            if (peek() == '\n') {
                if (partend(pos)) { pos++; break; }   /* a one-line text ends with its -e */
                bputc(&b, next());
                continue;
            }
            if ((e = escape(S + pos, slen - pos, &used)) >= 0) { bputc(&b, e); pos += used; continue; }
            ch = next();
        }
        bputc(&b, ch);
    }
    bputc(&b, sep);
    c->text = b.s; c->tlen = b.n;
}

/* s's replacement into pieces */
static void replacement(Cmd *c, const char *s, LONG n)
{
    Buf lit;
    int np = 0, max = 8;
    LONG i, used;
    Piece *p = cu_alloc(max * sizeof *p);
    memset(&lit, 0, sizeof lit);
    if (!p) die(20);
#define ADD(k, g, o, l) do { if (np + 1 >= max) { Piece *t = cu_alloc(2 * max * sizeof *t); if (!t) die(20); memcpy(t, p, np * sizeof *t); cu_free(p); p = t; max *= 2; } \
                             p[np].kind = (k); p[np].group = (g); p[np].off = (o); p[np].len = (l); np++; } while (0)
    for (i = 0; i < n; i++) {
        int ch = (unsigned char)s[i], e;
        if (ch == '&') { ADD(P_GROUP, 0, 0, 0); continue; }
        if (ch == '\\' && i + 1 < n) {
            int d = (unsigned char)s[++i];
            if (d >= '0' && d <= '9') {
                if (d - '0' > (c->re ? c->re->ngroup : 9)) {
                    char t[60];
                    strcpy(t, "invalid reference \\");
                    t[19] = d; t[20] = 0;
                    strcat(t, " on `s' command's RHS");
                    bad(t);
                }
                ADD(P_GROUP, d - '0', 0, 0);
                continue;
            }
            if (d == 'U') { ADD(P_UPPER, 0, 0, 0); continue; }
            if (d == 'L') { ADD(P_LOWER, 0, 0, 0); continue; }
            if (d == 'u') { ADD(P_UONE, 0, 0, 0); continue; }
            if (d == 'l') { ADD(P_LONE, 0, 0, 0); continue; }
            if (d == 'E') { ADD(P_END, 0, 0, 0); continue; }
            if (d == '\n') { ch = '\n'; }
            else if ((e = escape(s + i, n - i, &used)) >= 0) { ch = e; i += used - 1; }
            else ch = d;
        }
        /* a plain byte: into the last literal piece, or a new one */
        if (np && p[np - 1].kind == P_LIT && p[np - 1].off + p[np - 1].len == lit.n) p[np - 1].len++;
        else ADD(P_LIT, 0, lit.n, 1);
        bputc(&lit, ch);
    }
#undef ADD
    c->rep = p; c->nrep = np; c->rtext = lit.s;
}

static void scmd(Cmd *c)
{
    int d = next();
    LONG rn, pn;
    char *re, *rp;
    int icase = 0, multi = 0;
    if (d < 0 || d == '\n' || d == '\\') bad("unterminated `s' command");
    re = delimited(d, 1, &rn, "unterminated `s' command");
    rp = delimited(d, 0, &pn, "unterminated `s' command");
    for (;;) {
        int ch = next();
        if (ch == 'g') { if (c->gflag) bad("multiple `g' options to `s' command"); c->gflag = 1; }
        else if (ch == 'p') { if (c->pflag) bad("multiple `p' options to `s' command"); c->pflag = 1; }
        else if (ch == 'i' || ch == 'I') icase = 1;
        else if (ch == 'm' || ch == 'M') multi = 1;
        else if (ch == 'e') bad("the e flag is not supported (no shell to run)");
        else if (ch >= '0' && ch <= '9') {
            if (c->nth) bad("multiple number options to `s' command");
            pos--;
            if (!(c->nth = number())) bad("number option to `s' command may not be zero");
        } else if (ch == 'w') { c->wout = wfile(filename()); break; }
        else if (blank(ch)) ;
        else if (ch == '}' || ch == '#') { pos--; break; }
        else if (ch == '\n' || ch == ';' || ch < 0) break;
        else bad("unknown option to `s'");
    }
    if (!rn) {
        if (icase || multi) bad("no previous regular expression");
        c->re = NULL;
    } else c->re = compile(re, rn, icase, multi);
    replacement(c, rp, pn);
    cu_free(re); cu_free(rp);
    if (!c->nth) c->nth = 1;
}

static void ycmd(Cmd *c)
{
    int d = next(), k;
    LONG an, bn, i, j;
    char *a, *b;
    unsigned char fa[256], fb[256];
    int na = 0, nb = 0;
    if (d < 0 || d == '\n' || d == '\\') bad("unterminated `y' command");
    a = delimited(d, 0, &an, "unterminated `y' command");
    b = delimited(d, 0, &bn, "unterminated `y' command");
    for (k = 0; k < 2; k++) {
        char *s = k ? b : a;
        LONG n = k ? bn : an, used;
        unsigned char *f = k ? fb : fa;
        int *cnt = k ? &nb : &na;
        for (i = 0; i < n; i++) {
            int ch = (unsigned char)s[i], e;
            if (ch == '\\' && i + 1 < n) {
                if (s[i + 1] == '\\') { ch = '\\'; i++; }
                else if ((e = escape(s + i + 1, n - i - 1, &used)) >= 0) { ch = e; i += used; }
                else bad("unknown command: `\\'");
            }
            if (*cnt >= 256) bad("`y' command strings have different lengths");
            f[(*cnt)++] = ch;
        }
    }
    if (na != nb) bad("`y' command strings have different lengths");
    if (!(c->ymap = cu_alloc(256))) die(20);
    for (j = 0; j < 256; j++) c->ymap[j] = j;
    for (j = na - 1; j >= 0; j--) c->ymap[fa[j]] = fb[j];   /* the first one wins */
    cu_free(a); cu_free(b);
    endcmd();
}

static RFile *rfiles;

static void parse(void)
{
    int stack[64], depth = 0, i, j;
    pos = 0;
    /* #n alone on the first line: -n */
    if (slen >= 2 && S[0] == '#' && S[1] == 'n' && (slen == 2 || S[2] == '\n')) o_quiet = 1;
    for (;;) {
        Cmd *c;
        int ch;
        while (pos < slen && (blank(S[pos]) || S[pos] == '\n' || S[pos] == ';')) pos++;
        if (pos >= slen) break;
        if (S[pos] == '#') { while (pos < slen && S[pos] != '\n') pos++; continue; }
        if (ncmd == maxcmd) {
            Cmd *t = cu_alloc((maxcmd = maxcmd ? maxcmd * 2 : 32) * sizeof *t);
            if (!t) die(20);
            if (cmds) { memcpy(t, cmds, ncmd * sizeof *t); cu_free(cmds); }
            cmds = t;
        }
        c = &cmds[ncmd];
        memset(c, 0, sizeof *c);
        c->num = -1;
        if (address(&c->a1, 0)) {
            skipblank();
            if (peek() == ',') {
                pos++;
                skipblank();
                if (!address(&c->a2, 1)) bad("unexpected `,'");
            }
            if (c->a1.type == A_LINE && c->a1.n == 0 && (c->a2.type != A_RE)) bad("invalid usage of line address 0");
            if (c->a1.type == A_LINE && c->a1.n == 0) { c->a1.type = A_ZERO; c->range = c->range0 = R_ACTIVE; }
        }
        skipblank();
        if (peek() == '!') {
            pos++;
            c->neg = 1;
            skipblank();
            if (peek() == '!') bad("multiple `!'s");
        }
        ch = next();
        if (ch < 0 || ch == '\n' || ch == ';') { pos--; bad("missing command"); }
        c->cmd = ch;
        if (c->a1.type && strchr(":}", ch)) bad(ch == ':' ? ": doesn't want any addresses" : "} doesn't want any addresses");
        if (c->a2.type && (ch == 'q' || ch == 'Q')) bad("command only uses one address");
        switch (ch) {
        case '{':
            if (depth == 64) bad("too many { (64 deep)");
            stack[depth++] = ncmd;
            break;
        case '}':
            if (!depth) bad("unexpected `}'");
            cmds[stack[--depth]].num = ncmd;
            endcmd();
            break;
        case ':':
            c->label = label(1);
            for (i = 0; i < ncmd; i++) if (cmds[i].cmd == ':' && !strcmp(cmds[i].label, c->label)) bad("duplicate label");   /* hmm GNU 4.10 allows */
            if (peek() == ';') pos++;
            break;
        case 'b': case 't': case 'T':
            c->label = label(0);
            skipblank();
            if (peek() == ';') pos++;      /* GNU: ta b is ta, then b */
            break;
        case 'a': case 'i': case 'c':
            text(c);
            break;
        case 'r': case 'R': case 'w': case 'W':
            c->text = filename();
            if (ch == 'w' || ch == 'W') c->wout = wfile(c->text);
            if (ch == 'R') {
                RFile *f;
                for (f = rfiles; f; f = f->next) if (!strcmp(f->name, c->text)) break;
                if (!f) {
                    if (!(f = cu_alloc(sizeof *f))) die(20);
                    memset(f, 0, sizeof *f);
                    f->name = c->text;
                    f->next = rfiles; rfiles = f;
                }
                c->rf = f;
            }
            break;
        case 's': scmd(c); break;
        case 'y': ycmd(c); break;
        case 'q': case 'Q': case 'l': case 'L':
            if (ch == 'L') bad("unknown command: `L'");
            skipblank();
            if (peek() >= '0' && peek() <= '9') c->num = number();
            else if (ch != 'l') c->num = 0;
            endcmd();
            break;
        case 'v':
            while (pos < slen && S[pos] != '\n' && S[pos] != ';' && S[pos] != '}' && S[pos] != '#') pos++;
            endcmd();
            break;
        case 'e':
            bad("the e command is not supported (no shell to run)");
            break;
        case '=': case 'd': case 'D': case 'g': case 'G': case 'h': case 'H': case 'x': case 'n': case 'N':
        case 'p': case 'P': case 'z': case 'F':
            endcmd();
            break;
        default:
            bad2("unknown command: `", ch, "'");
        }
        ncmd++;
    }
    if (depth) { pos = slen; bad("unmatched `{'"); }
    /* the jumps */
    for (i = 0; i < ncmd; i++) {
        Cmd *c = &cmds[i];
        if (!strchr("btT", c->cmd)) continue;
        if (!*c->label) { c->num = ncmd; continue; }
        for (j = 0; j < ncmd; j++) if (cmds[j].cmd == ':' && !strcmp(cmds[j].label, c->label)) break;
        if (j == ncmd) {
            char t[300];
            strcpy(t, "can't find label for jump to `");
            strncat(t, c->label, 250);
            strcat(t, "'");
            cu_err(10, NULL, t);
            die(10);
        }
        c->num = j;
    }
}

/* ---- the input: files one after another, a line looked ahead ($) ---- */

static char **files;
static int nfiles, fileat, fileend;     /* the files of this run: fileat up to fileend */
static Src src;
static int srcopen;
static char *srcname, *curname, *nxname;
static Buf ps, hs, nx;
static int havenx;
static LONG lineno;
static int quitting, tflag;
static LONG exitcode = -1;

static int openfile(const char *name)
{
    BPTR l;
    if (strcmp(name, "-") && strcmp(name, "/dev/stdin") && (l = Lock((STRPTR)name, SHARED_LOCK))) {
        struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
        int dir = fib && Examine(l, fib) && fib->fib_DirEntryType > 0 && fib->fib_DirEntryType != ST_SOFTLINK;
        if (fib) FreeDosObject(DOS_FIB, fib);
        UnLock(l);
        if (dir) { cu_err(10, name, "is a drawer"); return 0; }
    }
    if (!src_open(&src, name)) { cu_fault(10, name); return 0; }
    srcopen = 1;
    srcname = (char *)name;
    return 1;
}

static void lookahead(void)
{
    havenx = 0;
    for (;;) {
        if (!srcopen) {
            if (fileat >= fileend) return;
            if (!openfile(files[fileat++])) continue;
        }
        if (src_line(&src, &nx, 0)) { havenx = 1; nxname = srcname; return; }
        if (src.err) cu_fault(10, srcname);
        src_close(&src);
        srcopen = 0;
    }
}

/* the next line into the pattern space (append: N) */
static int readline(int append)
{
    if (!havenx) return 0;
    if (append) { bputc(&ps, sep); bcat(&ps, nx.s, nx.n); ps.nl = nx.nl; }
    else bswap(&ps, &nx);
    curname = nxname;
    lineno++;
    tflag = 0;
    lookahead();
    return 1;
}

/* ---- running ---- */

static int matchre(Re *re, const char *s, LONG n, LONG from, int *caps)
{
    int r;
    if (!re) {
        if (!lastre) { cu_err(10, NULL, "no previous regular expression"); die(10); }
        re = lastre;
    }
    lastre = re;
    r = re_exec(re->prog, s, n, from, caps);
    if (r < 0) { cu_err(20, NULL, "out of memory"); die(20); }
    return r;
}

static int match1(Addr *a)
{
    int caps[2 * RE_GROUPS];
    switch (a->type) {
    case A_LINE: return lineno == a->n;
    case A_LAST: return !havenx;
    case A_STEP: return lineno >= a->n && (lineno - a->n) % a->step == 0;
    case A_RE: return matchre(a->re, ps.s, ps.n, 0, caps);
    }
    return 0;
}

static int matches(Cmd *c)
{
    if (!c->a1.type) return 1;
    if (!c->a2.type) return match1(&c->a1);
    switch (c->range) {
    case R_CLOSED: return 0;
    case R_INACTIVE:
        if (!match1(&c->a1)) return 0;
        switch (c->a2.type) {
        case A_LINE:
            if (lineno >= c->a2.n) return 1;
            c->rend = c->a2.n;
            break;
        case A_PLUS:
            if (!c->a2.n) return 1;
            c->rend = lineno + c->a2.n;
            break;
        case A_MULT:
            if (c->a2.n <= 0) return 1;
            c->rend = (lineno / c->a2.n + 1) * c->a2.n;   /* the next multiple (4,~4 is 4 to 8: GNU) */
            break;
        }
        c->range = R_ACTIVE;
        return 1;
    default:                               /* R_ACTIVE */
        switch (c->a2.type) {
        case A_LINE:
            if (lineno >= c->rend) {
                c->range = R_INACTIVE;
                if (lineno > c->rend) return match1(&c->a1);   /* past it (N): only when a1 matches (GNU) */
            }
            return 1;
        case A_PLUS: case A_MULT:
            if (lineno >= c->rend) c->range = R_INACTIVE;     /* past it too: this line is in (GNU) */
            return 1;
        case A_LAST:
            if (!havenx) c->range = R_INACTIVE;
            return 1;
        default:
            if (match1(&c->a2)) c->range = R_INACTIVE;
            return 1;
        }
    }
}

/* things a, r, R put aside until the line is out */
typedef struct App { struct App *next; int kind; const char *s; LONG n; } App;
static App *apps, **applast = &apps;

static void append(int kind, const char *s, LONG n)
{
    App *a = cu_alloc(sizeof *a + (kind == 'R' ? n : 0));
    if (!a) die(20);
    a->next = NULL; a->kind = kind; a->n = n;
    if (kind == 'R') { memcpy(a + 1, s, n); a->s = (char *)(a + 1); } else a->s = s;
    *applast = a; applast = &a->next;
}

static void dumpapp(void)
{
    App *a, *nx_;
    for (a = apps; a; a = nx_) {
        nx_ = a->next;
        if (a->kind == 'r') {
            Src f;
            if (src_open(&f, a->s)) {
                ofix(cur);
                while (src_fill(&f)) owrite(cur, f.buf, f.len);
                src_close(&f);
            }
        } else otext(cur, a->s, a->n);
        cu_free(a);
    }
    apps = NULL; applast = &apps;
}

static Buf sb;                             /* s builds its result here */

static void addcase(const char *s, LONG n, int *mode, int *one)
{
    LONG i;
    if (!*mode && !*one) { bcat(&sb, s, n); return; }
    bneed(&sb, sb.n + n);
    for (i = 0; i < n; i++) {
        int c = (unsigned char)s[i];
        if (*one) { c = *one == 'u' ? upper(c) : lower(c); *one = 0; }
        else if (*mode) c = *mode == 'U' ? upper(c) : lower(c);
        sb.s[sb.n++] = c;
    }
}

static int subst(Cmd *c)
{
    int caps[2 * RE_GROUPS], did = 0, k;
    LONG at = 0, count = 0, prev = -1;
    sb.n = 0;
    while (at <= ps.n) {
        LONG ms, me;
        if (!matchre(c->re, ps.s, ps.n, at, caps)) break;
        ms = caps[0]; me = caps[1];
        if (ms == me && ms == prev) {      /* empty, right after the last match: not one */
            if (ms >= ps.n) break;
            bputc(&sb, ps.s[ms]);
            at = ms + 1;
            continue;
        }
        bcat(&sb, ps.s + at, ms - at);
        count++;
        if (count >= c->nth) {
            int mode = 0, one = 0;
            for (k = 0; k < c->nrep; k++) {
                Piece *p = &c->rep[k];
                switch (p->kind) {
                case P_LIT: addcase(c->rtext + p->off, p->len, &mode, &one); break;
                case P_GROUP:
                    if (caps[2 * p->group] >= 0 && caps[2 * p->group + 1] >= caps[2 * p->group])
                        addcase(ps.s + caps[2 * p->group], caps[2 * p->group + 1] - caps[2 * p->group], &mode, &one);
                    break;
                case P_UPPER: mode = 'U'; one = 0; break;
                case P_LOWER: mode = 'L'; one = 0; break;
                case P_UONE: one = 'u'; break;
                case P_LONE: one = 'l'; break;
                case P_END: mode = one = 0; break;
                }
            }
            did = 1;
        } else bcat(&sb, ps.s + ms, me - ms);
        at = prev = me;
        if (did && !c->gflag) break;
        if (ms == me) {
            if (me < ps.n) bputc(&sb, ps.s[me]);
            at = me + 1;
        }
    }
    if (!did) return 0;
    if (at < ps.n) bcat(&sb, ps.s + at, ps.n - at);
    sb.nl = ps.nl;
    bswap(&ps, &sb);
    return 1;
}

static void list(LONG width)
{
    char o[8];
    LONG i, col = 0;
    int k;
    ofix(cur);
    for (i = 0; i < ps.n; i++) {
        int c = (unsigned char)ps.s[i];
        k = 0;
        if (c == '\\') { o[k++] = '\\'; o[k++] = '\\'; }
        else if (c >= 32 && c < 127) o[k++] = c;
        else {
            const char *e = strchr("\aa\bb\ff\nn\rr\tt\vv", c);
            o[k++] = '\\';
            if (c && e && (e - "\aa\bb\ff\nn\rr\tt\vv") % 2 == 0) o[k++] = e[1];
            else { o[k++] = '0' + (c >> 6); o[k++] = '0' + ((c >> 3) & 7); o[k++] = '0' + (c & 7); }
        }
        if (width > 0 && col + k > width - 1) { owrite(cur, "\\\n", 2); col = 0; }
        owrite(cur, o, k);
        col += k;
    }
    owrite(cur, "$", 1);
    owrite(cur, &sep, 1);
}

static void putnum(Out *o, LONG v)
{
    char t[12], b[12];
    int i = 0, n = 0;
    do t[i++] = '0' + v % 10; while ((v /= 10));
    while (i) b[n++] = t[--i];
    b[n++] = sep;
    otext(o, b, n);
}

static void resetranges(void)
{
    int i;
    for (i = 0; i < ncmd; i++) cmds[i].range = cmds[i].range0;
}

/* one cycle on the pattern space: 0 go on, 1 stop reading (q Q) */
static int cycle(void)
{
    int pc = 0;
    LONG jumps = 0;
again:
    while (pc < ncmd) {
        Cmd *c = &cmds[pc];
        if (matches(c) == c->neg) {
            pc = c->cmd == '{' ? c->num + 1 : pc + 1;
            continue;
        }
        switch (c->cmd) {
        case '{': case '}': case ':': break;
        case '=': putnum(cur, lineno); break;
        case 'a': append('t', c->text, c->tlen); break;
        case 'i': otext(cur, c->text, c->tlen); break;
        case 'c':
            if (c->range != R_ACTIVE) otext(cur, c->text, c->tlen);
            goto deleted;
        case 'b': pc = c->num; goto jumped;
        case 't': if (tflag) { tflag = 0; pc = c->num; goto jumped; } break;
        case 'T': if (!tflag) { pc = c->num; goto jumped; } tflag = 0; break;
        case 'd': goto deleted;
        case 'D': {
            char *e = memchr(ps.s, sep, ps.n);
            LONG k;
            if (!e) goto deleted;
            k = e - ps.s + 1;
            memmove(ps.s, ps.s + k, ps.n - k);
            ps.n -= k;
            pc = 0;                        /* again, without reading (the a's wait: GNU) */
            if (cu_break()) return 1;
            goto again;
        }
        case 'g': bset(&ps, hs.s, hs.n); ps.nl = hs.nl; break;
        case 'G': bputc(&ps, sep); bcat(&ps, hs.s, hs.n); ps.nl = hs.nl; break;
        case 'h': bset(&hs, ps.s, ps.n); hs.nl = ps.nl; break;
        case 'H': bputc(&hs, sep); bcat(&hs, ps.s, ps.n); hs.nl = ps.nl; break;
        case 'x': bswap(&ps, &hs); break;
        case 'l': list(c->num >= 0 ? c->num : o_width); break;
        case 'n':
            if (!havenx) goto end;         /* no next line: as at the end (GNU) */
            if (!o_quiet) oline(cur, ps.s, ps.n, ps.nl);
            dumpapp();
            readline(0);
            break;
        case 'N':
            if (!havenx) { if (o_posix) goto deleted; goto end; }
            dumpapp();
            readline(1);
            break;
        case 'p': oline(cur, ps.s, ps.n, ps.nl); break;
        case 'P': {
            char *e = memchr(ps.s, sep, ps.n);
            oline(cur, ps.s, e ? e - ps.s : ps.n, e ? 1 : ps.nl);
            break;
        }
        case 'q':
            exitcode = c->num;
            if (!o_quiet) oline(cur, ps.s, ps.n, ps.nl);
            dumpapp();
            return 1;
        case 'Q':
            exitcode = c->num;
            return 1;
        case 'r': append('r', c->text, 0); break;
        case 'R':
            if (!c->rf->done) {
                Buf l;
                memset(&l, 0, sizeof l);
                if (!c->rf->open) {
                    c->rf->open = 1;
                    if (!src_open(&c->rf->src, c->rf->name)) c->rf->done = 1;
                }
                if (!c->rf->done) {
                    if (src_line(&c->rf->src, &l, 0)) {
                        if (l.nl) bputc(&l, sep);
                        append('R', l.s, l.n);
                    } else c->rf->done = 1;
                    cu_free(l.s);
                }
            }
            break;
        case 's':
            if (subst(c)) {
                tflag = 1;
                if (c->pflag) oline(cur, ps.s, ps.n, ps.nl);
                if (c->wout) oline(c->wout, ps.s, ps.n, ps.nl);
            }
            break;
        case 'w': oline(c->wout, ps.s, ps.n, ps.nl); break;
        case 'W': {
            char *e = memchr(ps.s, sep, ps.n);
            oline(c->wout, ps.s, e ? e - ps.s : ps.n, e ? 1 : ps.nl);
            break;
        }
        case 'y': {
            LONG i;
            for (i = 0; i < ps.n; i++) ps.s[i] = c->ymap[(unsigned char)ps.s[i]];
            break;
        }
        case 'z': ps.n = 0; break;
        case 'F': {
            const char *f = curname && strcmp(curname, "/dev/stdin") ? curname : "-";
            otext(cur, f, strlen(f));
            owrite(cur, &sep, 1);
            break;
        }
        }
        pc++;
        continue;
    jumped:
        if (!(++jumps & 4095) && cu_break()) return 1;
    }
end:
    if (!o_quiet) oline(cur, ps.s, ps.n, ps.nl);
deleted:
    dumpapp();
    return 0;
}

/* all lines of files[fileat..fileend) */
static void run(void)
{
    LONG n = 0;
    lookahead();
    while (!quitting && readline(0)) {
        if (cycle()) quitting = 1;
        if (o_unbuf) { oflush(cur); }
        if (!(++n & 255) && cu_break()) { quitting = 1; break; }
        if (cu_rc >= 20) { quitting = 1; break; }
    }
    if (srcopen) { src_close(&src); srcopen = 0; }
}

/* ---- -i: a file edited in place ---- */

static char tmpname[CU_PATH + 32];
static int tmpopen;

static void die(int rc)
{
    if (tmpopen) { Close(mainout.fh); DeleteFile((STRPTR)tmpname); tmpopen = 0; }
    cu_flush();
    exit(rc);
}

static void inplace(const char *name)
{
    BPTR l;
    struct FileInfoBlock *fib;
    LONG prot = 0, dlen;
    char note[80], backup[CU_PATH + 64];
    static int count;
    int ok = 1, k;
    ULONG id;
    note[0] = 0;
    if (!strcmp(name, "-") || !strcmp(name, "/dev/stdin")) { cu_err(10, name, "cannot edit the input in place"); return; }
    if (!(l = Lock((STRPTR)name, SHARED_LOCK))) { cu_fault(10, name); return; }
    if (!(fib = AllocDosObject(DOS_FIB, NULL))) { UnLock(l); die(20); }
    if (Examine(l, fib)) {
        if (fib->fib_DirEntryType > 0) ok = 0;
        prot = fib->fib_Protection;
        strncpy(note, (char *)fib->fib_Comment, sizeof note - 1);
        note[sizeof note - 1] = 0;
    }
    FreeDosObject(DOS_FIB, fib);
    UnLock(l);
    if (!ok) { cu_err(10, name, "not a file (is a drawer)"); return; }
    dlen = (const char *)PathPart((STRPTR)name) - name;
    if (dlen + 20 > CU_PATH) { cu_err(10, name, "name too long"); return; }
    memcpy(tmpname, name, dlen);
    id = (ULONG)FindTask(NULL) ^ (ULONG)++count;
    strcpy(tmpname + dlen, dlen && name[dlen - 1] != ':' && name[dlen - 1] != '/' ? "/sed" : "sed");
    k = strlen(tmpname);
    { int i; for (i = 28; i >= 0; i -= 4) tmpname[k++] = "0123456789abcdef"[(id >> i) & 15]; tmpname[k] = 0; }
    if (!(mainout.fh = Open((STRPTR)tmpname, MODE_NEWFILE))) { cu_fault(10, tmpname); return; }
    tmpopen = 1;
    mainout.name = tmpname;
    mainout.missing = 0;
    if (!mainout.buf && !(mainout.buf = cu_alloc(4096))) die(20);
    run();
    oflush(&mainout);
    Close(mainout.fh);
    mainout.fh = 0;
    tmpopen = 0;
    if (cu_rc >= 20) { DeleteFile((STRPTR)tmpname); return; }
    /* the old one out of the way: a backup, or gone */
    if (o_suffix && *o_suffix) {
        if (strchr(o_suffix, '*')) {
            const char *s, *base = (const char *)FilePart((STRPTR)name);
            char t[CU_PATH + 64];
            int n = 0;
            for (s = o_suffix; *s && n < CU_PATH; s++) {
                if (*s == '*') { int bl = strlen(base); if (n + bl >= CU_PATH) break; memcpy(t + n, base, bl); n += bl; }
                else t[n++] = *s;
            }
            t[n] = 0;
            if (strchr(t, '/') || strchr(t, ':')) strcpy(backup, t);
            else { memcpy(backup, name, dlen); backup[dlen] = 0; if (!cu_join(backup, backup, t)) strcpy(backup, t); if (!dlen) strcpy(backup, t); }
        } else {
            if (strlen(name) + strlen(o_suffix) >= sizeof backup) { cu_err(10, name, "name too long"); DeleteFile((STRPTR)tmpname); return; }
            strcpy(backup, name);
            strcat(backup, o_suffix);
        }
        DeleteFile((STRPTR)backup);
        if (!Rename((STRPTR)name, (STRPTR)backup)) { cu_fault(10, backup); DeleteFile((STRPTR)tmpname); return; }
    } else if (!DeleteFile((STRPTR)name)) { cu_fault(10, name); DeleteFile((STRPTR)tmpname); return; }
    if (!Rename((STRPTR)tmpname, (STRPTR)name)) { cu_fault(10, name); return; }
    SetProtection((STRPTR)name, prot);
    if (note[0]) SetComment((STRPTR)name, (STRPTR)note);
}

/* ---- the command line ---- */

static void addpart(const char *s, LONG n, int e, char *file)
{
    if (nparts % 16 == 0) {
        Part *t = cu_alloc((nparts + 16) * sizeof *t);
        if (!t) die(20);
        if (parts) { memcpy(t, parts, nparts * sizeof *t); cu_free(parts); }
        parts = t;
    }
    if (slen + n + 2 > smax) {
        char *t;
        smax = (slen + n + 2) * 2;
        if (!(t = cu_alloc(smax))) die(20);
        if (S) { memcpy(t, S, slen); cu_free(S); }
        S = t;
    }
    if (nparts) S[slen++] = '\n';
    parts[nparts].at = slen; parts[nparts].e = e; parts[nparts].file = file;
    nparts++;
    memcpy(S + slen, s, n);
    slen += n;
    S[slen] = 0;
}

static void addfile(char *name)
{
    Src f;
    Buf b;
    memset(&b, 0, sizeof b);
    if (!src_open(&f, name)) { cu_fault(10, name); die(10); }
    while (src_fill(&f)) bcat(&b, f.buf, f.len);
    src_close(&f);
    if (b.n && b.s[b.n - 1] == '\n') b.n--;
    addpart(b.s ? b.s : "", b.n, 0, name);
    cu_free(b.s);
}

static int addname(const char *path, void *u)
{
    (void)u;
    if (nfiles % 32 == 0) {
        char **t = cu_alloc((nfiles + 33) * sizeof *t);
        if (!t) die(20);
        if (files) { memcpy(t, files, nfiles * sizeof *t); cu_free(files); }
        files = t;
    }
    if (!(files[nfiles] = cu_strdup(path))) die(20);
    nfiles++;
    return 0;
}

static int longopt(const char *w, const char *name, char **val, int *ai)
{
    int n = strlen(name);
    if (strncmp(w + 2, name, n)) return 0;
    if (w[2 + n] == '=') { if (!val) return 0; *val = (char *)w + 3 + n; return 1; }
    if (w[2 + n]) return 0;
    if (val) {
        if (*ai + 1 >= cu_argc) { cu_err(10, w, "needs a value"); die(10); }
        *val = cu_argv[++*ai];
    }
    return 1;
}

int main(void)
{
    char **words, *v;
    int i, nw = 0, script = 0, noopt = 0;
    (void)version;
    if (cu_args("sed [-nEsuz] [-i[SUFFIX]] [-l N] [-e SCRIPT]... [-f FILE]... [SCRIPT] [file ...]") < 0) return cu_rc;
    if (!(words = cu_alloc((cu_argc + 1) * sizeof *words))) return 20;
    for (i = 0; i < cu_argc; i++) {
        char *w = cu_argv[i];
        if (noopt || w[0] != '-' || !w[1]) { words[nw++] = w; continue; }
        if (w[1] == '-') {
            if (!w[2]) { noopt = 1; continue; }
            if (longopt(w, "expression", &v, &i)) { addpart(v, strlen(v), ++nexpr, NULL); script = 1; }
            else if (longopt(w, "file", &v, &i)) { addfile(v); script = 1; }
            else if (longopt(w, "quiet", NULL, &i) || longopt(w, "silent", NULL, &i)) o_quiet = 1;
            else if (longopt(w, "regexp-extended", NULL, &i)) o_ext = 1;
            else if (longopt(w, "separate", NULL, &i)) o_sep = 1;
            else if (longopt(w, "unbuffered", NULL, &i)) o_unbuf = 1;
            else if (longopt(w, "null-data", NULL, &i) || longopt(w, "zero-terminated", NULL, &i)) sep = 0;
            else if (longopt(w, "posix", NULL, &i)) o_posix = 1;
            else if (longopt(w, "debug", NULL, &i) || longopt(w, "sandbox", NULL, &i) || longopt(w, "follow-symlinks", NULL, &i)) ;
            else if (!strncmp(w, "--in-place", 10) && (!w[10] || w[10] == '=')) { o_inplace = o_sep = 1; o_suffix = w[10] ? w + 11 : NULL; }
            else if (longopt(w, "line-length", &v, &i)) { int ok; o_width = cu_num(v, &ok); if (!ok) { cu_err(10, v, "not a number"); return cu_rc; } }
            else if (!strcmp(w, "--version")) { cu_puts((char *)version + 6); cu_putc('\n'); cu_flush(); return 0; }
            else if (!strcmp(w, "--help")) { cu_puts("usage: sed [-nEsuz] [-i[SUFFIX]] [-l N] [-e SCRIPT]... [-f FILE]... [SCRIPT] [file ...]\n"); cu_flush(); return 0; }
            else { cu_err(10, w, "unknown option"); return cu_rc; }
            continue;
        }
        for (w++; *w; w++) {
            switch (*w) {
            case 'n': o_quiet = 1; continue;
            case 'E': case 'r': o_ext = 1; continue;
            case 's': o_sep = 1; continue;
            case 'u': o_unbuf = 1; continue;
            case 'z': sep = 0; continue;
            case 'i': o_inplace = o_sep = 1; o_suffix = w[1] ? w + 1 : NULL; break;
            case 'e': case 'f': case 'l':
                if (w[1]) v = w + 1;
                else if (i + 1 < cu_argc) v = cu_argv[++i];
                else { char t[3] = { '-', *w, 0 }; cu_err(10, t, "needs a value"); return cu_rc; }
                if (*w == 'e') addpart(v, strlen(v), ++nexpr, NULL);
                else if (*w == 'f') addfile(v);
                else { int ok; o_width = cu_num(v, &ok); if (!ok) { cu_err(10, v, "not a number"); return cu_rc; } }
                if (*w != 'l') script = 1;
                break;
            default: {
                char t[3] = { '-', *w, 0 };
                cu_err(10, t, "unknown option");
                return cu_rc;
            }
            }
            break;
        }
    }
    if (!script) {
        if (!nw) { cu_err(10, NULL, "no script (sed ? for help)"); return cu_rc; }
        addpart(words[0], strlen(words[0]), ++nexpr, NULL);
        words++; nw--;
    }
    parse();
    for (i = 0; i < nw && cu_rc < 20; i++) cu_each(words[i], addname, NULL);
    if (cu_rc >= 20) return cu_rc;
    hs.nl = 1;
    bneed(&ps, 0); bneed(&hs, 0); bneed(&nx, 0); bneed(&sb, 0);
    if (!nfiles && !nw) {
        if (o_inplace) { cu_err(10, NULL, "no input files"); return cu_rc; }
        static char *dash[1] = { "-" };
        files = dash; nfiles = 1;
    }
    if (o_inplace) {
        char **all = files;
        int n = nfiles;
        for (i = 0; i < n && !quitting && cu_rc < 20; i++) {
            files = &all[i]; fileat = 0; fileend = 1;
            lineno = 0;
            resetranges();
            inplace(all[i]);
        }
    } else if (o_sep) {
        for (i = 0; i < nfiles && !quitting && cu_rc < 20; i++) {
            fileat = i; fileend = i + 1;
            lineno = 0;
            resetranges();
            run();
        }
    } else {
        fileat = 0; fileend = nfiles;
        run();
    }
    oflush(&mainout);
    if (errout_.fh) oflush(&errout_);
    {
        Out *o;
        for (o = outs; o; o = o->next) { oflush(o); if (o->own) Close(o->fh); }
    }
    cu_flush();
    if (cu_rc >= 20) return cu_rc;
    if (exitcode >= 0 && cu_rc < 10) return exitcode ? exitcode : cu_rc;
    return cu_rc;
}
