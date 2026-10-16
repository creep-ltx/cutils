/* xargs - commands built from the words of the input.
 *
 *   xargs [-0rt] [-n N] [-L N] [-I R] [-s N] [command [argument ...]]
 *
 * The input's words go after the command (Echo when none), as many to a
 * command line as fit, and each line is run, one after the other:
 * `find . -name *.bak | xargs rm`. Words are split at blanks and line
 * ends; '...' and "..." keep blanks in a word, \ takes the next character
 * as it is (as GNU xargs).
 *   -n N at most N words a command   -L N the words of at most N lines a
 *   command   -I R one command per input line, R in the arguments
 *   replaced by the line (blanks at its start left out)   -s N command
 *   lines at most N bytes (512 unless told: the Shell's own line)
 *   -t each command shown (on the error stream) before it runs   -0 words
 *   end at 0 bytes only (find -print0), quotes are plain   -r accepted:
 *   with no words at all nothing is run - always (GNU runs the command
 *   once; BSD and this one do not, so `rm` never runs with no names).
 * Each word is passed as the Shell reads it: in "..." when it has a blank
 * or a ; < > | " in it (or is empty), " as *" and * as ** inside. The
 * commands run through the Shell (System()), their input NIL:, their
 * output this one's, so `| xargs grep x | wc -l` works.
 *
 * Return code the worst of the commands' (0 when all were fine), 10 bad
 * arguments, a word longer than a command line, or a command that could
 * not be started (then it stops), 20 Ctrl-C. */
#include "cu.h"
#include <dos/dostags.h>

const char *cu_name = "xargs";
static const char version[] = "$VER: xargs 0.1b2 (16.10.2026)";

static int o_zero, o_trace;
static LONG o_n, o_lines, o_size = 512;
static char *o_repl;
static CuIn in;
static int worst, stop;

/* ---- the input's words ---- */

static char *word;
static LONG wsize;
static int wnl;                  /* the word ended a line */

static int grow(LONG k)
{
    if (k < wsize) return 1;
    {
        LONG ns = wsize ? wsize * 2 : 256;
        char *t;
        while (ns <= k) ns *= 2;
        if (!(t = cu_alloc(ns))) { cu_err(20, NULL, "out of memory"); return 0; }
        if (word) { memcpy(t, word, wsize); cu_free(word); }
        word = t; wsize = ns;
    }
    return 1;
}

static int getch(void)
{
    char c;
    return cu_read(&in, &c, 1) == 1 ? (unsigned char)c : -1;
}

/* the next word: its length, -1 at the end. lines: -I (a whole line a word) */
static LONG next(int lines)
{
    LONG k = 0;
    int c, q = 0, any = 0;
    wnl = 0;
    if (o_zero) {
        while ((c = getch()) >= 0 && c != 0) { if (!grow(k + 1)) return -1; word[k++] = c; any = 1; }
        if (c < 0 && !any) return -1;
        word[k] = 0;
        wnl = 1;
        return k;
    }
    for (;;) {
        c = getch();
        if (c < 0) {
            if (q) { cu_err(10, NULL, "a quote is not closed"); stop = 1; return -1; }
            if (!any) return -1;
            wnl = 1;
            break;
        }
        if (q) {
            if (c == q) { q = 0; continue; }
            if (c == '\n') { cu_err(10, NULL, "a quote is not closed"); stop = 1; return -1; }
        } else {
            if (c == '\n') { if (any) { wnl = 1; break; } continue; }
            if (c == ' ' || c == '\t') {
                if (lines) { if (!any) continue; }
                else if (any) break;
                else continue;
            }
            if (c == '\'' || c == '"') { q = c; any = 1; continue; }
            if (c == '\\') { if ((c = getch()) < 0) break; }
        }
        if (!grow(k + 1)) return -1;
        word[k++] = c;
        any = 1;
    }
    if (lines) while (k && (word[k - 1] == ' ' || word[k - 1] == '\t')) k--;   /* blanks at the end of an -I line */
    word[k] = 0;
    return k;
}

/* ---- the command line ---- */

static char *cmd;
static LONG clen, csize;

static int needq(const char *s)
{
    if (!*s) return 1;
    for (; *s; s++) if (strchr(" \t;<>|\"\n", *s)) return 1;
    return 0;
}

static LONG qlen(const char *s)       /* its length as it goes on the line */
{
    LONG n = 0;
    if (!needq(s)) return strlen(s);
    for (; *s; s++) n += (*s == '"' || *s == '*' || *s == '\n' || *s == 0x1b) ? 2 : 1;
    return n + 2;
}

static int append(const char *s)
{
    LONG n = qlen(s) + 1;
    char *d;
    if (clen + n + 1 > csize) {
        LONG ns = (clen + n + 1) * 2;
        char *t = cu_alloc(ns);
        if (!t) { cu_err(20, NULL, "out of memory"); return 0; }
        if (cmd) memcpy(t, cmd, clen);
        cu_free(cmd);
        cmd = t; csize = ns;
    }
    d = cmd + clen;
    if (clen) *d++ = ' ';
    if (!needq(s)) { strcpy(d, s); d += strlen(s); }
    else {
        *d++ = '"';
        for (; *s; s++) {
            if (*s == '"' || *s == '*') { *d++ = '*'; *d++ = *s; }
            else if (*s == '\n') { *d++ = '*'; *d++ = 'n'; }
            else if (*s == 0x1b) { *d++ = '*'; *d++ = 'e'; }
            else *d++ = *s;
        }
        *d++ = '"';
    }
    *d = 0;
    clen = d - cmd;
    return 1;
}

static BPTR errfh(void)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    return me->pr_CES ? me->pr_CES : Output();
}

static void run(void)
{
    LONG rc;
    BPTR nil;
    cu_flush();
    if (o_trace) { BPTR e = errfh(); Write(e, cmd, clen); Write(e, "\n", 1); }
    if (!(nil = Open("NIL:", MODE_OLDFILE))) { cu_fault(10, "NIL:"); stop = 1; return; }
    rc = SystemTags(cmd, SYS_Input, nil, SYS_Output, Output(), TAG_DONE);
    Close(nil);
    if (rc == -1) { cu_fault(10, cmd); stop = 1; return; }
    if (rc > worst) worst = rc;
    if (cu_break()) stop = 1;
}

/* the fixed part: the command and its arguments */
static int fixed(void)
{
    int i;
    clen = 0;
    if (!cu_argc) return append("Echo");
    for (i = 0; i < cu_argc; i++) if (!append(cu_argv[i])) return 0;
    return 1;
}

/* -I: R in each argument replaced by the line */
static int replaced(const char *line)
{
    int i;
    LONG rl = strlen(o_repl), ll = strlen(line);
    clen = 0;
    for (i = 0; i < cu_argc; i++) {
        const char *a = cu_argv[i], *p;
        LONG cnt = 0, n;
        char *t, *d;
        for (p = a; rl && (p = strstr(p, o_repl)); p += rl) cnt++;
        if (!(t = cu_alloc(strlen(a) + cnt * ll + 1))) { cu_err(20, NULL, "out of memory"); return 0; }
        for (d = t, p = a; *p; ) {
            if (rl && !strncmp(p, o_repl, rl)) { memcpy(d, line, ll); d += ll; p += rl; }
            else *d++ = *p++;
        }
        *d = 0;
        n = append(t);
        cu_free(t);
        if (!n) return 0;
    }
    return 1;
}

int main(void)
{
    char *v;
    int c, ok;
    (void)version;
    if (cu_args("xargs [-0rt] [-n N] [-L N] [-I R] [-s N] [command [argument ...]]") < 0) return cu_rc;
    while ((c = cu_opt("0rtxn:L:l:I:i:s:", &v)) >= 0) switch (c) {
        case '0': o_zero = 1; break;
        case 'r': case 'x': break;
        case 't': o_trace = 1; break;
        case 'n': case 'L': case 'l': case 's':
            {
                LONG x = cu_num(v, &ok);
                if (!ok || x < 1) { cu_err(10, v, "not a count"); return cu_rc; }
                if (c == 'n') { o_n = x; o_lines = 0; }
                else if (c == 's') o_size = x;
                else { o_lines = x; o_n = 0; }
            }
            break;
        case 'I': case 'i': o_repl = v; break;
        default: return cu_rc;
    }
    if (o_repl && !cu_argc) { cu_err(10, NULL, "-I needs a command"); return cu_rc; }
    if (!cu_open(&in, NULL)) return cu_rc;
    if (o_repl) {
        LONG n;
        while (!stop && (n = next(1)) >= 0) {
            if (!n && !o_zero) continue;
            if (!replaced(word)) break;
            if (clen > o_size) { cu_err(10, word, "the command line would be too long"); break; }
            run();
        }
    } else {
        LONG base, n, words = 0, lines = 0;
        if (!fixed()) goto end;
        base = clen;
        if (base > o_size) { cu_err(10, NULL, "the command is longer than a command line"); goto end; }
        while (!stop && (n = next(0)) >= 0) {
            if (clen > base && clen + 1 + qlen(word) > o_size) {   /* full: run, start again */
                run();
                if (stop) break;
                clen = base; cmd[clen] = 0; words = 0; lines = 0;
            }
            if (base + 1 + qlen(word) > o_size) { cu_err(10, word, "longer than a command line"); stop = 1; break; }
            if (!append(word)) break;
            words++;
            if (wnl) lines++;
            if ((o_n && words >= o_n) || (o_lines && wnl && lines >= o_lines)) {
                run();
                clen = base; cmd[clen] = 0; words = 0; lines = 0;
            }
        }
        if (!stop && clen > base) run();
    }
end:
    cu_close(&in);
    cu_flush();
    if (worst > cu_rc) cu_rc = worst;
    return cu_rc;
}
