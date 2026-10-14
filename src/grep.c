/* grep - lines that match a pattern.
 *
 *   grep [-EFivwxcLlnhHoqsr] [-e PAT] [-m N] [-A N] [-B N] [-C N] PATTERN [file ...]
 *
 * The pattern is a regular expression: basic (GNU grep's default: ( ) | { }
 * + ? are plain, \( \) \| \{ \} \+ \? are special) or with -E extended.
 * . [abc] [^a-z] [[:alpha:]] ^ $ * \w \s as usual. No file, or -: the input.
 * Several files: each line after its file's name. A drawer with -r: every
 * file inside, at any depth (else it is skipped with a word).
 *   -E extended   -F the pattern is plain text   -e PAT a pattern starting
 *   with -   -i case blind   -v the lines that do NOT match   -w whole words
 *   -x whole lines   -c a count per file   -l only the names of files that
 *   match   -L of files that do not   -n line numbers   -h never names
 *   -H always names   -o only the matching parts   -q nothing, just the
 *   return code   -s no message about unreadable files   -m N stop after N
 *   matches   -A/-B/-C N lines after/before/around   -r into drawers
 *   --colour=never|always (auto: colours when the output is a window)
 *
 * Return code 0 something matched, 5 nothing did, 10 an error (a file that
 * could not be read, a bad pattern), 20 Ctrl-C. (GNU's 1 and 2: on the
 * Amiga 5 is WARN, so `If WARN` reads "no match", and a script goes on.) */
#include "cu.h"

const char *cu_name = "grep";
static const char version[] = "$VER: grep 0.1b1 (14.10.2026)";

static int o_ext, o_fixed, o_icase, o_inv, o_word, o_line, o_count, o_list, o_nlist,
           o_num, o_only, o_quiet, o_rec, o_names, colour;
static LONG o_max = -1, o_after, o_before;
static void *prog;
static char *lit;              /* the pattern as plain text (no special characters) */
static int litlen, group;      /* group: which capture is the match (-w -x wrap it) */
static int found, files_seen;

/* ---- the pattern ---- */

static int lower(int c) { c &= 0xff; return (c >= 'A' && c <= 'Z') || (c >= 0xc0 && c <= 0xde && c != 0xd7) ? c + 32 : c; }

static int special(int c) { return strchr(".[]()|*+?{}^$\\", c) != NULL; }

/* basic (BRE) to extended (ERE): \( -> (, ( -> \( ... brackets as they are */
static char *bre2ere(const char *s)
{
    char *d = cu_alloc(strlen(s) * 2 + 1), *o = d;
    if (!d) return NULL;
    while (*s) {
        if (*s == '[') {                     /* copy a bracket whole */
            const char *e = s + 1;
            if (*e == '^') e++;
            if (*e == ']') e++;
            while (*e && *e != ']') { if (*e == '[' && e[1] == ':') { const char *f = strstr(e, ":]"); if (f) e = f + 1; } e++; }
            if (*e) { memcpy(o, s, e - s + 1); o += e - s + 1; s = e + 1; continue; }
        }
        if (*s == '\\' && s[1] && strchr("()|{}+?", s[1])) { *o++ = s[1]; s += 2; continue; }
        if (strchr("()|{}+?", *s)) { *o++ = '\\'; *o++ = *s++; continue; }
        if (*s == '*' && (o == d || o[-1] == '(' || o[-1] == '|' || (o - d == 1 && d[0] == '^'))) { *o++ = '\\'; *o++ = *s++; continue; }   /* a leading * is plain in BRE */
        if (*s == '\\' && s[1]) *o++ = *s++;
        *o++ = *s++;
    }
    *o = 0;
    return d;
}

static int plain(const char *s)
{
    for (; *s; s++) if (special((unsigned char)*s)) return 0;
    return 1;
}

static char *quoted(const char *s)      /* -F with -w/-x: the text, escaped */
{
    char *d = cu_alloc(strlen(s) * 2 + 1), *o = d;
    if (!d) return NULL;
    for (; *s; s++) { if (special((unsigned char)*s)) *o++ = '\\'; *o++ = *s; }
    *o = 0;
    return d;
}

static int setup(const char *pat)
{
    char *re;
    if (o_fixed && !o_word && !o_line) { lit = (char *)pat; }
    else {
        re = o_fixed ? quoted(pat) : o_ext ? cu_strdup(pat) : bre2ere(pat);
        if (!re) return 0;
        if (!o_word && !o_line && plain(re)) { lit = re; }   /* plain text: a fast search */
        else {
            if (o_word || o_line) {
                char *w = cu_alloc(strlen(re) + 40);
                if (!w) return 0;
                strcpy(w, o_line ? "^(" : "(^|[^[:alnum:]_])(");
                strcat(w, re);
                strcat(w, o_line ? ")$" : ")([^[:alnum:]_]|$)");
                re = w;
                group = o_line ? 1 : 2;
            }
            if (!(prog = re_comp(re, o_icase))) { cu_err(10, pat, "bad pattern"); return 0; }
        }
    }
    if (lit) {
        litlen = strlen(lit);
        if (o_icase) { char *p, *c = cu_strdup(lit); if (!c) return 0; for (p = c; *p; p++) *p = lower(*p); lit = c; }
    }
    return 1;
}

/* the first match in s[from..n): 1 and its start and end (from s) */
static int find(const char *s, LONG n, LONG from, LONG *ms, LONG *me)
{
    if (lit) {
        LONG i, last = n - litlen;
        if (!litlen) { *ms = *me = from; return 1; }
        if (o_icase) {
            int f = lit[0];
            for (i = from; i <= last; i++)
                if (lower(s[i]) == f) {
                    int k = 1;
                    while (k < litlen && lower(s[i + k]) == (unsigned char)lit[k]) k++;
                    if (k == litlen) { *ms = i; *me = i + litlen; return 1; }
                }
        } else {
            const char *p = s + from, *end = s + last;
            while (p <= end && (p = memchr(p, lit[0], end - p + 1))) {
                if (!memcmp(p, lit, litlen)) { *ms = p - s; *me = *ms + litlen; return 1; }
                p++;
            }
        }
        return 0;
    } else {
        int caps[2 * RE_GROUPS], r = re_exec(prog, s, n, from, caps);
        if (r < 0) { cu_err(20, NULL, "out of memory"); return 0; }
        if (r) { *ms = caps[2 * group]; *me = caps[2 * group + 1]; }
        return r;
    }
}

/* ---- the lines ---- */

#define C_NAME  "\x1b[35m"
#define C_NUM   "\x1b[32m"
#define C_SEP   "\x1b[36m"
#define C_MATCH "\x1b[1;31m"
#define C_OFF   "\x1b[0m"

static void head(const char *name, LONG no, int sep)
{
    if (o_names) {
        if (colour) cu_puts(C_NAME);
        cu_puts(name);
        if (colour) cu_puts(C_SEP);
        cu_putc(sep);
        if (colour) cu_puts(C_OFF);
    }
    if (o_num) {
        if (colour) cu_puts(C_NUM);
        cu_putnum(no, 0);
        if (colour) cu_puts(C_SEP);
        cu_putc(sep);
        if (colour) cu_puts(C_OFF);
    }
}

/* a matching line, the matches marked when in colour */
static void matched(const char *s, LONG n)
{
    LONG at = 0, from = 0, ms, me;
    if (!colour || o_inv) { cu_put(s, n); cu_putc('\n'); return; }
    while (from <= n && find(s, n, from, &ms, &me)) {
        if (me == ms) { from = ms + 1; if (from > n) break; continue; }
        cu_put(s + at, ms - at);
        cu_puts(C_MATCH); cu_put(s + ms, me - ms); cu_puts(C_OFF);
        at = from = me;
        if (at >= n || (prog && group == 1)) break;
    }
    if (at < n) cu_put(s + at, n - at);
    cu_putc('\n');
}

/* -B: the lines before, kept round */
typedef struct { char *s; LONG n, size, no; } Kept;
static Kept *ring;
static int rhead, rcount;

static void keep(const char *s, LONG n, LONG no)
{
    Kept *k;
    if (!o_before || !ring) return;
    k = &ring[(rhead + rcount) % o_before];
    if (rcount == o_before) rhead = (rhead + 1) % o_before; else rcount++;
    if (k->size < n + 1) {
        cu_free(k->s);
        k->size = n + 64;
        if (!(k->s = cu_alloc(k->size))) { k->size = 0; k->n = -1; return; }
    }
    memcpy(k->s, s, n); k->n = n; k->no = no;
}

static int grepfile(const char *path, void *u)
{
    CuIn in;
    char *line;
    LONG n, no = 0, count = 0, after = 0, lastout = 0, ms, me;
    int nl;
    const char *name = path ? path : "(standard input)";
    (void)u;
    if (path && o_rec) {
        BPTR l = Lock((STRPTR)path, SHARED_LOCK);
        struct FileInfoBlock *fib;
        if (l && (fib = AllocDosObject(DOS_FIB, NULL))) {
            int dir = Examine(l, fib) && fib->fib_DirEntryType > 0 && fib->fib_DirEntryType != ST_SOFTLINK;
            FreeDosObject(DOS_FIB, fib);
            if (dir) {
                /* into the drawer: its entries, then each in turn (files first) */
                char sub[CU_PATH];
                struct FileInfoBlock *f2 = AllocDosObject(DOS_FIB, NULL);
                char **names = NULL;
                int nn = 0, max = 0, i;
                if (f2 && Examine(l, f2)) {
                    while (ExNext(l, f2)) {
                        if (f2->fib_DirEntryType == ST_SOFTLINK || f2->fib_DirEntryType == ST_LINKDIR) continue;
                        if (nn == max) {
                            char **t = cu_alloc((max = max ? max * 2 : 32) * sizeof *t);
                            if (!t) break;
                            if (names) { memcpy(t, names, nn * sizeof *t); cu_free(names); }
                            names = t;
                        }
                        if (!cu_join(sub, path, f2->fib_FileName) || !(names[nn] = cu_strdup(sub))) break;
                        nn++;
                    }
                }
                if (f2) FreeDosObject(DOS_FIB, f2);
                UnLock(l);
                if (cu_stackleft() < 1500) { cu_err(10, path, "too deep for the stack (Stack 16000 helps)"); nn = 0; }
                for (i = 0; i < nn; i++) {
                    if (cu_rc < 20 && !cu_break() && !(o_quiet && found)) grepfile(names[i], NULL);
                    cu_free(names[i]);
                }
                cu_free(names);
                return cu_rc >= 20;
            }
        }
        if (l) UnLock(l);
    }
    if (!cu_open(&in, path)) { if (cu_rc < 10) cu_rc = 10; return 0; }
    files_seen++;
    rcount = rhead = 0;
    while ((n = cu_line(&in, &line, &nl)) >= 0) {
        int hit;
        no++;
        if (!(no & 255) && cu_break()) break;
        hit = find(line, n, 0, &ms, &me) != o_inv;
        if (cu_rc >= 20) break;
        if (!hit) {
            if (after > 0 && !o_count && !o_list && !o_nlist && !o_quiet) {
                head(name, no, '-'); cu_put(line, n); cu_putc('\n'); after--; lastout = no;
            } else keep(line, n, no);
            continue;
        }
        count++;
        found = 1;
        if (o_quiet) break;
        if (o_list || o_nlist) break;
        if (o_count) { if (o_max >= 0 && count >= o_max) break; continue; }
        if (o_before || o_after) {
            /* a -- between groups that do not touch */
            LONG first = rcount ? ring[rhead].no : no;
            if (lastout && first > lastout + 1) { if (colour) cu_puts(C_SEP); cu_puts("--\n"); if (colour) cu_puts(C_OFF); }
            while (rcount) {
                Kept *k = &ring[rhead];
                rhead = (rhead + 1) % o_before; rcount--;
                if (k->n < 0) continue;
                head(name, k->no, '-'); cu_put(k->s, k->n); cu_putc('\n');
            }
            after = o_after;
        }
        if (o_only && !o_inv) {
            LONG at = 0;
            while (at <= n && find(line, n, at, &ms, &me)) {
                if (me > ms) {
                    head(name, no, ':');
                    if (colour) cu_puts(C_MATCH);
                    cu_put(line + ms, me - ms);
                    if (colour) cu_puts(C_OFF);
                    cu_putc('\n');
                }
                at = me > ms ? me : ms + 1;
                if (at >= n || (prog && group == 1)) break;
            }
        } else {
            head(name, no, ':');
            matched(line, n);
        }
        lastout = no;
        if (o_max >= 0 && count >= o_max) break;
    }
    if (in.err) cu_fault(10, name);
    cu_close(&in);
    if (o_count && !o_quiet) {
        if (o_names) { if (colour) cu_puts(C_NAME); cu_puts(name); if (colour) cu_puts(C_SEP); cu_putc(':'); if (colour) cu_puts(C_OFF); }
        cu_putnum(count, 0); cu_putc('\n');
    }
    if (!o_quiet && ((o_list && count) || (o_nlist && !count))) {
        if (colour) cu_puts(C_NAME);
        cu_puts(name);
        if (colour) cu_puts(C_OFF);
        cu_putc('\n');
    }
    return cu_rc >= 20 || (o_quiet && found);
}

int main(void)
{
    char *v, *pat = NULL;
    int c, i, ok, names = -1, col = -1;
    (void)version;
    if (cu_args("grep [-EFivwxcLlnhHoqsr] [-e PAT] [-m N] [-A N] [-B N] [-C N] PATTERN [file ...]") < 0) return cu_rc;
    /* --colour=..., before the short flags */
    for (i = 0; i < cu_argc; i++) {
        v = cu_argv[i];
        if (v[0] == '-' && v[1] == '-' && v[2]) {
            if (!strncmp(v, "--colour", 8) || !strncmp(v, "--color", 7)) {
                char *e = strchr(v, '=');
                col = !e || !strcmp(e + 1, "always") ? 1 : !strcmp(e + 1, "never") ? 0 : -1;
                memmove(&cu_argv[i], &cu_argv[i + 1], (cu_argc - i) * sizeof *cu_argv);
                cu_argc--; i--;
            } else if (!v[2]) break;
        }
    }
    while ((c = cu_opt("EFivwxcLlnhHoqsrRe:m:A:B:C:", &v)) >= 0) switch (c) {
        case 'E': o_ext = 1; break;
        case 'F': o_fixed = 1; break;
        case 'i': o_icase = 1; break;
        case 'v': o_inv = 1; break;
        case 'w': o_word = 1; break;
        case 'x': o_line = 1; break;
        case 'c': o_count = 1; break;
        case 'l': o_list = 1; o_nlist = 0; break;
        case 'L': o_nlist = 1; o_list = 0; break;
        case 'n': o_num = 1; break;
        case 'h': names = 0; break;
        case 'H': names = 1; break;
        case 'o': o_only = 1; break;
        case 'q': o_quiet = 1; break;
        case 's': cu_silent = 1; break;
        case 'r': case 'R': o_rec = 1; break;
        case 'e': pat = v; break;
        case 'm': case 'A': case 'B': case 'C': {
            LONG k = cu_num(v, &ok);
            if (!ok || k < 0) { cu_err(10, v, "not a count"); return cu_rc; }
            if (c == 'm') o_max = k;
            if (c == 'A' || c == 'C') o_after = k;
            if (c == 'B' || c == 'C') o_before = k;
            break;
        }
        default: return cu_rc;
    }
    if (!pat) {
        if (!cu_argc) { cu_err(10, NULL, "no pattern (grep ? for help)"); return cu_rc; }
        pat = cu_argv[0]; cu_argv++; cu_argc--;
    }
    if (!setup(pat)) return cu_rc ? cu_rc : 20;
    if (o_before && !(ring = cu_alloc(o_before * sizeof *ring))) { cu_err(20, NULL, "out of memory"); return cu_rc; }
    if (ring) memset(ring, 0, o_before * sizeof *ring);
    o_names = names >= 0 ? names : cu_argc > 1 || o_rec || (cu_argc == 1 && cu_haswild(cu_argv[0]));
    colour = col >= 0 ? col : IsInteractive(Output());
    if (!cu_argc) grepfile(NULL, NULL);
    for (i = 0; i < cu_argc && cu_rc < 20 && !(o_quiet && found); i++) cu_each(cu_argv[i], grepfile, NULL);
    cu_flush();
    if (cu_rc) return cu_rc < 10 && found ? 0 : cu_rc;
    return found ? 0 : 5;
}
