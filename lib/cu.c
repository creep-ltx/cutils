/* cu.c - the parts every CUtils command shares (cu.h says what each does). */
#include <stdlib.h>
#include "cu.h"

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

static BPTR errout(void)
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

long cu_stackleft(void)
{
    char here;
    return &here - (char *)FindTask(NULL)->tc_SPLower;
}

