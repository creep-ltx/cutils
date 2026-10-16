/* cu.c - the parts every CUtils command shares (cu.h says what each does). */
#include <stdlib.h>
#include "cu.h"

/* Every command runs on a stack of at least 32 KB: libnix's startup
 * (swapstack.o, linked by -u ___stkinit in the Makefile) swaps to one of
 * that size when the Shell gave less. The Shell's default is 4 KB, and
 * some frames alone are bigger (miniz's huffman table 5.5 KB, bzip2's
 * code lengths 5.2 KB, tail -f 4.1 KB, man's draw chain 6 KB): on 4 KB
 * they wrote below the stack, and tar czf reset the machine. A bigger
 * stack set with Stack is kept. */
unsigned long __stack = 32768;

int cu_rc;
int cu_silent;
BPTR cu_out;
int cu_argc;
char **cu_argv;

void *cu_alloc(ULONG n) { return malloc(n ? n : 1); }   /* libnix frees what is left at the end */
void cu_free(void *p) { if (p) free(p); }

char *cu_strdup(const char *s)
{
    char *d = cu_alloc(strlen(s) + 1);
    if (d) strcpy(d, s);
    return d;
}

/* ---- output, gathered ---- */

static char obuf[4096];
static int olen;

void cu_flush(void)
{
    if (olen) {
        if (!cu_out) cu_out = Output();
        Write(cu_out, obuf, olen);
    }
    olen = 0;
}

void cu_put(const char *s, LONG n)
{
    if (n >= (LONG)sizeof obuf) {          /* big blocks (cat) go straight out */
        cu_flush();
        if (!cu_out) cu_out = Output();
        Write(cu_out, (APTR)s, n);
        return;
    }
    while (n > 0) {
        int k = (int)sizeof obuf - olen;
        if (k > n) k = n;
        memcpy(obuf + olen, s, k);
        olen += k; s += k; n -= k;
        if (olen == (int)sizeof obuf) cu_flush();
    }
}

void cu_puts(const char *s) { cu_put(s, strlen(s)); }

void cu_putc(int c)
{
    if (olen == (int)sizeof obuf) cu_flush();
    obuf[olen++] = c;
}

void cu_putnum(LONG v, int width)
{
    char t[12], b[24];
    int i = 0, n = 0, neg = v < 0;
    ULONG u = neg ? -(ULONG)v : (ULONG)v;
    do { t[i++] = '0' + u % 10; u /= 10; } while (u);
    if (neg) t[i++] = '-';
    while (i + n < width && n < 11) b[n++] = ' ';
    while (i) b[n++] = t[--i];
    cu_put(b, n);
}

/* ---- messages ---- */

/* noinline: inlined into errput, bebbo's gcc (-Os) kept SysBase in a6 from
 * FindTask() and called Write() on it whenever pr_CES was set, so every
 * message to a redirected error stream (*> in the Shell, 2> in csh) was
 * lost (found by wget -o, 2026-10-15; seen in objdump of errput) */
static BPTR __attribute__((noinline)) errout(void)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    return me->pr_CES ? me->pr_CES : Output();
}

static void errput(const char *s)
{
    Write(errout(), (APTR)s, strlen(s));
}

void cu_err(int rc, const char *what, const char *why)
{
    if (cu_rc < rc) cu_rc = rc;
    if (cu_silent) return;
    cu_flush();
    errput(cu_name);
    if (what) { errput(": "); errput(*what ? what : "\"\""); }
    if (why) { errput(": "); errput(why); }
    errput("\n");
    if (cu_rc < rc) cu_rc = rc;
}

void cu_fault(int rc, const char *what)
{
    char why[100];
    LONG e = IoErr();
    if (!e || !Fault(e, NULL, why, sizeof why)) strcpy(why, "failed");
    cu_err(rc, what, why);
}

int cu_break(void)
{
    if (cu_rc == 20) return 1;
    if (SetSignal(0, SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C) {
        cu_flush();
        errput("***Break\n");
        cu_rc = 20;
        return 1;
    }
    return 0;
}

/* ---- the command line ---- */

#define MAXWORDS 256

static int optind_ = 0, optpos;

/* the words, from GetArgStr(): "..." with *" *n *e ** as the Shell has them */
int cu_args(const char *usage)
{
    static char *words[MAXWORDS + 1];
    char *line = GetArgStr(), *p, *d;
    int n = 0;
    cu_out = Output();
    if (!line) line = "";
    if (!(p = cu_strdup(line))) return -1;
    while (n < MAXWORDS) {
        while (*p == ' ' || *p == '\t') p++;
        if (!*p || *p == '\n') break;
        d = words[n++] = p;
        if (*p == '"') {
            for (p++; *p && *p != '"' && *p != '\n'; p++) {
                if (*p == '*' && p[1]) {
                    p++;
                    if (*p == 'n' || *p == 'N') { *d++ = '\n'; continue; }
                    if (*p == 'e' || *p == 'E') { *d++ = 0x1b; continue; }
                }
                *d++ = *p;
            }
            if (*p) p++;
        } else {
            while (*p && *p != ' ' && *p != '\t' && *p != '\n') *d++ = *p++;
        }
        if (*p) p++;
        *d = 0;
    }
    if (n == MAXWORDS) { cu_err(10, NULL, "too many arguments"); return -1; }
    words[n] = NULL;
    if (n == 1 && words[0][0] == '?' && !words[0][1] && usage) {
        PutStr((STRPTR)usage);
        PutStr("\n");
        return -1;
    }
    cu_argc = n; cu_argv = words;
    optind_ = 0; optpos = 0;
    return n;
}

int cu_opt(const char *spec, char **val)
{
    char *w;
    const char *s;
    int c;
    for (;;) {
        if (optind_ >= cu_argc) goto done;
        w = cu_argv[optind_];
        if (!optpos) {
            if (w[0] != '-' || !w[1]) goto done;           /* a word, or - alone */
            if (w[1] == '-' && !w[2]) { optind_++; goto done; }
            if (w[1] >= '0' && w[1] <= '9' && strchr(spec, '#')) {
                *val = w + 1; optind_++;
                return '#';
            }
            optpos = 1;
        }
        c = w[optpos++];
        if (!c) { optind_++; optpos = 0; continue; }
        s = strchr(spec, c);
        if (!s || c == ':' || c == '#') {
            char t[3] = { '-', c, 0 };
            cu_err(10, t, "unknown option");
            return '?';
        }
        if (s[1] == ':') {
            if (w[optpos]) *val = w + optpos;
            else if (optind_ + 1 < cu_argc) *val = cu_argv[++optind_];
            else {
                char t[3] = { '-', c, 0 };
                cu_err(10, t, "needs a value");
                return '?';
            }
            optind_++; optpos = 0;
        }
        return c;
    }
done:
    cu_argv += optind_;
    cu_argc -= optind_;
    optind_ = 0; optpos = 0;
    return -1;
}

long cu_num(const char *s, int *ok)
{
    long v = 0;
    int neg = 0, any = 0;
    if (*s == '-') neg = 1, s++;
    else if (*s == '+') s++;
    while (*s >= '0' && *s <= '9') v = v * 10 + (*s++ - '0'), any = 1;
    if (*s == 'k' || *s == 'K') v <<= 10, s++;
    else if (*s == 'm' || *s == 'M') v <<= 20, s++;
    else if (*s == 'g' || *s == 'G') v <<= 30, s++;
    if (ok) *ok = any && !*s;
    return neg ? -v : v;
}

/* ---- paths ---- */

int cu_join(char *buf, const char *dir, const char *name)
{
    int n = strlen(dir);
    if (n + strlen(name) + 2 > CU_PATH) return 0;
    if (buf != dir) strcpy(buf, dir);
    if (n && buf[n - 1] != ':' && buf[n - 1] != '/') buf[n++] = '/';
    strcpy(buf + n, name);
    return 1;
}

int cu_termwidth(BPTR in, BPTR out)
{
    UBYTE b[64];
    int n = 0, p[4] = { 0, 0, 0, 0 }, np = 0, i, w = 0;
    if (!in || !out || !IsInteractive(in)) return 0;
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
    return w >= 10 && w < 1000 ? w : 0;
}

long cu_stackleft(void)
{
    char here;
    return &here - (char *)FindTask(NULL)->tc_SPLower;
}

