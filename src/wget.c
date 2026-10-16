/* wget - fetch files from the web: http:// and https://.
 *
 *   wget [-cqSV] [-nv] [-nc] [-O FILE] [-P DIR] [-t N] [-T SECS] [-U AGENT]
 *        [--header "Name: value"] [-i FILE] [-o LOGFILE] [--spider]
 *        [--content-disposition] [--no-check-certificate] URL ...
 *
 * Each URL is fetched into a file in the current drawer, named after the
 * URL's last part (%xx decoded; "index.html" when it ends in /). When that
 * name is taken, wget's rule: name.1, name.2 ... (not with -O, -c, -nc).
 * HTTP/1.1 with Host, Connection: close; Content-Length or chunked bodies;
 * redirects (301 302 303 307 308, up to 20, also to another host or from
 * http to https, relative ones too); user:password@ in the URL is sent as
 * Basic authorisation. A URL without http:// is taken as http.
 *   -O FILE  everything into FILE (- = the output)   -P DIR  files into DIR
 *   (made when missing)   -c  go on with a partial file (Range: bytes=N-);
 *   a server that cannot resume sends it all again, a file already whole is
 *   left alone   -nc  never fetch a file whose name is already there
 *   -q  nothing said at all   -nv  one line per file and the errors
 *   -S  the server's headers   -t N  tries (3; 0 = no limit): a lost
 *   connection, a timeout and a 5xx are tried again, going on from where
 *   it stopped; a 4xx never   -T SECS  how long a connection may be silent
 *   (30; 0 = for ever)   -U AGENT  the User-Agent ("wget/0.1b2 (AmigaOS;
 *   CUtils)")   --header H  an extra header line, as often as wanted
 *   (Host or User-Agent given this way replace wget's own)   -i FILE  the
 *   URLs from FILE, one a line (- = the input; # starts a comment)
 *   -o LOGFILE  the messages into LOGFILE   --spider  only ask whether it
 *   is there (HEAD), nothing saved   --content-disposition  the name the
 *   server suggests   --no-check-certificate  https without checking
 *   --ca-certificate=FILE  --ca-directory=DIR  other certificates to trust
 *   than AmiSSL's   -V  the version
 * Long names work too: --output-document= --directory-prefix= --continue
 * --quiet --no-verbose --no-clobber --tries= --timeout= --user-agent=
 * --server-response --input-file= --output-file=.
 * Options may come after the URLs too (as GNU wget takes them). Rare wget
 * options (-r -m -N -k -p, cookies, --post-data ...) answer "not supported".
 *
 * https needs AmiSSL 5, opened only when an https address comes up (plain
 * http works without it). Certificates are checked against AmiSSL's store
 * (AmiSSL:Certs, AmiSSL:UserCerts), with the host's name; SNI is sent.
 *
 * Progress, when the messages go to a window: one line redrawn in place,
 * as wide as the window ($COLUMNS, else the console's own width): name,
 * percent, a bar, bytes, speed and time left (a moving marker when the
 * size is not known). Otherwise a line per MB. Messages on the error
 * stream. The file gets the server's Last-Modified date.
 *
 * Ctrl-C stops at once and keeps the partial file, so wget -c goes on.
 *
 * Return code 0 fine, 5 --spider: not there (4xx), 10 a failure (no such
 * host, refused, 404, certificate, write error, bad arguments; the worst
 * of several URLs), 20 Ctrl-C. */
#include "cu.h"
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <proto/bsdsocket.h>
#include <exec/io.h>
#include <devices/conunit.h>
#include <dos/var.h>
#include "wget.h"

const char *cu_name = "wget";
static const char version[] = "$VER: wget 0.1b2 (16.10.2026)";

#define AGENT "wget/0.1b2 (AmigaOS; CUtils)"
#define FIONBIO_ 0x8004667EUL      /* _IOW('f', 126, long): not in the headers */
#define IBUF 16384                 /* recv buffer */
#define OBUF 32768                 /* writes gathered */
#define HBUF 16384                 /* the response's header block */

typedef unsigned long long U64;

struct Library *SocketBase;

/* ---- options ---- */
static char *o_out, *o_dir, *o_agent = AGENT, *o_input, *o_log, *o_cafile, *o_cadir;
static int o_cont, o_quiet, o_nv, o_tries = 3, o_timeout = 30, o_show, o_spider,
           o_nocheck, o_cd, o_nc;
static char *o_hdr[32];
static int o_nhdr;

/* ---- messages ---- */
static BPTR errfh;
static int info_ok;                /* the long messages (not -q, not -nv) */
static char mb[600];
static int ml;

static void ms(const char *s) { while (*s && ml < (int)sizeof mb - 1) mb[ml++] = *s++; }
static void mn(U64 v)
{
    char t[24];
    int i = 0;
    do { t[i++] = '0' + (int)(v % 10); v /= 10; } while (v);
    while (i && ml < (int)sizeof mb - 1) mb[ml++] = t[--i];
}
static void mout(void)            /* the gathered message, on the error stream */
{
    if (ml) { cu_flush(); Write(errfh, mb, ml); }
    ml = 0;
}
static void info(const char *s) { if (info_ok) { ms(s); ms("\n"); mout(); } }

/* 12, 1023, 1.5K, 12.3K, 123K, 1.2M ... into b */
static void human(char *b, U64 v)
{
    static const char units[] = "KMGT";
    int u = -1;
    U64 t = v * 10;
    char *o = b;
    if (v < 1024) { int i = 0; char r[8]; do { r[i++] = '0' + v % 10; v /= 10; } while (v); while (i) *o++ = r[--i]; *o = 0; return; }
    while (t >= 10240 && u < 3) { t /= 1024; u++; }
    if (t < 1000) {                /* one decimal */
        U64 w = t / 10;
        if (w >= 10) *o++ = '0' + (int)(w / 10);
        *o++ = '0' + (int)(w % 10);
        *o++ = '.'; *o++ = '0' + (int)(t % 10);
    } else {
        U64 w = t / 10;
        char r[8]; int i = 0;
        do { r[i++] = '0' + (int)(w % 10); w /= 10; } while (w);
        while (i) *o++ = r[--i];
    }
    *o++ = units[u];
    *o = 0;
}

static int lc(int c) { return c >= 'A' && c <= 'Z' ? c + 32 : c; }
static int ncase(const char *a, const char *b, int n)   /* 0: the same, case blind */
{
    while (n-- > 0) { if (lc(*a) != lc(*b)) return 1; if (!*a) return 0; a++; b++; }
    return 0;
}

/* the time now, for messages: "2026-10-15 12:00:00" */
static void stamp(void)
{
    struct DateStamp d;
    char b[24];
    DateStamp(&d);
    cu_dsfix(&d);
    cu_isodate(&d, b);
    ms(b);
}

/* ---- URLs ---- */

typedef struct {
    int tls, port;
    char host[256];
    char auth[256];                /* "user:password" in base64, "" none */
    char *path;                    /* "/..." */
    char *full;                    /* the whole URL, for messages */
} Url;

static void b64(char *d, const char *s, int n)
{
    static const char t[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    const UBYTE *p = (const UBYTE *)s;
    while (n > 0) {
        ULONG v = p[0] << 16 | (n > 1 ? p[1] << 8 : 0) | (n > 2 ? p[2] : 0);
        *d++ = t[v >> 18]; *d++ = t[(v >> 12) & 63];
        *d++ = n > 1 ? t[(v >> 6) & 63] : '=';
        *d++ = n > 2 ? t[v & 63] : '=';
        p += 3; n -= 3;
    }
    *d = 0;
}

static int hexv(int c)
{
    return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
}

static void decode(char *s)       /* %xx in place */
{
    char *d = s;
    for (; *s; s++) {
        if (*s == '%' && hexv(s[1]) >= 0 && hexv(s[2]) >= 0) { *d++ = hexv(s[1]) * 16 + hexv(s[2]); s += 2; }
        else *d++ = *s;
    }
    *d = 0;
}

/* 1: parsed; 0: said why (rc 10) */
static int url_parse(Url *u, const char *s)
{
    const char *p = s, *e, *at, *c;
    int n;
    cu_free(u->path); cu_free(u->full);
    memset(u, 0, sizeof *u);
    while (*p == ' ' || *p == '\t') p++;
    if (!ncase(p, "http://", 7)) p += 7;
    else if (!ncase(p, "https://", 8)) { p += 8; u->tls = 1; }
    else if (strstr(p, "://")) {
        cu_err(10, s, "only http:// and https:// are supported");
        return 0;
    }
    u->port = u->tls ? 443 : 80;
    for (e = p; *e && *e != '/' && *e != '?' && *e != '#'; e++) ;
    at = NULL;
    for (c = p; c < e; c++) if (*c == '@') at = c;
    if (at) {
        char tmp[190];
        n = at - p;
        if (n > (int)sizeof tmp - 1) n = sizeof tmp - 1;
        memcpy(tmp, p, n); tmp[n] = 0;
        decode(tmp);
        b64(u->auth, tmp, strlen(tmp));
        p = at + 1;
    }
    for (c = p; c < e && *c != ':'; c++) ;
    n = c - p;
    if (n <= 0 || n >= (int)sizeof u->host) { cu_err(10, s, "no host in the address"); return 0; }
    memcpy(u->host, p, n); u->host[n] = 0;
    if (c < e) {
        LONG v = 0;
        for (c++; c < e && *c >= '0' && *c <= '9'; c++) v = v * 10 + *c - '0';
        if (c != e || v <= 0 || v > 65535) { cu_err(10, s, "bad port"); return 0; }
        u->port = v;
    }
    /* the path, without #fragment, spaces and 8-bit bytes %-encoded */
    {
        const char *q = e;
        char *d;
        int len = 0;
        for (q = e; *q && *q != '#'; q++) len += (UBYTE)*q <= ' ' || (UBYTE)*q >= 0x7f ? 3 : 1;
        if (!(u->path = d = cu_alloc(len + 2))) return 0;
        if (*e != '/') *d++ = '/';
        for (q = e; *q && *q != '#'; q++) {
            UBYTE b = *q;
            if (b <= ' ' || b >= 0x7f) { *d++ = '%'; *d++ = "0123456789ABCDEF"[b >> 4]; *d++ = "0123456789ABCDEF"[b & 15]; }
            else *d++ = b;
        }
        *d = 0;
    }
    if (!(u->full = cu_alloc(strlen(u->host) + strlen(u->path) + 20))) return 0;
    strcpy(u->full, u->tls ? "https://" : "http://");
    strcat(u->full, u->host);
    if (u->port != (u->tls ? 443 : 80)) {
        char *o = u->full + strlen(u->full), r[8];
        int i = 0, v = u->port;
        *o++ = ':';
        do { r[i++] = '0' + v % 10; v /= 10; } while (v);
        while (i) *o++ = r[--i];
        *o = 0;
    }
    strcat(u->full, u->path);
    return 1;
}

/* Location against the address it came from: a new absolute URL (heap) */
static char *resolve(const Url *b, const char *loc)
{
    int sl = strlen(b->full), ll = strlen(loc);
    char *r = cu_alloc(sl + ll + 16), *p;
    if (!r) return NULL;
    if (strstr(loc, "://")) { strcpy(r, loc); return r; }
    if (loc[0] == '/' && loc[1] == '/') { strcpy(r, b->tls ? "https:" : "http:"); strcat(r, loc); return r; }
    strcpy(r, b->full);
    p = strstr(r, "://") + 3;
    p = strchr(p, '/');            /* the path starts here */
    if (!p) p = r + strlen(r);
    if (loc[0] == '/') *p = 0;
    else {
        char *q = strchr(p, '?');
        if (q) *q = 0;
        if (loc[0] != '?') {
            q = strrchr(p, '/');
            if (q) q[1] = 0;
        }
    }
    strcat(r, loc);
    return r;
}

/* a name for a file: after the last / \ :, no control characters */
static void tidyname(char *s)
{
    char *p, *b = s;
    for (p = s; *p; p++) if (*p == '/' || *p == '\\' || *p == ':') b = p + 1;
    if (b != s) memmove(s, b, strlen(b) + 1);
    for (p = s; *p; p++) if ((UBYTE)*p < ' ' || *p == 0x7f) *p = '_';
    if (!strcmp(s, ".") || !strcmp(s, "..")) *s = 0;
}

/* the file name a URL gives (heap) */
static char *urlname(const Url *u)
{
    const char *p = u->path, *e = strchr(p, '?'), *s;
    char *n;
    int len;
    if (!e) e = p + strlen(p);
    for (s = e; s > p && s[-1] != '/'; s--) ;
    len = e - s;
    if (!(n = cu_alloc(len + 11))) return NULL;
    memcpy(n, s, len); n[len] = 0;
    decode(n);
    tidyname(n);
    if (!*n) strcpy(n, "index.html");
    return n;
}

/* -P DIR joined in front (heap) */
static char *withdir(const char *name)
{
    char *r;
    if (!o_dir || !*o_dir) return cu_strdup(name);
    if (!(r = cu_alloc(strlen(o_dir) + strlen(name) + 2))) return NULL;
    strcpy(r, o_dir);
    { int n = strlen(r); if (n && r[n - 1] != ':' && r[n - 1] != '/') strcat(r, "/"); }
    strcat(r, name);
    return r;
}

static int exists(const char *path)
{
    BPTR l = Lock((STRPTR)path, SHARED_LOCK);
    if (l) UnLock(l);
    return l != 0;
}

static LONG filesize(const char *path, ULONG *size)
{
    BPTR l = Lock((STRPTR)path, SHARED_LOCK);
    struct FileInfoBlock *fib;
    int ok = 0;
    *size = 0;
    if (!l) return 0;
    if ((fib = AllocDosObject(DOS_FIB, NULL))) {
        if (Examine(l, fib) && fib->fib_DirEntryType < 0) { *size = (ULONG)fib->fib_Size; ok = 1; }
        FreeDosObject(DOS_FIB, fib);
    }
    UnLock(l);
    return ok;
}

/* -P's drawers, made part by part */
static int makedirs(const char *dir)
{
    char p[CU_PATH];
    int i, n = strlen(dir);
    if (n >= CU_PATH) return 0;
    for (i = 0; i <= n; i++) {
        if (i == n || dir[i] == '/') {
            BPTR l;
            if (i == 0 || dir[i - 1] == ':' || dir[i - 1] == '/') continue;
            memcpy(p, dir, i); p[i] = 0;
            if (exists(p)) continue;
            if (!(l = CreateDir(p))) return 0;
            UnLock(l);
        }
    }
    return 1;
}

/* ---- the connection ---- */

static LONG fd = -1;
static void *tls;
static char *ib;                   /* what came in */
static LONG ipos, ilen;
static char nwhy[200];             /* why the network failed */
static int tls_ready;

enum { R_OK, R_RETRY, R_FATAL, R_BREAK, R_MISSING, R_SAID };   /* R_SAID: failed, already said */

static int broken(void)
{
    if (cu_rc >= 20) return 1;
    if (SetSignal(0, SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C) { cu_rc = 20; return 1; }
    return 0;
}

/* 1 the socket is ready, 0 the timeout, -1 Ctrl-C */
static int net_wait(int wr)
{
    fd_set r, w;
    struct timeval tv;
    ULONG sigs = SIGBREAKF_CTRL_C;
    LONG n;
    FD_ZERO(&r); FD_ZERO(&w);
    if (wr) FD_SET(fd, &w); else FD_SET(fd, &r);
    tv.tv_sec = o_timeout; tv.tv_usec = 0;
    n = WaitSelect(fd + 1, wr ? NULL : &r, wr ? &w : NULL, NULL, o_timeout ? &tv : NULL, &sigs);
    if (sigs & SIGBREAKF_CTRL_C) { cu_rc = 20; return -1; }
    return n > 0 ? 1 : n == 0 ? 0 : 1;   /* an error: the call after it says */
}

static void errtext(int e)
{
    const char *w = e == ECONNREFUSED ? "connection refused" : e == ETIMEDOUT ? "timed out"
                  : e == ENETUNREACH ? "network unreachable" : e == EHOSTUNREACH ? "no route to host"
                  : e == ECONNRESET ? "connection reset" : NULL;
    if (w) strcpy(nwhy, w);
    else { ml = 0; ms("network error "); mn(e); mb[ml] = 0; strcpy(nwhy, mb); ml = 0; }
}

static void net_close(void)
{
    if (tls) { tls_free(tls); tls = NULL; }
    if (fd >= 0) { CloseSocket(fd); fd = -1; }
    ipos = ilen = 0;
}

/* R_OK connected (with TLS for https), else what to do */
static int net_open(const Url *u)
{
    struct sockaddr_in sa;
    LONG one = 1;
    ULONG addr;
    char ip[20];
    memset(&sa, 0, sizeof sa);
    sa.sin_family = AF_INET;
    sa.sin_port = htons(u->port);
    addr = inet_addr((STRPTR)u->host);
    if (addr == INADDR_NONE) {
        struct hostent *he = gethostbyname((STRPTR)u->host);
        if (broken()) return R_BREAK;
        if (!he || he->h_length != 4) {
            ml = 0; ms("unable to resolve host address '"); ms(u->host); ms("'"); mb[ml] = 0;
            strcpy(nwhy, mb); ml = 0;
            return R_FATAL;
        }
        memcpy(&addr, he->h_addr, 4);
    }
    sa.sin_addr.s_addr = addr;
    {
        UBYTE *a = (UBYTE *)&addr;
        char *o = ip;
        int i;
        for (i = 0; i < 4; i++) {
            int v = a[i];
            if (v >= 100) *o++ = '0' + v / 100;
            if (v >= 10) *o++ = '0' + v / 10 % 10;
            *o++ = '0' + v % 10;
            if (i < 3) *o++ = '.';
        }
        *o = 0;
    }
    if (info_ok) {
        ms("Connecting to "); ms(u->host);
        if (strcmp(ip, u->host)) { ms(" ("); ms(ip); ms(")"); }
        ms(":"); mn(u->port); ms("... "); mout();
    }
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { errtext(errno); return R_RETRY; }
    IoctlSocket(fd, FIONBIO_, (APTR)&one);
    if (connect(fd, (struct sockaddr *)&sa, sizeof sa) < 0) {
        LONG err = 0;
        socklen_t len = sizeof err;
        int w;
        if (errno != EINPROGRESS && errno != EWOULDBLOCK) { errtext(errno); net_close(); if (info_ok) info("failed."); return R_RETRY; }
        w = net_wait(1);
        if (w < 0) { net_close(); return R_BREAK; }
        if (!w) { strcpy(nwhy, "connection timed out"); net_close(); if (info_ok) info("failed."); return R_RETRY; }
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0) err = errno;
        if (err) { errtext(err); net_close(); if (info_ok) info("failed."); return R_RETRY; }
    }
    setsockopt(fd, IPPROTO_TCP, 1 /* TCP_NODELAY */, &one, sizeof one);
    if (info_ok) info("connected.");
    if (u->tls) {
        struct Process *me = (struct Process *)FindTask(NULL);
        APTR wp = me->pr_WindowPtr;
        int r;
        if (!tls_ready) {
            const char *why;
            me->pr_WindowPtr = (APTR)-1;
            why = tls_init(SocketBase, &errno, !o_nocheck, o_cafile, o_cadir);
            me->pr_WindowPtr = wp;
            if (why) { strcpy(nwhy, why); net_close(); return R_FATAL; }
            tls_ready = 1;
        }
        if (!(tls = tls_new(fd, u->host))) { strcpy(nwhy, "out of memory"); net_close(); return R_FATAL; }
        me->pr_WindowPtr = (APTR)-1;   /* certificate drawers looked into: no requesters */
        while (errno = 0, (r = tls_handshake(tls, nwhy, sizeof nwhy)) != 1) {
            int w;
            if (r < 0) break;
            w = net_wait(r == 2);
            if (w <= 0) { r = w < 0 ? -2 : -3; break; }
        }
        me->pr_WindowPtr = wp;
        if (r != 1) {
            net_close();
            if (r == -2) return R_BREAK;
            if (r == -3) { strcpy(nwhy, "TLS handshake timed out"); return R_RETRY; }
            return strstr(nwhy, "certificate") ? R_FATAL : R_RETRY;
        }
    }
    return R_OK;
}

/* more bytes into ib: >0 how many there are, 0 the end, -1 failed (nwhy), -2 Ctrl-C */
static LONG fill(void)
{
    LONG n;
    int readable = 0;
    if (ipos < ilen) return ilen - ipos;
    ipos = ilen = 0;
    for (;;) {
        int w;
        if (broken()) return -2;
        if (tls) {
            errno = 0;
            n = tls_read(tls, ib, IBUF);
            if (n >= 0) { ilen = n; return n; }
            if (n == -1) { strcpy(nwhy, "TLS read error: "); strncat(nwhy, tls_error(), 150); return -1; }
            /* AmiSSL takes the end of a connection closed without TLS's
             * close_notify for "try again" (a stale EWOULDBLOCK in the
             * stack's errno), for ever: after the socket said readable,
             * ask it whether that was the end */
            if (n == -2 && readable) {
                char c;
                if (recv(fd, &c, 1, MSG_PEEK) == 0) { ilen = 0; return 0; }
            }
            w = net_wait(n == -3);
            readable = w > 0 && n == -2;
        } else {
            n = recv(fd, ib, IBUF, 0);
            if (n >= 0) { ilen = n; return n; }
            if (errno != EWOULDBLOCK && errno != EINTR) { errtext(errno); return -1; }
            w = net_wait(0);
        }
        if (w < 0) return -2;
        if (!w) { strcpy(nwhy, "read timed out"); return -1; }
    }
}

static int net_send(const char *b, LONG n)   /* R_OK, R_RETRY (nwhy), R_BREAK */
{
    while (n > 0) {
        LONG k;
        int w;
        if (tls) {
            errno = 0;
            k = tls_write(tls, b, n);
            if (k == -1) { strcpy(nwhy, "TLS write error: "); strncat(nwhy, tls_error(), 150); return R_RETRY; }
            if (k < 0) { w = net_wait(k == -3); goto wait; }
        } else {
            k = send(fd, (APTR)b, n, 0);
            if (k < 0) {
                if (errno != EWOULDBLOCK && errno != EINTR) { errtext(errno); return R_RETRY; }
                w = net_wait(1);
                goto wait;
            }
        }
        b += k; n -= k;
        continue;
    wait:
        if (w < 0) return R_BREAK;
        if (!w) { strcpy(nwhy, "write timed out"); return R_RETRY; }
    }
    return R_OK;
}

/* a line of the body's framing (chunk sizes): its length, -1/-2 as fill */
static LONG getline_(char *l, int max)
{
    int n = 0;
    for (;;) {
        LONG r = fill();
        if (r < 0) return r;
        if (r == 0) { strcpy(nwhy, "connection closed early"); return -1; }
        while (ipos < ilen) {
            char c = ib[ipos++];
            if (c == '\n') { while (n && l[n - 1] == '\r') n--; l[n] = 0; return n; }
            if (n < max - 1) l[n++] = c;
        }
    }
}

/* ---- the response ---- */

typedef struct {
    int status;
    char *line;                    /* "404 Not Found" */
    U64 len, rstart, rtotal;
    int haslen, chunked, hasrange, haslm;
    char *location, *disp, *ctype;
    struct DateStamp lm;
} Resp;

static char *hb;                   /* the header block */
static LONG hlen;

static int readhead(void)           /* R_OK, or what failed */
{
    hlen = 0;
    for (;;) {
        LONG r = fill();
        if (r == -2) return R_BREAK;
        if (r < 0) return R_RETRY;
        if (r == 0) { strcpy(nwhy, hlen ? "connection closed in the headers" : "no answer (the server closed the connection)"); return R_RETRY; }
        while (ipos < ilen) {
            char c = ib[ipos++];
            if (hlen >= HBUF - 1) { strcpy(nwhy, "the server's headers are too long"); return R_FATAL; }
            hb[hlen++] = c;
            if (c == '\n' && ((hlen >= 2 && hb[hlen - 2] == '\n') || (hlen >= 3 && hb[hlen - 2] == '\r' && hb[hlen - 3] == '\n'))) {
                hb[hlen] = 0;
                return R_OK;
            }
        }
    }
}

static U64 num64(const char *s, int base, const char **end)
{
    U64 v = 0;
    int d;
    while (*s == ' ' || *s == '\t') s++;
    while ((d = hexv(*s)) >= 0 && d < base) { v = v * base + d; s++; }
    if (end) *end = s;
    return v;
}

/* "Sun, 06 Nov 1994 08:49:37 GMT" */
static int httpdate(const char *s, struct DateStamp *ds)
{
    static const char mon[] = "janfebmaraprmayjunjulaugsepoctnovdec";
    LONG d, m, y, h, mi, sec;
    const char *e;
    while (*s && !(*s >= '0' && *s <= '9')) s++;
    d = num64(s, 10, &e); s = e;
    while (*s == ' ' || *s == '-') s++;
    for (m = 0; m < 12; m++) if (!ncase(s, mon + m * 3, 3)) break;
    if (m == 12) return 0;
    s += 3;
    while (*s == ' ' || *s == '-') s++;
    y = num64(s, 10, &e); s = e;
    if (y < 100) y += y < 70 ? 2000 : 1900;
    h = num64(s, 10, &e); if (*e != ':') return 0;
    mi = num64(e + 1, 10, &e); if (*e != ':') return 0;
    sec = num64(e + 1, 10, &e);
    if (y < 1978 || d < 1 || d > 31 || h > 23 || mi > 59 || sec > 60) return 0;
    /* GMT: the Amiga's clock is local (ENV:TZ, else Locale's offset) */
    cu_fromunix(((ULONG)cu_days(y, m + 1, d) + 2922) * 86400 + h * 3600 + mi * 60 + (sec > 59 ? 59 : sec), ds);
    return 1;
}

/* the name in a Content-Disposition (heap), NULL none */
static char *dispname(const char *v)
{
    const char *p;
    char *n, *d;
    int star = 0;
    for (p = v; *p; p++) if (!ncase(p, "filename*=", 10)) { star = 1; break; }
    if (!*p) for (p = v; *p; p++) if (!ncase(p, "filename=", 9)) break;
    if (!*p) return NULL;
    p += star ? 10 : 9;
    if (star) { const char *q = strstr(p, "''"); if (q) p = q + 2; }
    if (!(n = d = cu_alloc(strlen(p) + 1))) return NULL;
    if (*p == '"') { for (p++; *p && *p != '"'; p++) { if (*p == '\\' && p[1]) p++; *d++ = *p; } }
    else while (*p && *p != ';' && *p != ' ') *d++ = *p++;
    *d = 0;
    if (star) decode(n);
    tidyname(n);
    if (!*n) { cu_free(n); return NULL; }
    return n;
}

static void parsehead(Resp *r)
{
    char *p = hb, *e, *v;
    int first = 1;
    memset(r, 0, sizeof *r);
    while (*p) {
        for (e = p; *e && *e != '\n'; e++) ;
        if (*e) *e++ = 0;
        { int n = strlen(p); while (n && p[n - 1] == '\r') p[--n] = 0; }
        if (first) {
            first = 0;
            for (v = p; *v && *v != ' '; v++) ;
            while (*v == ' ') v++;
            r->line = v;
            r->status = (int)num64(v, 10, NULL);
        } else if ((v = strchr(p, ':'))) {
            *v++ = 0;
            while (*v == ' ' || *v == '\t') v++;
            if (!ncase(p, "content-length", 15)) { r->len = num64(v, 10, NULL); r->haslen = 1; }
            else if (!ncase(p, "transfer-encoding", 18)) { if (strstr(v, "chunked") || strstr(v, "Chunked")) r->chunked = 1; }
            else if (!ncase(p, "location", 9)) r->location = v;
            else if (!ncase(p, "content-disposition", 20)) r->disp = v;
            else if (!ncase(p, "content-type", 13)) r->ctype = v;
            else if (!ncase(p, "last-modified", 14)) r->haslm = httpdate(v, &r->lm);
            else if (!ncase(p, "content-range", 14)) {
                const char *q = v;
                while (*q && !(*q >= '0' && *q <= '9')) q++;
                r->rstart = num64(q, 10, &q);
                q = strchr(q, '/');
                r->hasrange = 1;
                if (q && q[1] >= '0' && q[1] <= '9') r->rtotal = num64(q + 1, 10, NULL);
            }
        }
        p = e;
    }
    if (r->chunked) r->haslen = 0;
}

static void showhead(void)         /* -S: the lines as they came, indented */
{
    char *p = hb, *e;
    while (*p) {
        for (e = p; *e && *e != '\n'; e++) ;
        if (e > p && !(e - p == 1 && *p == '\r')) {
            ms("  ");
            { int n = e - p; char sv; while (n && p[n - 1] == '\r') n--; sv = p[n]; p[n] = 0; ms(p); p[n] = sv; }
            ms("\n");
            mout();
        }
        p = *e ? e + 1 : e;
    }
}

/* ---- the output ---- */

static BPTR ofh;                   /* the file being written */
static int ostd;                   /* the output (-O -) */
static int oglobal;                /* -O FILE, open for all the URLs */
static char *oname;                /* its name, for messages */
static char *obuf;
static LONG olen;
static LONG obase;                 /* where this URL's bytes start in it */
static U64 have;                   /* bytes of this URL stored */
static U64 total;                  /* its whole size, 0 unknown */
static int werr;

static int oflush(void)
{
    if (olen && !werr) {
        if (Write(ofh, obuf, olen) != olen) {
            werr = 1;
            cu_fault(10, ostd ? "write error" : oname);
        }
    }
    olen = 0;
    return !werr;
}

static int oput(const char *p, LONG n)
{
    while (n > 0) {
        LONG k = OBUF - olen;
        if (k > n) k = n;
        memcpy(obuf + olen, p, k);
        olen += k; p += k; n -= k;
        have += k;
        if (olen == OBUF && !oflush()) return 0;
    }
    return 1;
}

/* back to this URL's start (a server that did not resume) */
static int orestart(void)
{
    olen = 0;
    if (ostd) { if (have) { strcpy(nwhy, "the server does not resume and the output cannot go back"); return 0; } return 1; }
    if (ofh) {
        Seek(ofh, obase, OFFSET_BEGINNING);
        SetFileSize(ofh, obase, OFFSET_BEGINNING);
    }
    have = 0;
    return 1;
}

/* ---- progress ---- */

static struct DateStamp t0;
static U64 got0;                   /* have when this transfer began */
static int ptty, pwidth, pshown;
static const char *pname;
static ULONG plast, pspin;
static U64 pnext;

static ULONG ticks(void)           /* 1/50 s since t0 */
{
    struct DateStamp d;
    DateStamp(&d);
    return ((d.ds_Days - t0.ds_Days) * 1440 + d.ds_Minute - t0.ds_Minute) * 3000 + d.ds_Tick - t0.ds_Tick;
}

static int columns(void)
{
    char v[16];
    LONG c;
    struct FileHandle *f = (struct FileHandle *)BADDR(errfh);
    struct InfoData *id;
    int w = 80, known = 0;
    if (GetVar("COLUMNS", v, sizeof v, 0) > 0 && StrToLong(v, &c) > 0 && c >= 20 && c < 1000) return c;
    if (!f->fh_Type) return w;
    if ((id = AllocVec(sizeof *id, MEMF_PUBLIC | MEMF_CLEAR))) {
        if (DoPkt(f->fh_Type, ACTION_DISK_INFO, MKBADDR(id), 0, 0, 0, 0)
            && (id->id_DiskType == 0x434F4E00UL || id->id_DiskType == 0x52415700UL)) {   /* CON\0 RAW\0 */
            struct IOStdReq *io = (struct IOStdReq *)id->id_InUse;
            if (io && io->io_Unit) {
                c = ((struct ConUnit *)io->io_Unit)->cu_XMax + 1;
                if (c >= 20 && c < 1000) { w = c; known = 1; }
            }
        }
        FreeVec(id);
        if (!known) {
            /* not CON: or RAW: (CTerm says 'CTRM'): ask the console itself, else a
             * line of 80 in a narrower window wrapped on every redraw */
            c = cu_termwidth(Input(), errfh);
            if (c >= 20) w = c;
        }
    }
    return w;
}

static void pstart(const char *name)
{
    DateStamp(&t0);
    got0 = have;
    pname = name;
    plast = 0; pspin = 0;
    pnext = (have / 1048576 + 1) * 1048576;
    pshown = 0;
    ptty = info_ok && IsInteractive(errfh);
    if (ptty) pwidth = columns();
}

static void pad(char *l, int *n, const char *s, int w)   /* s right aligned in w */
{
    int k = strlen(s);
    while (k < w) { l[(*n)++] = ' '; w--; }
    while (*s) l[(*n)++] = *s++;
}

static void speedtext(char *b, ULONG t)
{
    U64 got = have - got0;
    U64 sp = t ? got * 50 / t : 0;
    if (!t || !got) { strcpy(b, "--.-KB/s"); return; }
    human(b, sp);
    strcat(b, sp < 1024 ? "B/s" : "B/s");
}

static void timetext(char *b, ULONG secs)
{
    char *o = b;
    ULONG h = secs / 3600, m = secs / 60 % 60, s = secs % 60;
    char r[12]; int i;
#define PUT(v) { ULONG x = (v); i = 0; do { r[i++] = '0' + x % 10; x /= 10; } while (x); while (i) *o++ = r[--i]; }
    if (h) { PUT(h); *o++ = 'h'; PUT(m); *o++ = 'm'; }
    else if (m) { PUT(m); *o++ = 'm'; if (s < 10) *o++ = '0'; PUT(s); *o++ = 's'; }
    else { PUT(s); *o++ = 's'; }
    *o = 0;
#undef PUT
}

static void pdraw(int done)
{
    ULONG t = ticks();
    char l[300], s[32], sp[32], tm[32];
    int n = 0, w, namew, bar, i;
    if (!info_ok) return;
    if (!ptty) {                   /* a line per MB, and the end */
        if (done || have < pnext) return;
        pnext = (have / 1048576 + 1) * 1048576;
        human(s, have); speedtext(sp, t);
        ml = 0; ms("  "); ms(s);
        if (total) { ms(" "); mn(have * 100 / total); ms("%"); }
        ms(" "); ms(sp); ms("\n"); mout();
        return;
    }
    if (!done && pshown && t - plast < 10) return;   /* 5 times a second */
    plast = t; pshown = 1;
    w = pwidth - 1;
    if (w > (int)sizeof l - 2) w = sizeof l - 2;
    l[n++] = '\r';
    namew = w / 4;
    if (namew > 20) namew = 20;
    { int pl = strlen(pname); for (i = 0; i < namew; i++) l[n++] = i < pl ? pname[i] : ' '; }
    if (total) { char p[8]; int k = 0; ULONG pc = (ULONG)(have * 100 / total); if (pc > 100) pc = 100;
        if (pc >= 100) p[k++] = '1';
        if (pc >= 10) p[k++] = '0' + pc / 10 % 10;
        p[k++] = '0' + pc % 10; p[k++] = '%'; p[k] = 0;
        pad(l, &n, p, 5);
    } else pad(l, &n, "", 5);
    human(s, have); speedtext(sp, t);
    if (done) { strcpy(tm, "in "); timetext(tm + 3, t / 50); }
    else if (total && have > got0 && have < total) {
        U64 got = have - got0, left = total - have;
        strcpy(tm, "eta "); timetext(tm + 4, (ULONG)(left * t / got / 50));
    } else tm[0] = 0;
    /* [bar] then 7 + 11 + 12 columns of numbers */
    bar = w - (n - 1) - 2 - 7 - 11 - 12;
    if (bar >= 4) {
        l[n++] = '[';
        if (total) {
            int f = (int)((have > total ? total : have) * bar / total);
            for (i = 0; i < bar; i++) l[n++] = i < f - 1 ? '=' : i == f - 1 ? (f == bar ? '=' : '>') : ' ';
        } else {
            int span = bar - 3, at;
            pspin++;
            at = span > 0 ? (int)(pspin % (2 * span)) : 0;
            if (at > span) at = 2 * span - at;
            if (done) at = -10;
            for (i = 0; i < bar; i++) l[n++] = i >= at && i < at + 3 ? "<=>"[i - at] : ' ';
        }
        l[n++] = ']';
    }
    pad(l, &n, s, 7);
    pad(l, &n, sp, 11);
    l[n++] = ' '; l[n++] = ' ';
    for (i = 0; tm[i] && n < (int)sizeof l - 2; i++) l[n++] = tm[i];
    while (n - 1 < w) l[n++] = ' ';
    if (n > w + 1) n = w + 1;
    if (done) l[n++] = '\n';
    cu_flush();
    Write(errfh, l, n);
}

/* ---- one fetch ---- */

static char *reqbuf;

static int request(const Url *u, int head)
{
    int i, len = strlen(u->path) + strlen(u->host) + strlen(o_agent) + strlen(u->auth) + 400;
    int ownhost = 0, ownagent = 0;
    char *b;
    for (i = 0; i < o_nhdr; i++) {
        len += strlen(o_hdr[i]) + 2;
        if (!ncase(o_hdr[i], "host:", 5)) ownhost = 1;
        if (!ncase(o_hdr[i], "user-agent:", 11)) ownagent = 1;
    }
    cu_free(reqbuf);
    if (!(reqbuf = b = cu_alloc(len))) { strcpy(nwhy, "out of memory"); return R_FATAL; }
    strcpy(b, head ? "HEAD " : "GET ");
    strcat(b, u->path);
    strcat(b, " HTTP/1.1\r\n");
    if (!ownhost) {
        strcat(b, "Host: "); strcat(b, u->host);
        if (u->port != (u->tls ? 443 : 80)) { ml = 0; ms(":"); mn(u->port); mb[ml] = 0; strcat(b, mb); ml = 0; }
        strcat(b, "\r\n");
    }
    if (!ownagent) { strcat(b, "User-Agent: "); strcat(b, o_agent); strcat(b, "\r\n"); }
    strcat(b, "Accept: */*\r\nAccept-Encoding: identity\r\n");
    if (have && !head) { ml = 0; ms("Range: bytes="); mn(have); ms("-\r\n"); mb[ml] = 0; strcat(b, mb); ml = 0; }
    if (u->auth[0]) { strcat(b, "Authorization: Basic "); strcat(b, u->auth); strcat(b, "\r\n"); }
    strcat(b, "Connection: close\r\n");
    for (i = 0; i < o_nhdr; i++) { strcat(b, o_hdr[i]); strcat(b, "\r\n"); }
    strcat(b, "\r\n");
    return net_send(b, strlen(b));
}

static int body(const Resp *r)
{
    U64 left = r->len;
    LONG n;
    if (r->chunked) {
        char l[80];
        for (;;) {
            U64 csz;
            n = getline_(l, sizeof l);
            if (n == -2) return R_BREAK;
            if (n < 0) return R_RETRY;
            if (!n) continue;          /* the CRLF after a chunk */
            csz = num64(l, 16, NULL);
            if (!csz) {                /* the last; trailers to the empty line */
                while ((n = getline_(l, sizeof l)) > 0) ;
                return n == -2 ? R_BREAK : R_OK;
            }
            while (csz > 0) {
                LONG k;
                n = fill();
                if (n == -2) return R_BREAK;
                if (n < 0) return R_RETRY;
                if (n == 0) { strcpy(nwhy, "connection closed early"); return R_RETRY; }
                k = csz < (U64)n ? (LONG)csz : n;
                if (!oput(ib + ipos, k)) return R_FATAL;
                ipos += k; csz -= k;
                pdraw(0);
            }
        }
    }
    for (;;) {
        LONG k;
        if (r->haslen && !left) return R_OK;
        n = fill();
        if (n == -2) return R_BREAK;
        if (n < 0) return R_RETRY;
        if (n == 0) {
            if (!r->haslen) return R_OK;
            strcpy(nwhy, "connection closed early");
            return R_RETRY;
        }
        k = n;
        if (r->haslen && left < (U64)k) k = (LONG)left;
        if (!oput(ib + ipos, k)) return R_FATAL;
        ipos += k;
        if (r->haslen) left -= k;
        pdraw(0);
    }
}

static Url cur;
static char *fname;                /* the file this URL goes into (NULL: not chosen yet) */
static int keepname;               /* -c: the name is used as it is */
static int fresh;                  /* the file was made by this run (deleted when nothing came) */
static int complete;               /* already whole (416): no date set */
static struct DateStamp lmdate;
static int haslm;

static const Url *orig;            /* the URL as asked for: it names the file, as in wget */

static int openfile(const Resp *r)
{
    char *n = NULL;
    if (ofh) return 1;
    if (!fname) {
        char *d = o_cd && r->disp ? dispname(r->disp) : NULL;
        char *base = d ? d : urlname(orig);
        if (!base) return 0;
        n = withdir(base);
        if (!n) return 0;
        if (!keepname && exists(n)) {  /* name.1, name.2 ... */
            char *t = cu_alloc(strlen(n) + 8);
            int i;
            if (!t) return 0;
            for (i = 1; i < 100000; i++) {
                strcpy(t, n); ml = 0; ms("."); mn(i); mb[ml] = 0; strcat(t, mb); ml = 0;
                if (!exists(t)) break;
            }
            n = t;
        }
        fname = n;
    }
    if (o_dir && *o_dir && !makedirs(o_dir)) { cu_fault(10, o_dir); return 0; }
    if (keepname && have) {
        ofh = Open(fname, MODE_READWRITE);
        if (ofh) Seek(ofh, 0, OFFSET_END);
    } else {
        ofh = Open(fname, MODE_NEWFILE);
        fresh = 1;
    }
    if (!ofh) { cu_fault(10, fname); return 0; }
    oname = fname;
    obase = 0;
    return 1;
}

static void sayname(const char *s)   /* 'name' */
{
    ms("'"); ms(s); ms("'");
}

static int fetch(const Url *start, int *hops)
{
    Resp r;
    int res;
    char *loc;
    cu_free(cur.path); cu_free(cur.full);
    memcpy(&cur, start, sizeof cur);
    cur.path = cu_strdup(start->path); cur.full = cu_strdup(start->full);
    if (!cur.path || !cur.full) return R_FATAL;
    for (;;) {
        if (info_ok) { ms("--"); stamp(); ms("--  "); ms(cur.full); ms("\n"); mout(); }
        if ((res = net_open(&cur)) != R_OK) return res;
        if ((res = request(&cur, o_spider)) != R_OK) { net_close(); return res; }
        if (info_ok && !o_show) { ms("HTTP request sent, awaiting response... "); mout(); }
        do {
            if ((res = readhead()) != R_OK) { net_close(); if (info_ok && !o_show) info(""); return res; }
            if (o_show && !o_quiet) showhead();
            parsehead(&r);
        } while (r.status >= 100 && r.status < 200);
        if (info_ok && !o_show) info(r.line);
        if (r.location && (r.status == 301 || r.status == 302 || r.status == 303 || r.status == 307 || r.status == 308)) {
            net_close();
            if (++*hops > 20) { strcpy(nwhy, "more than 20 redirections"); return R_FATAL; }
            if (!(loc = resolve(&cur, r.location))) return R_FATAL;
            if (info_ok) { ms("Location: "); ms(r.location); ms(" [following]\n"); mout(); }
            {
                Url nu;
                memset(&nu, 0, sizeof nu);
                if (!url_parse(&nu, loc)) { cu_free(loc); return R_SAID; }
                if (strcmp(nu.host, cur.host) || nu.port != cur.port) nu.auth[0] = 0;
                else if (!nu.auth[0]) strcpy(nu.auth, cur.auth);
                cu_free(cur.path); cu_free(cur.full);
                memcpy(&cur, &nu, sizeof cur);
            }
            cu_free(loc);
            continue;
        }
        if (r.status == 416 && have && !o_spider) {
            net_close();
            info("The file is already fully retrieved; nothing to do.");
            complete = 1;
            return R_OK;
        }
        if (r.status < 200 || r.status >= 300) {
            net_close();
            strncpy(nwhy, r.line, sizeof nwhy - 1);
            nwhy[sizeof nwhy - 1] = 0;
            if (r.status >= 500) return R_RETRY;
            return o_spider ? R_MISSING : R_FATAL;
        }
        if (o_spider) {
            net_close();
            info("Remote file exists.");
            return R_OK;
        }
        if (have) {
            if (r.status == 206) {
                if (!r.hasrange || r.rstart != have) { net_close(); strcpy(nwhy, "the server sent another part than asked for"); return R_FATAL; }
            } else {
                info("The server does not resume: starting from the beginning.");
                if (!orestart()) { net_close(); return R_FATAL; }
            }
        }
        total = r.status == 206 ? (r.rtotal ? r.rtotal : r.haslen ? have + r.len : 0) : r.haslen ? r.len : 0;
        if (!ostd && !oglobal && !openfile(&r)) { net_close(); return R_SAID; }
        if (info_ok) {
            char h[16];
            ms("Length: ");
            if (r.haslen) { U64 l = r.status == 206 ? total : r.len; mn(l); human(h, l); ms(" ("); ms(h); ms(")");
                if (r.status == 206) { human(h, total - have); ms(", "); ms(h); ms(" remaining"); } }
            else ms("unspecified");
            if (r.ctype) { ms(" ["); ms(r.ctype); ms("]"); }
            ms("\nSaving to: "); sayname(ostd ? "STDOUT" : oname); ms("\n\n"); mout();
        }
        haslm = r.haslm;
        if (haslm) memcpy(&lmdate, &r.lm, sizeof lmdate);
        pstart(ostd ? "-" : *FilePart((STRPTR)oname) ? (const char *)FilePart((STRPTR)oname) : oname);
        res = body(&r);
        if (res == R_OK || res == R_BREAK) { if (pshown || !ptty) pdraw(1); }
        else if (ptty && pshown) { cu_flush(); Write(errfh, "\n", 1); }
        oflush();
        net_close();
        if (werr) return R_SAID;
        return res;
    }
}

/* ---- one URL, with its tries ---- */

static int stopall;
static U64 ocont;                  /* -c -O FILE: its size, for the first URL */

static void geturl(const char *arg)
{
    Url u;
    int tries = 0, res, hops;
    ULONG sz;
    memset(&u, 0, sizeof u);
    if (!url_parse(&u, arg)) return;
    orig = &u;
    fname = NULL; keepname = 0; fresh = 0; complete = 0; haslm = 0; werr = 0;
    have = 0; total = 0; olen = 0;
    if (oglobal || ostd) {
        if (oglobal) obase = Seek(ofh, 0, OFFSET_CURRENT);
        if (ocont) { have = ocont; obase = 0; ocont = 0; }   /* -c -O: the file so far */
    } else if (!o_spider) {
        char *n = urlname(&u);
        char *p = n ? withdir(n) : NULL;
        if (!p) { cu_err(10, NULL, "out of memory"); return; }
        if (o_nc && exists(p)) {
            if (!o_quiet) { ms("File "); sayname(p); ms(" already there; not retrieving.\n"); mout(); }
            return;
        }
        if (o_cont) {
            keepname = 1; fname = p;
            if (filesize(p, &sz)) have = sz;
        }
    }
    for (;;) {
        tries++;
        hops = 0;
        nwhy[0] = 0;
        res = fetch(&u, &hops);
        if (res == R_OK || res == R_BREAK || res == R_SAID) break;
        if (res == R_MISSING) {
            char w[220];
            strcpy(w, "does not exist ("); strncat(w, nwhy, 190); strcat(w, ")");
            cu_err(5, cur.full ? cur.full : u.full, w);
            break;
        }
        if (res == R_FATAL) { cu_err(10, cur.full ? cur.full : u.full, nwhy); break; }
        /* R_RETRY */
        if (o_tries && tries >= o_tries) {
            cu_err(10, cur.full ? cur.full : u.full, nwhy);
            if (tries > 1 && !o_quiet) { ms("wget: giving up after "); mn(tries); ms(" tries\n"); mout(); }
            break;
        }
        if (info_ok) { ms(nwhy); ms(". Retrying.\n\n"); mout(); }
        {
            int i, wait = tries < 10 ? tries : 10;
            for (i = 0; i < wait * 5 && !broken(); i++) Delay(10);
        }
        if (broken()) { res = R_BREAK; break; }
    }
    if (res == R_BREAK) {
        oflush();
        cu_flush();
        Write(errfh, "***Break\n", 9);
        cu_rc = 20;
        stopall = 1;
        if (ofh && !oglobal && !ostd && !o_quiet) {
            ms("wget: "); sayname(oname); ms(" kept, "); mn(have); ms(" bytes (wget -c goes on)\n"); mout();
        }
    }
    if (ofh && !oglobal && !ostd) {
        Close(ofh);
        ofh = 0;
        if (res == R_OK && haslm && !complete) SetFileDate(fname, &lmdate);
        if (res != R_OK && res != R_BREAK && fresh && have == 0) DeleteFile(fname);
    }
    if (res == R_OK && !o_spider && !complete) {
        const char *nm = ostd ? "-" : oname;
        if (info_ok) {
            ULONG t = ticks();
            char sp[32];
            speedtext(sp, t);
            stamp(); ms(" ("); ms(sp); ms(") - "); sayname(ostd ? "STDOUT" : nm); ms(" saved [");
            mn(have); if (total) { ms("/"); mn(total); } ms("]\n\n"); mout();
        } else if (o_nv && !o_quiet) {
            ms(cur.full); ms(" -> "); sayname(nm); ms(" ["); mn(have);
            if (total && total != have) { ms("/"); mn(total); }
            ms("]\n"); mout();
        }
    }
    if (o_spider && res == R_OK && o_nv && !o_quiet) { ms(cur.full); ms(": exists\n"); mout(); }
    cu_free(u.path); cu_free(u.full);
}

/* ---- the command line ---- */

static const char *unsupported[] = {
    "recursive", "mirror", "level", "convert-links", "page-requisites", "timestamping",
    "post-data", "post-file", "load-cookies", "save-cookies", "keep-session-cookies",
    "no-cookies", "accept", "reject", "domains", "span-hosts", "no-parent", "no-directories",
    "force-directories", "no-host-directories", "cut-dirs", "background", "wait", "random-wait",
    "execute", "limit-rate", "method", "body-data", "body-file", "user", "password",
    "http-user", "http-passwd", "http-password", "proxy-user", "proxy-password", "no-proxy",
    "restrict-file-names", "adjust-extension", "html-extension", "backup-converted",
    "follow-ftp", "ftp-user", "ftp-password", "inet6-only", "inet4-only", "bind-address",
    "append-output", "debug", "quota", "save-headers", "referer", "dns-timeout",
    "connect-timeout", "read-timeout", "waitretry", "retry-connrefused", NULL
};

static int number(const char *v, int *to)
{
    int ok;
    long n = cu_num(v, &ok);
    if (!ok || n < 0) { cu_err(10, v, "not a number"); return 0; }
    *to = n;
    return 1;
}

/* a long option: 1 used (*skip words taken), 0 bad (said) */
static int longopt(char *a, char *next, int *skip)
{
    char *eq = strchr(a, '='), *v = eq ? eq + 1 : next;
    int n = eq ? eq - a : (int)strlen(a), i;
#define IS(s) (n == (int)strlen(s) && !strncmp(a, s, n))
#define VAL() (eq ? 0 : (v ? (*skip = 1) : 0), v)
    *skip = 0;
    if (IS("spider")) o_spider = 1;
    else if (IS("no-check-certificate")) o_nocheck = 1;
    else if (IS("check-certificate")) o_nocheck = 0;
    else if (IS("content-disposition")) o_cd = 1;
    else if (IS("continue")) o_cont = 1;
    else if (IS("quiet")) o_quiet = 1;
    else if (IS("no-verbose")) o_nv = 1;
    else if (IS("verbose")) o_nv = o_quiet = 0;
    else if (IS("no-clobber")) o_nc = 1;
    else if (IS("server-response")) o_show = 1;
    else if (IS("version")) { cu_puts(version + 6); cu_putc('\n'); cu_flush(); return -1; }
    else if (IS("help")) return -2;
    else if (IS("header") || IS("output-document") || IS("directory-prefix") || IS("tries") || IS("timeout")
             || IS("user-agent") || IS("input-file") || IS("output-file") || IS("ca-certificate")
             || IS("ca-directory")) {
        if (!VAL()) { cu_err(10, a, "needs a value"); return 0; }
        if (IS("header")) { if (o_nhdr < 32) o_hdr[o_nhdr++] = v; }
        else if (IS("output-document")) o_out = v;
        else if (IS("directory-prefix")) o_dir = v;
        else if (IS("tries")) { if (!number(v, &o_tries)) return 0; }
        else if (IS("timeout")) { if (!number(v, &o_timeout)) return 0; }
        else if (IS("user-agent")) o_agent = v;
        else if (IS("input-file")) o_input = v;
        else if (IS("output-file")) o_log = v;
        else if (IS("ca-certificate")) o_cafile = v;
        else o_cadir = v;
    } else {
        for (i = 0; unsupported[i]; i++) if (IS(unsupported[i])) { cu_err(10, a - 2, "not supported"); return 0; }
        cu_err(10, a - 2, "unknown option");
        return 0;
    }
    return 1;
#undef IS
#undef VAL
}

#define USAGE "wget [-cqSV] [-nv] [-nc] [-O FILE] [-P DIR] [-t N] [-T SECS] [-U AGENT] [--header H] [-i FILE] [-o LOG] [--spider] [--content-disposition] [--no-check-certificate] URL ..."

static int wmain(void)
{
    char *v;
    int c, i, skip, nurl = 0;
    struct Process *me = (struct Process *)FindTask(NULL);
    BPTR oldces = me->pr_CES, logfh = 0;
    (void)version;
    if (cu_args(USAGE) < 0) return cu_rc;
    for (i = 0; i < cu_argc; i++) {   /* long options and -nv -nc, before the short ones */
        v = cu_argv[i];
        if (v[0] != '-') continue;
        if (v[1] == '-' && !v[2]) break;
        skip = -1;
        if (v[1] == '-') {
            int r = longopt(v + 2, i + 1 < cu_argc ? cu_argv[i + 1] : NULL, &skip);
            if (r == -1) return 0;
            if (r == -2) { cu_puts("usage: " USAGE "\n"); cu_flush(); return 0; }
            if (!r) return cu_rc;
        } else if (!strcmp(v, "-nv")) { o_nv = 1; skip = 0; }
        else if (!strcmp(v, "-nc")) { o_nc = 1; skip = 0; }
        else if (v[1] == 'n' && v[2] && v[3] == 0 && strchr("dHp", v[2])) { cu_err(10, v, "not supported"); return cu_rc; }
        if (skip >= 0) {
            memmove(&cu_argv[i], &cu_argv[i + 1 + skip], (cu_argc - i - skip) * sizeof *cu_argv);
            cu_argc -= 1 + skip; i--;
        }
    }
    {   /* GNU's order: flags may come after the URLs too; flags first, then -- */
        char **o = cu_alloc((cu_argc + 2) * sizeof *o), **w = cu_alloc((cu_argc + 1) * sizeof *w);
        int n = 0, m = 0, k;
        if (!o || !w) { cu_err(20, NULL, "out of memory"); return cu_rc; }
        for (i = 0; i < cu_argc; i++) {
            char *a = cu_argv[i];
            if (!strcmp(a, "--")) { for (i++; i < cu_argc; i++) w[m++] = cu_argv[i]; break; }
            if (a[0] == '-' && a[1]) {
                o[n++] = a;
                for (k = 1; a[k]; k++)
                    if (strchr("OPtTUioAlRDwe", a[k])) { if (!a[k + 1] && i + 1 < cu_argc) o[n++] = cu_argv[++i]; break; }
            } else w[m++] = a;
        }
        o[n++] = "--";
        for (k = 0; k < m; k++) o[n++] = w[k];
        cu_argv = o; cu_argc = n;
    }
    while ((c = cu_opt("O:P:cqt:T:U:Si:o:hVrmNkpEKHxLbl:A:R:D:w:e:", &v)) >= 0) switch (c) {
        case 'O': o_out = v; break;
        case 'P': o_dir = v; break;
        case 'c': o_cont = 1; break;
        case 'q': o_quiet = 1; break;
        case 't': if (!number(v, &o_tries)) return cu_rc; break;
        case 'T': if (!number(v, &o_timeout)) return cu_rc; break;
        case 'U': o_agent = v; break;
        case 'S': o_show = 1; break;
        case 'i': o_input = v; break;
        case 'o': o_log = v; break;
        case 'h': cu_puts("usage: " USAGE "\n"); cu_flush(); return 0;
        case 'V': cu_puts(version + 6); cu_putc('\n'); cu_flush(); return 0;
        case '?': return cu_rc;
        default: {
            char f[3] = { '-', (char)c, 0 };
            cu_err(10, f, "not supported");
            return cu_rc;
        }
    }
    if (!cu_argc && !o_input) { cu_err(10, NULL, "missing URL (wget ? for the usage)"); return cu_rc; }
    if (o_quiet) cu_silent = 1;
    if (o_log) {
        if (!(logfh = Open(o_log, MODE_NEWFILE))) { cu_fault(10, o_log); return cu_rc; }
        me->pr_CES = logfh;

    }
    errfh = me->pr_CES ? me->pr_CES : Output();
    info_ok = !o_quiet && !o_nv;
    if (!(ib = cu_alloc(IBUF)) || !(obuf = cu_alloc(OBUF)) || !(hb = cu_alloc(HBUF))) { cu_err(20, NULL, "out of memory"); goto out; }
    if (!(SocketBase = OpenLibrary("bsdsocket.library", 4))) { cu_err(10, NULL, "no TCP/IP stack (bsdsocket.library)"); goto out; }
    {
        struct TagItem t[2];
        t[0].ti_Tag = SBTM_SETVAL(SBTC_ERRNOPTR(sizeof(errno))); t[0].ti_Data = (ULONG)&errno;
        t[1].ti_Tag = TAG_DONE; t[1].ti_Data = 0;
        SocketBaseTagList(t);
    }
    if (o_out) {
        if (!strcmp(o_out, "-")) {
            ostd = 1; ofh = Output(); oname = "-";
            if (errfh == ofh) info_ok = 0;   /* the messages would land in the data */
        } else {
            oglobal = 1; oname = o_out;
            ofh = Open(o_out, o_cont ? MODE_READWRITE : MODE_NEWFILE);
            if (!ofh) { cu_fault(10, o_out); goto out; }
            if (o_cont) { Seek(ofh, 0, OFFSET_END); ocont = (ULONG)Seek(ofh, 0, OFFSET_CURRENT); }
        }
    }
    for (i = 0; i < cu_argc && !stopall; i++) { geturl(cu_argv[i]); nurl++; }
    if (o_input && !stopall) {
        CuIn in;
        char *line;
        int nl;
        LONG n;
        if (cu_open(&in, strcmp(o_input, "-") ? o_input : NULL)) {
            while (!stopall && (n = cu_line(&in, &line, &nl)) >= 0) {
                char *s = line, *e = line + n;
                while (s < e && (*s == ' ' || *s == '\t')) s++;
                while (e > s && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\r')) e--;
                if (s == e || *s == '#') continue;
                *e = 0;
                geturl(s);
                nurl++;
            }
            cu_close(&in);
        }
        if (!nurl && cu_rc < 10) cu_err(10, o_input, "no URLs in it");
    }
out:
    if (oglobal && ofh) Close(ofh);
    if (tls_ready) tls_end();
    if (SocketBase) CloseLibrary(SocketBase);
    cu_flush();
    if (logfh) { me->pr_CES = oldces; Close(logfh); }
    return cu_rc;
}

/* OpenSSL wants more stack than the Shell's usual 4 KB: a stack of our own */
#define STACKSZ 32768
static struct StackSwapStruct sss;
static int sret;

static void __attribute__((noinline)) runswapped(void) { sret = wmain(); }

int main(void)
{
    struct Task *me = FindTask(NULL);
    char *big;
    if ((char *)me->tc_SPUpper - (char *)me->tc_SPLower >= STACKSZ || !(big = AllocVec(STACKSZ, MEMF_ANY)))
        return wmain();
    sss.stk_Lower = big;
    sss.stk_Upper = (ULONG)(big + STACKSZ);
    sss.stk_Pointer = (APTR)(big + STACKSZ);
    StackSwap(&sss);
    runswapped();
    StackSwap(&sss);
    FreeVec(big);
    return sret;
}
