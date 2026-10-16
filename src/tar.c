/* tar - bundle files and drawers into one archive, list one, unpack one;
 * GNU tar's flags and formats.
 *
 *   tar -c [-vzjJa] [-f ARCHIVE] [-C DRAWER] [--exclude PAT] [-T FILE] name ...
 *   tar -x [-vkmO] [-f ARCHIVE] [-C DRAWER] [--strip-components N] [member ...]
 *   tar -t [-v] [-f ARCHIVE] [member ...]
 *
 * The first word may also be the old bundle without a dash: `tar xzvf
 * f.tgz`, `tar cf out.tar Work:dir` (its f, C and T take the next words
 * in turn). Flags may come after the names too, as in GNU tar.
 *
 *   -c --create   a new archive from the names (files, drawers with all
 *                 that is in them, patterns; * works as #?)
 *   -x --extract  unpack (--get); member names or patterns: only those,
 *                 and what is in a drawer named
 *   -t --list     list the members; -v the long form (as GNU: mode,
 *                 owner/group, size, date, name)
 *   -f --file A   the archive; - (and the default) is the input / output
 *   -v --verbose  the names as they go (twice: the long form)
 *   -z -j -J --zstd  gzip, bzip2, xz, zstd when creating (reading finds
 *                 the kind from the archive's first bytes, always)
 *   -a --auto-compress  the kind from the archive's name (.tgz .tar.bz2 ...)
 *   -C --directory D  unpack into D; when creating, the names after it
 *                 are taken from D (several -C: each from the one before)
 *   --strip-components N  drop the first N parts of each member's name
 *   -k --keep-old-files  never replace an existing file (said, rc 5);
 *                 --skip-old-files: the same, quietly
 *   --exclude PAT  leave out what matches (the name, or any run of its
 *                 parts: --exclude #?.o, --exclude CVS)
 *   -O --to-stdout  the files' contents to the output, nothing written
 *   -m --touch    do not restore the dates
 *   -p            accepted: protection is always kept
 *   -T --files-from F  the names to store, one a line (- the input)
 *   --no-recursion  store a drawer, not what is in it
 *
 * Formats read: v7, ustar (with its prefix), GNU (long names and links,
 * ././@LongLink), pax (path, linkpath, size, mtime; AMIGA.protect and
 * AMIGA.comment); both checksums (signed and unsigned); the end is two zero
 * blocks, or simply the end. Written: ustar, with a pax header when a name
 * does not fit (100 + 155 bytes) or the Amiga has more to keep.
 *
 * Amiga details: the mode's r w x are the Amiga's r, w (and d), e; a
 * drawer is 0755. What Unix has no place for - s p a h, a w without d,
 * the file note - goes in the pax keys AMIGA.protect ("----rwed" as List
 * shows it) and AMIGA.comment, only when it differs from what the mode
 * says, so an archive made here comes back exactly. Dates: the Amiga's
 * clock is taken as UTC (as Amiga tools do: no time zone is known). Names
 * are written in UTF-8 and read back to Latin-1 (a character beyond
 * Latin-1 becomes _); names that are not UTF-8 are taken as they are.
 * Soft links are stored as symlinks (an Amiga "/" parent as "../", ":x"
 * as "/x"); a hard link met twice in the tree is stored once and linked.
 * The archive itself is never stored into itself.
 *
 * Safety when unpacking: a leading "/", a volume ("Work:") and anything
 * up to a ".." are dropped from member names, with a word (as GNU tar
 * does), so nothing is written outside the drawer. Soft links are made
 * last, so no member can be written through one. An existing file is
 * deleted and replaced (a delete-protected one is said and kept); a
 * drawer is never replaced by a file. A name the file system cannot hold
 * (FFS: 30 letters) is said and skipped - never silently cut. Drawers get
 * their dates and protection at the end.
 *
 * Return code 0, 5 when something was skipped (and said), 10 for a damaged
 * archive or a file that could not be written, 20 Ctrl-C. */
#include "cu.h"
#include "cz.h"

const char *cu_name = "tar";
static const char version[] = "$VER: tar 0.1b1 (14.10.2026)";

#define USAGE "tar -c|-x|-t [-vzjJakmOp] [-f ARCHIVE] [-C DRAWER] [--strip-components N] [--exclude PAT] [-T FILE] [name ...]"
#define NAMEMAX 1024             /* the longest member name handled */
#define IBUF 32768
#define OBUF 40960               /* four records of 10240 */
#define RECORD 10240
#define PAXMAX (1024L * 1024)

/* ---- options ---- */

static int mode, o_verbose, o_keep, o_quietkeep, o_stdout, o_nodate, o_norec, o_auto;
static int kind = CZ_NONE;
static long o_strip;
static const char *archive = "-";

typedef struct {
    char *s;
    char *parsed;                /* a pattern (members), else NULL */
    int isdir;                   /* a -C drawer, not a name */
    int found;
} Word;
static Word *words;
static int nwords, maxwords;

typedef struct { char *text, *parsed; } Pat;
static Pat *excl;
static int nexcl, maxexcl;

static struct FileInfoBlock *fib;
static BPTR verr;                /* where -v goes */

static int lc(int c)
{
    c &= 0xff;
    return (c >= 'A' && c <= 'Z') || (c >= 0xc0 && c <= 0xde && c != 0xd7) ? c + 32 : c;
}

static int eqi(const char *a, const char *b, int n)
{
    int i;
    for (i = 0; i < n; i++) if (lc(a[i]) != lc(b[i])) return 0;
    return 1;
}

static int addword(const char *s, int isdir)
{
    Word *w;
    if (nwords == maxwords) {
        Word *t = cu_alloc((maxwords = maxwords ? maxwords * 2 : 32) * sizeof *t);
        if (!t) { cu_err(20, NULL, "out of memory"); return 0; }
        if (words) { memcpy(t, words, nwords * sizeof *t); cu_free(words); }
        words = t;
    }
    w = &words[nwords++];
    memset(w, 0, sizeof *w);
    if (!(w->s = cu_strdup(s))) { cu_err(20, NULL, "out of memory"); return 0; }
    w->isdir = isdir;
    return 1;
}

/* a pattern: parsed when it has wild characters, else the text */
static int mkpat(const char *s, char **parsed)
{
    int n = strlen(s) * 4 + 8;
    *parsed = NULL;
    if (!cu_haswild(s)) return 1;
    if (!(*parsed = cu_alloc(n)) || !cu_pattern(s, *parsed, n)) {
        cu_err(10, s, "not a pattern that can be used");
        return 0;
    }
    return 1;
}

static int addexcl(const char *s)
{
    if (nexcl == maxexcl) {
        Pat *t = cu_alloc((maxexcl = maxexcl ? maxexcl * 2 : 8) * sizeof *t);
        if (!t) { cu_err(20, NULL, "out of memory"); return 0; }
        if (excl) { memcpy(t, excl, nexcl * sizeof *t); cu_free(excl); }
        excl = t;
    }
    excl[nexcl].text = (char *)s;
    if (!mkpat(s, &excl[nexcl].parsed)) return 0;
    nexcl++;
    return 1;
}

static int patmatch(const char *text, const char *parsed, const char *s)
{
    return parsed ? cu_match(parsed, s) : (strlen(text) == strlen(s) && eqi(text, s, strlen(s)));
}

/* --exclude: the name, or any run of its whole parts, matches */
static int excluded(const char *name)
{
    static char t[NAMEMAX];
    int i, s, e, n;
    if (!nexcl) return 0;
    n = strlen(name);
    if (n >= NAMEMAX) n = NAMEMAX - 1;
    memcpy(t, name, n);
    t[n] = 0;
    while (n > 1 && t[n - 1] == '/') t[--n] = 0;
    for (s = 0; s < n;) {
        for (e = s;; e++) {
            if (t[e] == '/' || !t[e]) {
                char c = t[e];
                t[e] = 0;
                for (i = 0; i < nexcl; i++)
                    if (patmatch(excl[i].text, excl[i].parsed, t + s)) { t[e] = c; return 1; }
                t[e] = c;
                if (!c) break;
            }
        }
        while (t[s] && t[s] != '/') s++;
        while (t[s] == '/') s++;
    }
    return 0;
}

/* -x / -t with names: a member is wanted when a name is it, or a drawer above it */
static int selected(const char *name)
{
    int i, any = 0, n = strlen(name), hit = 0;
    while (n > 1 && name[n - 1] == '/') n--;
    for (i = 0; i < nwords; i++) {
        Word *w = &words[i];
        int l;
        if (w->isdir) continue;
        any = 1;
        if (w->parsed) {
            static char t[NAMEMAX];
            memcpy(t, name, n);
            t[n] = 0;
            if (cu_match(w->parsed, t)) w->found = hit = 1;
            continue;
        }
        l = strlen(w->s);
        while (l > 1 && w->s[l - 1] == '/') l--;
        if (n >= l && eqi(w->s, name, l) && (n == l || name[l] == '/')) w->found = hit = 1;
    }
    return !any || hit;
}

/* ---- names: UTF-8 in the archive, Latin-1 here ---- */

/* UTF-8 with something beyond ASCII: made Latin-1 in place (beyond
 * Latin-1: _); anything that is not UTF-8 stays as it is */
static void fromutf8(char *s)
{
    UBYTE *p, *d;
    int any = 0, n, i;
    for (p = (UBYTE *)s; *p;) {
        if (*p < 0x80) { p++; continue; }
        n = (*p & 0xe0) == 0xc0 ? 1 : (*p & 0xf0) == 0xe0 ? 2 : (*p & 0xf8) == 0xf0 ? 3 : -1;
        if (n < 0 || *p == 0xc0 || *p == 0xc1) return;
        for (i = 1; i <= n; i++) if ((p[i] & 0xc0) != 0x80) return;
        p += n + 1;
        any = 1;
    }
    if (!any) return;
    for (p = d = (UBYTE *)s; *p;) {
        ULONG c;
        if (*p < 0x80) { *d++ = *p++; continue; }
        n = (*p & 0xe0) == 0xc0 ? 1 : (*p & 0xf0) == 0xe0 ? 2 : 3;
        c = *p & (0x3f >> n);
        for (i = 1; i <= n; i++) c = (c << 6) | (p[i] & 0x3f);
        *d++ = c <= 0xff ? c : '_';
        p += n + 1;
    }
    *d = 0;
}

/* Latin-1 to UTF-8; -1 when it does not fit max (with its 0) */
static int toutf8(const char *s, char *d, int max)
{
    int k = 0;
    for (; *s; s++) {
        UBYTE c = *s;
        if (k + 3 > max) return -1;
        if (c < 0x80) d[k++] = c;
        else { d[k++] = 0xc0 | (c >> 6); d[k++] = 0x80 | (c & 0x3f); }
    }
    d[k] = 0;
    return k;
}

/* soft link targets: an Amiga path ("/" up, ":" the volume's root) as a Unix one */
static void amiga2unix(const char *t, char *o, int max)
{
    int k = 0, i = 0;
    const char *c = strchr(t, ':');
    max -= 4;
    if (c && c != t) { strncpy(o, t, max); o[max] = 0; return; }   /* another volume: as it is */
    if (c) { o[k++] = '/'; i = 1; }
    while (t[i] && k < max) {
        if (t[i] == '/') { memcpy(o + k, "../", 3); k += 3; i++; continue; }
        while (t[i] && t[i] != '/' && k < max) o[k++] = t[i++];
        if (t[i] == '/') { i++; if (t[i]) o[k++] = '/'; }
    }
    o[k] = 0;
}

/* and back: "../x" as "/x", "/usr" as ":usr", "a/../b" as "a//b" */
static void unix2amiga(const char *t, char *o, int max)
{
    int k = 0, prev = 0;                 /* prev: the part before was a name (a / must come first) */
    const char *s = strchr(t, '/'), *c = strchr(t, ':');
    max -= 2;
    if (c && (!s || c < s)) { strncpy(o, t, max); o[max] = 0; return; }   /* "Work:x": as it is */
    if (*t == '/') o[k++] = ':';
    while (*t) {
        const char *e;
        int n;
        while (*t == '/') t++;
        if (!*t) break;
        for (e = t; *e && *e != '/'; e++) ;
        n = e - t;
        if (!(n == 1 && *t == '.')) {
            if (k + n + 2 > max) break;
            if (prev) o[k++] = '/';
            if (n == 2 && t[0] == '.' && t[1] == '.') { o[k++] = '/'; prev = 0; }
            else { memcpy(o + k, t, n); k += n; prev = 1; }
        }
        t = e;
    }
    o[k] = 0;
}

/* "removing leading `X' from member names", once for each X */
static void notice(const char *pre, int n)
{
    static char seen[8][40];
    static int nseen;
    static char m[100];
    int i;
    if (n > 30) n = 30;
    for (i = 0; i < nseen; i++) if ((int)strlen(seen[i]) == n && !memcmp(seen[i], pre, n)) return;
    if (nseen < 8) { memcpy(seen[nseen], pre, n); seen[nseen++][n] = 0; }
    strcpy(m, "removing leading `");
    memcpy(m + 18, pre, n);
    strcpy(m + 18 + n, "' from member names");
    cu_err(0, NULL, m);
}

/* ---- numbers ---- */

static void putu(ULONG v, int width)
{
    char b[12];
    int i = 11;
    b[i] = 0;
    do { b[--i] = '0' + v % 10; v /= 10; } while (v);
    while (11 - i < width) { cu_putc(' '); width--; }
    cu_puts(b + i);
}

static int udigits(ULONG v) { int n = 1; while (v >= 10) v /= 10, n++; return n; }

static void octal(char *p, int w, ULONG v)
{
    int i;
    p[w - 1] = 0;
    for (i = w - 2; i >= 0; i--) { p[i] = '0' + (v & 7); v >>= 3; }
}

/* a header number: octal, or base-256 (GNU, for big ones); *big when it
 * does not fit 32 bits, *neg for a negative base-256 one */
static ULONG tnum(const UBYTE *p, int w, int *big, int *neg)
{
    ULONG v = 0;
    int i;
    if (p[0] & 0x80) {
        if (p[0] == 0xff) { if (neg) *neg = 1; return 0; }
        for (i = 1; i < w; i++) {
            if (v >> 24) { if (big) *big = 1; }
            v = (v << 8) | p[i];
        }
        if (p[0] & 0x7f) if (big) *big = 1;
        return v;
    }
    for (i = 0; i < w && (p[i] == ' ' || p[i] == '0'); i++) ;
    for (; i < w && p[i] >= '0' && p[i] <= '7'; i++) {
        if (v >> 29) { if (big) *big = 1; }
        v = (v << 3) | (p[i] - '0');
    }
    return v;
}

/* an archive's UTC seconds as the Amiga's local clock (ENV:TZ, else
 * Locale's offset); before 1978: the Amiga's first day */
static void seconds2ds(ULONG s, struct DateStamp *ds)
{
    cu_fromunix(s, ds);
}

/* ---- protection ---- */

static ULONG prot2mode(LONG prot, int dir)
{
    ULONG m = 0;
    if (dir) return 0755;
    if (!(prot & FIBF_READ)) m |= 0444;
    if (!(prot & FIBF_WRITE)) m |= 0222;
    if (!(prot & FIBF_EXECUTE)) m |= 0111;
    return m;
}

static LONG mode2prot(ULONG m)
{
    LONG p = 0;
    if (!(m & 0400)) p |= FIBF_READ;
    if (!(m & 0200)) p |= FIBF_WRITE | FIBF_DELETE;
    if (!(m & 0100)) p |= FIBF_EXECUTE;
    return p;
}

/* "hsparwed" (a letter: set for h s p a, allowed for r w e d) */
static int parseprot(const char *s, LONG *prot)
{
    static const char l[] = "hsparwed";
    LONG p = 0;
    int i;
    if (strlen(s) != 8) return 0;
    for (i = 0; i < 8; i++) {
        int on = lc(s[i]) == l[i];
        if (i < 4 ? on : !on) p |= 1L << (7 - i);
    }
    *prot = p;
    return 1;
}

/* ---- dos calls for paths of any length (cu_long.c) ---- */

static BPTR xopen(const char *p, LONG m) { BPTR f = Open((STRPTR)cu_enter(p), m); cu_leave(); return f; }
static BPTR xcreatedir(const char *p) { BPTR l = CreateDir((STRPTR)cu_enter(p)); cu_leave(); return l; }
static int xdelete(const char *p) { int r = DeleteFile((STRPTR)cu_enter(p)); cu_leave(); return r; }
static int xlstat(const char *p) { int r = cu_lstat(cu_enter(p), fib); cu_leave(); return r; }
static int xmakelink(const char *p, LONG to, LONG soft) { int r = MakeLink((STRPTR)cu_enter(p), to, soft); cu_leave(); return r; }

/* ======================================================== reading ==== */

static UBYTE *ibuf;
static LONG ipos, ilen;
static CzIn *zin;
static int ieof;
static ULONG iblock;             /* blocks read, for messages */

static LONG ifill(void)
{
    LONG n;
    if (ipos < ilen) return ilen - ipos;
    if (ieof) return 0;
    n = cz_read(zin, ibuf, IBUF);
    if (n < 0) { cu_err(10, archive, *cz_error() ? cz_error() : "read error"); ieof = 1; return 0; }
    if (!n) { ieof = 1; return 0; }
    ipos = 0;
    ilen = n;
    return n;
}

static LONG iget(void *d, LONG n)
{
    LONG got = 0;
    while (n > 0) {
        LONG a = ifill(), k;
        if (!a) break;
        k = a < n ? a : n;
        memcpy((UBYTE *)d + got, ibuf + ipos, k);
        ipos += k; got += k; n -= k;
    }
    return got;
}

/* after a member's data of size bytes: the padding to the next block */
static void ipad(ULONG size)
{
    ULONG pad = (512 - (size & 511)) & 511;
    while (pad) {
        LONG a = ifill(), k;
        if (!a) break;
        k = (ULONG)a < pad ? a : (LONG)pad;
        ipos += k; pad -= k;
    }
    iblock += (size + 511) / 512;
}

/* skip n bytes and the padding; 0 at a short archive */
static int iskip(ULONG n)
{
    ULONG left = n;
    while (left) {
        LONG a = ifill(), k;
        if (!a) { cu_err(10, archive, "unexpected end of the archive"); return 0; }
        k = (ULONG)a < left ? a : (LONG)left;
        ipos += k; left -= k;
    }
    ipad(n);
    return 1;
}

typedef struct {
    char *path, *linkpath, *comment;
    ULONG size, mtime;
    LONG prot;
    int hassize, hasmtime, hasprot, sparse, big;
} Pax;
static Pax px, pg;

static void paxclear(Pax *p)
{
    cu_free(p->path); cu_free(p->linkpath); cu_free(p->comment);
    memset(p, 0, sizeof *p);
}

static ULONG decnum(const char *s, int *big)
{
    ULONG v = 0;
    while (*s == ' ') s++;
    if (*s == '-') return 0;
    while (*s >= '0' && *s <= '9') {
        if (v > 429496729UL) *big = 1;
        v = v * 10 + (*s++ - '0');
    }
    return v;
}

static void paxset(char **to, const char *v)
{
    cu_free(*to);
    *to = cu_strdup(v);
}

/* the records "LEN key=value\n" of a pax header into p */
static void paxparse(char *b, LONG len, Pax *p, int global)
{
    LONG pos = 0;
    while (pos < len) {
        LONG n = 0, i = pos;
        char *rec, *kv, *eq;
        while (i < len && b[i] >= '0' && b[i] <= '9') n = n * 10 + (b[i++] - '0');
        if (i >= len || b[i] != ' ' || n < 5 || pos + n > len || b[pos + n - 1] != '\n') {
            if (b[pos]) cu_err(5, archive, "a damaged pax header (the rest of it ignored)");
            return;
        }
        rec = b + pos;
        kv = b + i + 1;
        rec[n - 1] = 0;
        if ((eq = strchr(kv, '='))) {
            *eq++ = 0;
            if (!strncmp(kv, "GNU.sparse.", 11)) p->sparse = 1;
            else if (!strcmp(kv, "mtime")) {
                int big = 0;
                p->mtime = decnum(eq, &big);
                p->hasmtime = 1;
            } else if (!strcmp(kv, "AMIGA.protect")) p->hasprot = parseprot(eq, &p->prot);
            else if (!strcmp(kv, "AMIGA.comment")) paxset(&p->comment, eq);
            else if (!global) {
                if (!strcmp(kv, "path")) paxset(&p->path, eq);
                else if (!strcmp(kv, "linkpath")) paxset(&p->linkpath, eq);
                else if (!strcmp(kv, "size")) { p->size = decnum(eq, &p->big); p->hassize = 1; }
            }
        }
        pos += n;
    }
}

/* the member being read (or written, for -v) */
static struct {
    char name[NAMEMAX], link[NAMEMAX], comment[256];
    char uname[33], gname[33];
    ULONG size, mode, uid, gid, mtime;
    LONG prot;
    int type, hasprot, toolong;
} ent;

static UBYTE hdr[512];

static int cksumok(const UBYTE *h)
{
    ULONG u = 0, stored;
    LONG s = 0;
    int i;
    for (i = 0; i < 512; i++) {
        UBYTE c = i >= 148 && i < 156 ? ' ' : h[i];
        u += c;
        s += (signed char)c;
    }
    stored = tnum(h + 148, 8, NULL, NULL);
    return stored == u || stored == (ULONG)s;
}

static void fieldcpy(char *d, const UBYTE *s, int n)
{
    int i;
    for (i = 0; i < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

/* a member's data (size bytes) read whole into a buffer (long names, pax) */
static char *readdata(ULONG size, ULONG max)
{
    char *b;
    if (size > max) {
        cu_err(5, archive, "an extended header too big to read, ignored");
        iskip(size);
        return NULL;
    }
    if (!(b = cu_alloc(size + 1))) { cu_err(20, NULL, "out of memory"); return NULL; }
    if ((ULONG)iget(b, size) != size) { cu_err(10, archive, "unexpected end of the archive"); cu_free(b); return NULL; }
    b[size] = 0;
    ipad(size);
    return b;
}

/* ---- listing ---- */

static int ugswidth = 19;

static void modestr(char *m, int type, ULONG mode)
{
    static const char rwx[] = "rwxrwxrwx";
    int i;
    m[0] = type == '5' ? 'd' : type == '2' ? 'l' : type == '1' ? 'h' : type == '3' ? 'c' :
           type == '4' ? 'b' : type == '6' ? 'p' : '-';
    for (i = 0; i < 9; i++) m[i + 1] = mode & (0400 >> i) ? rwx[i] : '-';
    if (mode & 04000) m[3] = m[3] == 'x' ? 's' : 'S';
    if (mode & 02000) m[6] = m[6] == 'x' ? 's' : 'S';
    if (mode & 01000) m[9] = m[9] == 'x' ? 't' : 'T';
    m[10] = 0;
}

static void listent(int longform, const char *name)
{
    if (longform) {
        char m[11], u[12], g[12], d[20];
        const char *user = ent.uname, *grp = ent.gname;
        struct DateStamp ds;
        int pad;
        modestr(m, ent.type, ent.mode);
        if (!*user) { char *p = u + 11; ULONG v = ent.uid; *p = 0; do *--p = '0' + v % 10; while (v /= 10); user = p; }
        if (!*grp) { char *p = g + 11; ULONG v = ent.gid; *p = 0; do *--p = '0' + v % 10; while (v /= 10); grp = p; }
        pad = strlen(user) + 1 + strlen(grp) + 1 + udigits(ent.size);
        if (pad > ugswidth) ugswidth = pad;
        cu_puts(m); cu_putc(' ');
        cu_puts(user); cu_putc('/'); cu_puts(grp);
        putu(ent.size, ugswidth - pad + udigits(ent.size) + 1);
        seconds2ds(ent.mtime, &ds);
        cu_isodate(&ds, d);
        d[16] = 0;
        cu_putc(' '); cu_puts(d); cu_putc(' ');
    }
    cu_puts(name);
    if (longform && ent.type == '2') { cu_puts(" -> "); cu_puts(ent.link); }
    else if (longform && ent.type == '1') { cu_puts(" link to "); cu_puts(ent.link); }
    cu_putc('\n');
}

/* ---- names made safe ---- */

/* --strip-components: 0 when nothing is left */
static int strip(char *n)
{
    long k = o_strip;
    char *p = n;
    if (!k) return 1;
    while (k > 0) {
        while (*p == '/') p++;
        if (!*p) return 0;
        while (*p && *p != '/') p++;
        while (*p == '/') p++;
        k--;
    }
    if (!*p) return 0;
    memmove(n, p, strlen(p) + 1);
    return 1;
}

/* the name to write under: no volume, no leading /, nothing up to a ..,
 * no empty or . parts, no : inside; 0 when nothing is left */
static int safename(const char *in, char *out)
{
    const char *p = in, *s, *c, *last = NULL, *q;
    int k = 0;
    s = strchr(p, '/');
    c = strchr(p, ':');
    if (c && (!s || c < s)) { notice(p, c + 1 - p); p = c + 1; }
    if (*p == '/') { notice("/", 1); while (*p == '/') p++; }
    for (q = p; *q;) {                    /* the last .. part */
        const char *e;
        for (e = q; *e && *e != '/'; e++) ;
        if (e - q == 2 && q[0] == '.' && q[1] == '.') last = e;
        q = e;
        while (*q == '/') q++;
    }
    if (last) {
        while (*last == '/') last++;
        notice(p, last - p);
        p = last;
    }
    while (*p) {
        const char *e;
        for (e = p; *e && *e != '/'; e++) ;
        if (!(e - p == 1 && *p == '.') && e > p) {
            if (k + (e - p) + 2 > NAMEMAX) return 0;
            if (k) out[k++] = '/';
            for (; p < e; p++) out[k++] = *p == ':' ? '_' : *p;
        }
        p = e;
        while (*p == '/') p++;
    }
    out[k] = 0;
    return k > 0;
}

/* ---- unpacking ---- */

typedef struct Later {
    struct Later *next;
    char *path, *target, *comment;
    LONG prot;
    struct DateStamp date;
} Later;
static Later *links, *linkstail, *dirs;

static char lastpar[NAMEMAX];

/* the file system kept the whole last part of path (FFS cuts names past 30 letters, silently) */
static int samename(const char *path, const char *got)
{
    const char *n = (const char *)FilePart((STRPTR)path);
    return strlen(n) == strlen(got) && eqi(n, got, strlen(n));
}

static void toolong(const char *path)
{
    cu_err(5, path, "a name too long for this file system, skipped");
}

/* the drawers above path, made when missing */
static int mkparents(const char *path)
{
    static char t[NAMEMAX];
    const char *sl = strrchr(path, '/');
    int n, i;
    BPTR l;
    if (!sl) return 1;
    n = sl - path;
    if ((int)strlen(lastpar) == n && !memcmp(lastpar, path, n)) return 1;
    memcpy(t, path, n);
    t[n] = 0;
    for (i = 0; i <= n; i++) {
        char c = t[i];
        if (c != '/' && c) continue;
        t[i] = 0;
        if ((l = cu_lockdir(t))) {
            int ok = Examine(l, fib), same = ok && samename(t, (char *)fib->fib_FileName);
            ok = ok && fib->fib_DirEntryType > 0;
            UnLock(l);
            if (!same) { toolong(t); return 0; }     /* found under a name cut short */
            if (!ok) { cu_err(10, t, "is not a drawer"); return 0; }
        } else if ((l = xcreatedir(t))) {
            int same = Examine(l, fib) && samename(t, (char *)fib->fib_FileName);
            UnLock(l);
            if (!same) { xdelete(t); toolong(t); return 0; }
        } else {
            if (IoErr() == ERROR_INVALID_COMPONENT_NAME)
                cu_err(5, t, "a name too long (or not allowed) on this file system, skipped");
            else cu_fault(10, t);
            return 0;
        }
        t[i] = c;
    }
    memcpy(lastpar, path, n);
    lastpar[n] = 0;
    return 1;
}

/* the parents made, and what is there already dealt with: 1 go on, 0 skip (said) */
static int prepare(const char *path, int wantdir, const char *what)
{
    LONG t;
    if (!mkparents(path)) return 0;
    if (!xlstat(path)) return 1;
    if (!samename(path, (char *)fib->fib_FileName)) { toolong(path); return 0; }   /* never the one with the cut name */
    t = fib->fib_DirEntryType;
    if (CU_ISDIR(t)) {
        if (wantdir) return 1;
        static char m[48];
        strcpy(m, "is a drawer, not replaced by ");
        strcat(m, what);
        cu_err(10, path, m);
        return 0;
    }
    if (o_keep) {
        if (!o_quietkeep) cu_err(5, path, "is already there, kept (-k)");
        return 0;
    }
    if (!xdelete(path)) {
        if (IoErr() == ERROR_DELETE_PROTECTED) cu_err(10, path, "is delete-protected, not replaced");
        else cu_fault(10, path);
        return 0;
    }
    lastpar[0] = 0;
    return 1;
}

static void setmeta(const char *path, LONG prot, const struct DateStamp *ds, const char *note)
{
    const char *n = cu_enter(path);
    if (note && *note) SetComment((STRPTR)n, (STRPTR)note);
    SetProtection((STRPTR)n, prot);
    if (!o_nodate) SetFileDate((STRPTR)n, (struct DateStamp *)ds);
    cu_leave();
}

static Later *later(const char *path, const char *target)
{
    int pl = strlen(path) + 1, tl = target ? strlen(target) + 1 : 0, cl = strlen(ent.comment) + 1;
    Later *l = cu_alloc(sizeof *l + pl + tl + cl);
    if (!l) { cu_err(20, NULL, "out of memory"); return NULL; }
    memset(l, 0, sizeof *l);
    l->path = (char *)(l + 1);
    strcpy(l->path, path);
    l->comment = l->path + pl;
    strcpy(l->comment, ent.comment);
    if (target) { l->target = l->comment + cl; strcpy(l->target, target); }
    return l;
}

static int fullname(const char *path, struct FileInfoBlock *f)
{
    if (samename(path, (char *)f->fib_FileName)) return 1;
    toolong(path);
    return 0;
}

static void member_prot(int dir, LONG *prot)
{
    *prot = ent.hasprot ? ent.prot : dir ? 0 : mode2prot(ent.mode);
}

static void xdir(const char *path)
{
    BPTR l;
    Later *d;
    if (!prepare(path, 1, "")) return;
    if ((l = xcreatedir(path))) {
        int ok = Examine(l, fib) && fullname(path, fib);
        UnLock(l);
        if (!ok) { xdelete(path); return; }
    } else if (IoErr() == ERROR_OBJECT_EXISTS) {
        if ((l = cu_lockdir(path))) {
            int ok = Examine(l, fib) && fullname(path, fib);
            UnLock(l);
            if (!ok) return;
        }
    } else {
        if (IoErr() == ERROR_INVALID_COMPONENT_NAME)
            cu_err(5, path, "a name too long (or not allowed) on this file system, skipped");
        else cu_fault(10, path);
        return;
    }
    if (!(d = later(path, NULL))) return;
    member_prot(1, &d->prot);
    seconds2ds(ent.mtime, &d->date);
    d->next = dirs;
    dirs = d;
}

static void xfile(const char *path)
{
    BPTR fh;
    ULONG left = ent.size;
    int bad = 0;
    LONG prot;
    struct DateStamp ds;
    Later *l;
    for (l = links; l; l = l->next)                  /* a soft link of the same name to come: not any more */
        if (l->target && !strcmp(l->path, path)) l->target = NULL;
    if (!prepare(path, 0, "a file")) { iskip(ent.size); return; }
    if (!(fh = xopen(path, MODE_NEWFILE))) {
        if (IoErr() == ERROR_INVALID_COMPONENT_NAME)
            cu_err(5, path, "a name too long (or not allowed) on this file system, skipped");
        else cu_fault(10, path);
        iskip(ent.size);
        return;
    }
    if (ExamineFH(fh, fib) && !fullname(path, fib)) {
        Close(fh);
        xdelete(path);
        iskip(ent.size);
        return;
    }
    while (left) {
        LONG a = ifill(), k;
        if (!a) { cu_err(10, archive, "unexpected end of the archive"); bad = 2; break; }
        k = (ULONG)a < left ? a : (LONG)left;
        if (!bad && Write(fh, ibuf + ipos, k) != k) { cu_fault(10, path); bad = 1; }
        ipos += k;
        left -= k;
        if (cu_break()) { bad = 2; break; }
    }
    if (!Close(fh) && !bad) { cu_fault(10, path); bad = 1; }
    if (bad == 2) ieof = 1;                          /* the archive ended, or Ctrl-C: stop */
    else ipad(ent.size);
    if (bad) { xdelete(path); return; }
    member_prot(0, &prot);
    seconds2ds(ent.mtime, &ds);
    setmeta(path, prot, &ds, ent.comment);
}

static void xhard(const char *path, const char *target)
{
    BPTR l;
    Later *s;
    if (!strcmp(path, target)) return;
    if (!prepare(path, 0, "a link")) return;
    for (s = links; s; s = s->next)                  /* a link to a soft link still to be made */
        if (s->target && !strcmp(s->path, target)) {
            Later *n = later(path, s->target);
            if (n) { linkstail->next = n; linkstail = n; }
            return;
        }
    if (!(l = cu_lockdir(target))) {
        cu_err(5, path, "its link target was not unpacked, skipped");
        return;
    }
    if (xmakelink(path, (LONG)l, LINK_HARD)) { UnLock(l); return; }
    if (!Examine(l, fib)) { cu_fault(10, target); UnLock(l); return; }
    UnLock(l);
    cu_copyfile(target, path, fib->fib_Protection, &fib->fib_Date, (char *)fib->fib_Comment);
}

static void xsoft(const char *path)
{
    static char t[NAMEMAX];
    Later *l;
    unix2amiga(ent.link, t, sizeof t);
    if (!*t) { cu_err(5, path, "a soft link to nothing, skipped"); return; }
    if (!(l = later(path, t))) return;
    if (linkstail) linkstail->next = l; else links = l;
    linkstail = l;
}

static void makelinks(void)
{
    Later *l;
    for (l = links; l; l = l->next) {
        if (!l->target) continue;
        if (cu_rc >= 20) break;
        if (!prepare(l->path, 0, "a link")) continue;
        if (!xmakelink(l->path, (LONG)l->target, LINK_SOFT)) {
            if (IoErr() == ERROR_ACTION_NOT_KNOWN) cu_err(5, l->path, "this file system has no soft links, skipped");
            else cu_fault(10, l->path);
        }
    }
}

static void setdirs(void)
{
    Later *d;
    for (d = dirs; d; d = d->next) setmeta(d->path, d->prot, &d->date, d->comment);
}

/* -O: the data to the output */
static void tostdout(void)
{
    BPTR out = Output();
    ULONG left = ent.size;
    cu_flush();
    while (left) {
        LONG a = ifill(), k;
        if (!a) { cu_err(10, archive, "unexpected end of the archive"); return; }
        k = (ULONG)a < left ? a : (LONG)left;
        if (Write(out, ibuf + ipos, k) != k) { cu_fault(10, "output"); ieof = 1; return; }
        ipos += k;
        left -= k;
        if (cu_break()) { ieof = 1; return; }
    }
    ipad(ent.size);
}

static void member(int sparse)
{
    static char path[NAMEMAX], lpath[NAMEMAX];
    int t = ent.type, n = strlen(ent.name);
    if ((t == '0' || !t || t == '7') && n && ent.name[n - 1] == '/') t = ent.type = '5';
    if (t == 'D') t = ent.type = '5';
    if (t == 'V' || t == 'M') { iskip(ent.size); return; }
    if (!selected(ent.name) || excluded(ent.name)) { iskip(ent.size); return; }
    if (!strip(ent.name)) { iskip(ent.size); return; }
    if (t == '1' && !strip(ent.link)) { iskip(ent.size); return; }
    if (mode == 't') {
        listent(o_verbose, ent.name);
        iskip(ent.size);
        return;
    }
    if (ent.toolong) { cu_err(5, ent.name, "a name too long, skipped"); iskip(ent.size); return; }
    if (!safename(ent.name, path)) {
        if (t != '5') cu_err(5, ent.name, "no name left to unpack under, skipped");
        iskip(ent.size);
        return;
    }
    if (o_stdout) {
        if (t == '0' || !t || t == '7' || (t >= 'A' && t <= 'Z' && t != 'S' && t != 'L' && t != 'K')) tostdout();
        else iskip(ent.size);
        return;
    }
    if (o_verbose) {
        BPTR o = cu_out;
        cu_out = verr;
        if (t == '5') {
            int k = strlen(path);
            path[k] = '/'; path[k + 1] = 0;
            listent(o_verbose > 1, path);
            path[k] = 0;
        } else listent(o_verbose > 1, path);
        cu_flush();
        cu_out = o;
    }
    switch (t) {
    case '5':
        xdir(path);
        iskip(ent.size);
        break;
    case '2':
        xsoft(path);
        iskip(ent.size);
        break;
    case '1':
        if (!safename(ent.link, lpath)) cu_err(5, path, "its link target has no name, skipped");
        else xhard(path, lpath);
        iskip(ent.size);
        break;
    case '3': case '4': case '6':
        cu_err(5, path, "a device or pipe, not made");
        iskip(ent.size);
        break;
    case 'S':
        cu_err(5, path, "a sparse file (not handled), skipped");
        iskip(ent.size);
        break;
    default:
        if (sparse) { cu_err(5, path, "a sparse file (not handled), skipped"); iskip(ent.size); break; }
        if (t != '0' && t && t != '7') {
            static char m[40];
            strcpy(m, "an unknown kind (x), unpacked as a file");
            m[16] = t > ' ' && t < 127 ? t : '?';
            cu_err(0, path, m);
        }
        xfile(path);
    }
}

/* the archive to read, opened before any -C */
static BPTR openin(int *own)
{
    BPTR fh;
    *own = 0;
    if (!strcmp(archive, "-")) {
        fh = Input();
        if (IsInteractive(fh)) { cu_err(10, NULL, "will not read an archive from a console (-f ARCHIVE)"); return 0; }
        return fh;
    }
    if (!(fh = Open((STRPTR)archive, MODE_OLDFILE))) cu_fault(10, archive);
    *own = fh != 0;
    return fh;
}

static int readarchive(BPTR fh, int own)
{
    int first = 1, lost = 0;
    char *longname = NULL, *longlink = NULL;
    if (!(ibuf = cu_alloc(IBUF))) { cu_err(20, NULL, "out of memory"); goto done; }
    if (!(zin = cz_open(fh, CZ_AUTO, NULL))) { cu_err(10, archive, cz_error()); goto done; }
    for (;;) {
        int big = 0, neg = 0, i;
        LONG got;
        if (cu_break()) break;
        got = iget(hdr, 512);
        if (!got) {
            if (first) cu_err(10, archive, "is empty");
            break;
        }
        if (got < 512) { cu_err(10, archive, first ? "is not a tar archive" : "unexpected end of the archive"); break; }
        for (i = 0; i < 512 && !hdr[i]; i++) ;
        if (i == 512) break;                         /* the end */
        if (!cksumok(hdr)) {
            static char m[90];
            char b[12];
            int k = 11;
            ULONG v = iblock;
            if (first) { cu_err(10, archive, "is not a tar archive"); break; }
            iblock++;
            if (lost) continue;                      /* GNU's way: on to the next good header */
            lost = 1;
            b[k] = 0;
            do b[--k] = '0' + v % 10; while (v /= 10);
            strcpy(m, "damaged: a wrong header at block ");
            strcat(m, b + k);
            strcat(m, ", skipping to the next one");
            cu_err(10, archive, m);
            paxclear(&px);
            cu_free(longname); cu_free(longlink);
            longname = longlink = NULL;
            continue;
        }
        first = lost = 0;
        iblock++;
        memset(&ent, 0, sizeof ent);
        ent.type = hdr[156];
        ent.size = tnum(hdr + 124, 12, &big, NULL);
        ent.mode = tnum(hdr + 100, 8, NULL, NULL) & 07777;
        ent.uid = tnum(hdr + 108, 8, NULL, NULL);
        ent.gid = tnum(hdr + 116, 8, NULL, NULL);
        ent.mtime = tnum(hdr + 136, 12, NULL, &neg);
        if (neg) ent.mtime = 0;
        if (px.hassize) { ent.size = px.size; big = px.big; }
        if (big || ent.size > 0xfffffc00UL) {
            cu_err(10, archive, "a member too big for this tar (over 4 GB), stopped");
            break;
        }
        if (ent.type == 'x' || ent.type == 'g') {
            char *b = readdata(ent.size, PAXMAX);
            if (b) { paxparse(b, ent.size, ent.type == 'x' ? &px : &pg, ent.type == 'g'); cu_free(b); }
            if (cu_rc >= 20) break;
            continue;
        }
        if (ent.type == 'L' || ent.type == 'K') {
            char *b = readdata(ent.size, PAXMAX);
            if (!b) { if (cu_rc >= 10) break; continue; }
            if (ent.type == 'L') { cu_free(longname); longname = b; }
            else { cu_free(longlink); longlink = b; }
            continue;
        }
        if (ent.type == 'S' && hdr[482]) {          /* old GNU sparse: more maps follow */
            UBYTE x[512];
            do { if (iget(x, 512) != 512) break; iblock++; } while (x[504]);
        }
        /* the name */
        {
            const char *nm = px.path ? px.path : longname;
            if (nm) {
                if (strlen(nm) >= NAMEMAX) ent.toolong = 1;
                strncpy(ent.name, nm, NAMEMAX - 1);
            } else {
                char nb[101], pre[156];
                fieldcpy(nb, hdr, 100);
                if (!memcmp(hdr + 257, "ustar\0", 6) && hdr[345]) {
                    fieldcpy(pre, hdr + 345, 155);
                    strcpy(ent.name, pre);
                    strcat(ent.name, "/");
                    strcat(ent.name, nb);
                } else strcpy(ent.name, nb);
            }
            nm = px.linkpath ? px.linkpath : longlink;
            if (nm) strncpy(ent.link, nm, NAMEMAX - 1);
            else fieldcpy(ent.link, hdr + 157, 100);
        }
        if (!memcmp(hdr + 257, "ustar", 5)) {
            fieldcpy(ent.uname, hdr + 265, 32);
            fieldcpy(ent.gname, hdr + 297, 32);
        }
        if (px.hasmtime) ent.mtime = px.mtime;
        else if (pg.hasmtime) ent.mtime = pg.mtime;
        if (px.hasprot) { ent.prot = px.prot; ent.hasprot = 1; }
        else if (pg.hasprot) { ent.prot = pg.prot; ent.hasprot = 1; }
        {
            const char *c = px.comment ? px.comment : pg.comment;
            if (c) { strncpy(ent.comment, c, sizeof ent.comment - 1); fromutf8(ent.comment); }
        }
        fromutf8(ent.name);
        fromutf8(ent.link);
        member(px.sparse);
        paxclear(&px);
        cu_free(longname); cu_free(longlink);
        longname = longlink = NULL;
        if (cu_rc >= 20) break;
    }
    while (cu_rc < 20 && ifill()) ipos = ilen;      /* the rest (record padding): read, so a squeezer can check its end */
    paxclear(&px);
    paxclear(&pg);
    cu_free(longname); cu_free(longlink);
done:
    if (zin) cz_close(zin);
    if (own) Close(fh);
    return 1;
}

/* ======================================================== writing ==== */

static UBYTE *obuf;
static LONG olen;
static ULONG ototal;
static CzOut *zout;
static int werr;

static void oflush(void)
{
    if (olen && !werr && cz_write(zout, obuf, olen) != olen) {
        werr = 1;
        cu_err(10, archive, *cz_error() ? cz_error() : "write error");
    }
    ototal += olen;
    olen = 0;
}

static void owrite(const void *p, LONG n)
{
    while (n > 0) {
        LONG k = OBUF - olen;
        if (k > n) k = n;
        memcpy(obuf + olen, p, k);
        olen += k; n -= k;
        p = (const UBYTE *)p + k;
        if (olen == OBUF) oflush();
    }
}

static void ozero(ULONG n)
{
    while (n > 0) {
        LONG k = OBUF - olen;
        if ((ULONG)k > n) k = n;
        memset(obuf + olen, 0, k);
        olen += k; n -= k;
        if (olen == OBUF) oflush();
    }
}

static void opad(ULONG size) { ozero((512 - (size & 511)) & 511); }

static char *pxb;
static LONG pxlen, pxcap;

static int paxadd(const char *key, const char *val)
{
    LONG kl = strlen(key), vl = strlen(val), n = kl + vl + 3, len;
    char d[12];
    int k = 11;
    ULONG v;
    len = n + udigits(n);
    if (udigits(len) != udigits(n)) len = n + udigits(len);
    if (pxlen + len > pxcap) {
        char *t = cu_alloc(pxcap = (pxlen + len) * 2 + 256);
        if (!t) { cu_err(20, NULL, "out of memory"); return 0; }
        if (pxb) { memcpy(t, pxb, pxlen); cu_free(pxb); }
        pxb = t;
    }
    v = len; d[k] = 0;
    do d[--k] = '0' + v % 10; while (v /= 10);
    memcpy(pxb + pxlen, d + k, 11 - k); pxlen += 11 - k;
    pxb[pxlen++] = ' ';
    memcpy(pxb + pxlen, key, kl); pxlen += kl;
    pxb[pxlen++] = '=';
    memcpy(pxb + pxlen, val, vl); pxlen += vl;
    pxb[pxlen++] = '\n';
    return 1;
}

static void header(const char *name, int nlen, const char *prefix, int plen, ULONG mode, ULONG size,
                   ULONG mtime, int type, const char *link)
{
    ULONG sum = 0;
    int i;
    memset(hdr, 0, 512);
    memcpy(hdr, name, nlen > 100 ? 100 : nlen);
    octal((char *)hdr + 100, 8, mode);
    octal((char *)hdr + 108, 8, 0);
    octal((char *)hdr + 116, 8, 0);
    octal((char *)hdr + 124, 12, size);
    octal((char *)hdr + 136, 12, mtime);
    hdr[156] = type;
    if (link) { int l = strlen(link); memcpy(hdr + 157, link, l > 100 ? 100 : l); }
    memcpy(hdr + 257, "ustar\0" "00", 8);
    octal((char *)hdr + 329, 8, 0);
    octal((char *)hdr + 337, 8, 0);
    if (plen) memcpy(hdr + 345, prefix, plen);
    memset(hdr + 148, ' ', 8);
    for (i = 0; i < 512; i++) sum += hdr[i];
    octal((char *)hdr + 148, 7, sum);
    hdr[155] = ' ';
    owrite(hdr, 512);
}

/* a member's headers (pax when needed, then ustar); name in Latin-1 */
static int putentry(const char *name, int type, ULONG size, LONG prot, const struct DateStamp *ds,
                    const char *link, const char *note)
{
    static char un[NAMEMAX * 2 + 2], ul[NAMEMAX * 2], pt[NAMEMAX * 2];
    struct DateStamp fixed;
    int nl, ll = 0, split = -1, dir = type == '5';
    ULONG m = type == '2' ? 0777 : prot2mode(prot, dir), mt;
    fixed = *ds;
    cu_dsfix(&fixed);
    mt = cu_unixtime(&fixed);
    if ((nl = toutf8(name, un, NAMEMAX * 2)) < 0) { cu_err(5, name, "a name too long, not stored"); return 1; }
    if (dir) { un[nl++] = '/'; un[nl] = 0; }
    if (link && (ll = toutf8(link, ul, sizeof ul)) < 0) { cu_err(5, name, "a link too long, not stored"); return 1; }
    pxlen = 0;
    if (nl > 100) {                                  /* ustar's prefix: split at a / */
        int i;
        for (i = nl - 101; i < nl - 1 && i <= 155; i++)
            if (i > 0 && un[i] == '/') { split = i; break; }
        if (split < 0 && !paxadd("path", un)) return 0;
    }
    if (ll > 100 && !paxadd("linkpath", ul)) return 0;
    if (type == '0' || dir) {
        if ((prot & 0xff) != (dir ? 0 : mode2prot(m))) {
            char p[9];
            cu_protstr(prot, p);
            if (!paxadd("AMIGA.protect", p)) return 0;
        }
        if (note && *note) {
            char c[200];
            if (toutf8(note, c, sizeof c) >= 0 && !paxadd("AMIGA.comment", c)) return 0;
        }
    }
    if (pxlen) {
        const char *base = un;
        int bl, k;
        for (k = 0; k < nl - 1; k++) if (un[k] == '/') base = un + k + 1;
        strcpy(pt, "PaxHeaders/");
        bl = strlen(base);
        if (bl > 88) bl = 88;
        memcpy(pt + 11, base, bl);
        pt[11 + bl] = 0;
        header(pt, strlen(pt), NULL, 0, 0644, pxlen, mt, 'x', NULL);
        owrite(pxb, pxlen);
        opad(pxlen);
    }
    if (split >= 0) header(un + split + 1, nl - split - 1, un, split, m, size, mt, type, ul);
    else header(un, nl, NULL, 0, m, size, mt, type, link ? ul : NULL);
    if (o_verbose) {
        BPTR o = cu_out;
        cu_out = verr;
        if (o_verbose > 1) {
            memset(&ent, 0, sizeof ent);
            ent.type = type; ent.mode = m; ent.size = size; ent.mtime = mt;
            if (link) strncpy(ent.link, link, NAMEMAX - 1);
        }
        {
            static char vn[NAMEMAX + 2];
            strcpy(vn, name);
            if (dir) strcat(vn, "/");
            listent(o_verbose > 1, vn);
        }
        cu_flush();
        cu_out = o;
    }
    return !werr;
}

/* ---- the tree ---- */

static char start[CU_PATH], argbase[NAMEMAX], canon[CU_PATH], an[NAMEMAX];
static int startlen, havecanon;
static char archcanon[CU_PATH];
static LONG archkey;

typedef struct Hl { struct Hl *next; char *aname; char rel[1]; } Hl;
static Hl *hls;

static void freehls(void)
{
    while (hls) { Hl *n = hls->next; cu_free(hls); hls = n; }
}

static void addhl(const char *rel, const char *aname)
{
    Hl *h = cu_alloc(sizeof *h + strlen(rel) + strlen(aname) + 1);
    if (!h) return;
    strcpy(h->rel, rel);
    h->aname = h->rel + strlen(rel) + 1;
    strcpy(h->aname, aname);
    h->next = hls;
    hls = h;
}

static Hl *findhl(const char *rel)
{
    Hl *h;
    int n = strlen(rel);
    for (h = hls; h; h = h->next) if ((int)strlen(h->rel) == n && eqi(h->rel, rel, n)) return h;
    return NULL;
}

/* the order cu_walk goes in: part by part, naturally */
static int walkcmp(const char *a, const char *b)
{
    static char x[CU_PATH], y[CU_PATH];
    for (;;) {
        int i = 0, j = 0, c;
        while (*a && *a != '/') x[i++] = *a++;
        while (*b && *b != '/') y[j++] = *b++;
        x[i] = y[j] = 0;
        if (!i && !j) return 0;
        if (!i) return -1;
        if (!j) return 1;
        if ((c = cu_natcmp(x, y))) return c;
        while (*a == '/') a++;
        while (*b == '/') b++;
    }
}

/* the archive name of a path relative to the start */
static int mkname(char *to, const char *rel)
{
    int b = strlen(argbase), r = strlen(rel);
    if (b + r + 2 > NAMEMAX) return 0;
    strcpy(to, argbase);
    if (b && r) to[b++] = '/';
    strcpy(to + b, rel);
    return 1;
}

static int readlinkat(const char *path, char *to, int size)
{
    static char dir[CU_PATH];
    const char *nm = cu_enter(path);
    int n = (const char *)PathPart((STRPTR)nm) - nm;
    BPTR l;
    LONG r, e;
    memcpy(dir, nm, n);
    dir[n] = 0;
    if (!(l = Lock(dir, SHARED_LOCK))) { cu_leave(); return 0; }
    SetIoErr(0);
    r = ReadLink(((struct FileLock *)BADDR(l))->fl_Task, l, FilePart((STRPTR)nm), to, size);
    e = IoErr();
    UnLock(l);
    cu_leave();
    return r > 0 || (r == -1 && !e);
}

static void addsoft(const char *path, const struct DateStamp *ds)
{
    static char t[CU_PATH], u[CU_PATH];
    if (!readlinkat(path, t, sizeof t)) { cu_fault(5, path); return; }
    amiga2unix(t, u, sizeof u);
    putentry(an, '2', 0, 0, ds, u, NULL);
}

static void adddir(const char *path, LONG prot, const struct DateStamp *ds)
{
    BPTR l = cu_lockdir(path);
    const char *note = NULL;
    if (l && Examine(l, fib)) note = (char *)fib->fib_Comment;
    putentry(an, '5', 0, prot, ds, NULL, note);
    if (l) UnLock(l);
}

/* is this (failed to open, in use) the archive being written? */
static int isarchive(const char *path)
{
    static char t[CU_PATH];
    int n = (const char *)PathPart((STRPTR)path) - path;
    BPTR l;
    if (!archcanon[0]) return 0;
    memcpy(t, path, n);
    t[n] = 0;
    if (!(l = cu_lockdir(t))) return 0;
    n = NameFromLock(l, t, CU_PATH) && AddPart(t, FilePart((STRPTR)path), CU_PATH);
    UnLock(l);
    return n && strlen(t) == strlen(archcanon) && eqi(t, archcanon, strlen(t));
}

/* a hard link member: the file's own protection and date */
static void puthard(const char *path, const char *to)
{
    BPTR l = cu_lockdir(path);
    struct DateStamp ds;
    LONG prot = 0;
    DateStamp(&ds);
    if (l) {
        if (Examine(l, fib)) { prot = fib->fib_Protection; ds = fib->fib_Date; }
        UnLock(l);
    }
    putentry(an, '1', 0, prot, &ds, to, NULL);
}

static void addfile(const char *path, LONG type, const char *rel)
{
    static char real[CU_PATH], rname[NAMEMAX];
    BPTR fh;
    ULONG size, left;
    Hl *h;
    if (type == ST_LINKFILE && havecanon) {          /* a hard link: which file is it? */
        BPTR l = cu_lockdir(path);
        int cl = strlen(canon), in = 0;
        const char *rr = NULL;
        if (l) { in = NameFromLock(l, real, CU_PATH); UnLock(l); }
        if (in && (int)strlen(real) > cl && eqi(real, canon, cl) && (canon[cl - 1] == ':' || real[cl] == '/'))
            rr = real + cl + (canon[cl - 1] != ':');
        if (rr) {
            if ((h = findhl(rr))) { puthard(path, h->aname); return; }
            if (!o_norec && walkcmp(rr, rel) < 0 && mkname(rname, rr) && !excluded(rname)) {
                addhl(rr, rname);                    /* the file itself is stored already */
                puthard(path, rname);
                return;
            }
            addhl(rr, an);                           /* stored here: the file itself will link to this */
        }
    } else if (hls && (h = findhl(rel))) {
        puthard(path, h->aname);
        return;
    }
    if (!(fh = xopen(path, MODE_OLDFILE))) {
        if (IoErr() == ERROR_OBJECT_IN_USE && isarchive(path)) { cu_err(0, path, "is the archive itself, not stored"); return; }
        cu_fault(5, path);
        return;
    }
    if (!ExamineFH(fh, fib)) { cu_fault(5, path); Close(fh); return; }
    if (archkey && fib->fib_DiskKey == archkey && isarchive(path)) {
        cu_err(0, path, "is the archive itself, not stored");
        Close(fh);
        return;
    }
    size = left = fib->fib_Size;
    if (!putentry(an, '0', size, fib->fib_Protection, &fib->fib_Date, NULL, (char *)fib->fib_Comment)) { Close(fh); return; }
    while (left && !werr) {
        LONG k = OBUF - olen, n;
        if ((ULONG)k > left) k = left;
        n = Read(fh, obuf + olen, k);
        if (n <= 0) {
            if (n < 0) cu_fault(5, path);
            else cu_err(5, path, "shrank while it was read (filled with zeros)");
            ozero(left);
            break;
        }
        olen += n;
        left -= n;
        if (olen == OBUF) oflush();
        if (cu_break()) break;
    }
    opad(size);
    Close(fh);
}

static int addone(CuEnt *e, void *u)
{
    const char *rel = e->path + startlen;
    (void)u;
    if (!e->depth) rel = "";
    else while (*rel == '/') rel++;
    if (!mkname(an, rel)) { cu_err(5, e->path, "a name too long, not stored"); return CU_SKIP; }
    if (!*an) return 0;                              /* a volume's root: only what is in it */
    if (excluded(an)) return CU_SKIP;
    if (cu_isdir(e->type)) adddir(e->path, e->prot, &e->date);
    else if (e->type == ST_SOFTLINK) addsoft(e->path, &e->date);
    else if (e->type == ST_LINKDIR) cu_err(5, e->path, "a hard link to a drawer, not stored");
    else addfile(e->path, e->type, rel);
    return cu_rc >= 20 || werr ? CU_STOP : 0;
}

/* the archive name of an argument: the volume dropped, "/" (up) resolved */
static int baseof(const char *arg, char *out)
{
    const char *p = strchr(arg, ':');
    int k = 0, ups = 0;
    if (p) { notice(arg, p + 1 - arg); arg = p + 1; }
    while (*arg) {
        if (*arg == '/') {                           /* up: the part before goes */
            if (k) { while (k && out[k - 1] != '/') k--; if (k) k--; }
            else ups = 1;
            arg++;
            continue;
        }
        if (k) { if (k + 1 >= NAMEMAX) return 0; out[k++] = '/'; }
        while (*arg && *arg != '/') { if (k + 1 >= NAMEMAX) return 0; out[k++] = *arg++; }
        if (*arg == '/') arg++;
    }
    out[k] = 0;
    if (ups) notice("/", 1);
    return 1;
}

static int addarg(const char *arg, void *u)
{
    BPTR l;
    (void)u;
    if (strlen(arg) >= CU_PATH) { cu_err(5, arg, "a name too long"); return 0; }
    strcpy(start, arg);
    cu_trimslash(start);
    if (!baseof(start, argbase)) { cu_err(5, arg, "a name too long"); return 0; }
    if (!cu_lstat(start, fib)) { cu_fault(5, arg); return 0; }
    if (fib->fib_DirEntryType == ST_SOFTLINK) {
        struct DateStamp ds;
        DateStamp(&ds);
        strcpy(an, argbase);
        if (*an && !excluded(an)) addsoft(start, &ds);
        return cu_rc >= 20 || werr;
    }
    havecanon = 0;
    if ((l = Lock(start, SHARED_LOCK))) {
        havecanon = NameFromLock(l, canon, CU_PATH) != 0;
        UnLock(l);
    }
    freehls();
    startlen = strlen(start);
    cu_walk(start, 0, o_norec ? 0 : -1, addone, NULL);
    freehls();
    return cu_rc >= 20 || werr;
}

static BPTR olddir;
static int moved;

static int chdir_(const char *d)
{
    BPTR l = Lock((STRPTR)d, SHARED_LOCK), prev;
    if (!l) { cu_fault(10, d); return 0; }
    if (!Examine(l, fib) || fib->fib_DirEntryType <= 0) { cu_err(10, d, "is not a drawer"); UnLock(l); return 0; }
    prev = CurrentDir(l);
    if (!moved) { olddir = prev; moved = 1; } else UnLock(prev);
    lastpar[0] = 0;
    return 1;
}

static int createarchive(void)
{
    BPTR fh;
    int own = 0, i, any = 0;
    for (i = 0; i < nwords; i++) if (!words[i].isdir) any = 1;
    if (!any) { cu_err(10, NULL, "will not make an empty archive (no names given)"); return 0; }
    if (kind != CZ_NONE && !cz_can_write(kind)) {
        static char m[60];
        strcpy(m, "this tar cannot pack ");
        strcat(m, cz_kind_name(kind));
        strcat(m, " yet");
        cu_err(10, archive, m);
        return 0;
    }
    if (!strcmp(archive, "-")) {
        fh = Output();
        if (IsInteractive(fh)) { cu_err(10, NULL, "will not write an archive to a console (-f ARCHIVE)"); return 0; }
    } else {
        if (!(fh = Open((STRPTR)archive, MODE_NEWFILE))) { cu_fault(10, archive); return 0; }
        own = 1;
        if (ExamineFH(fh, fib)) archkey = fib->fib_DiskKey;
        if (!NameFromFH(fh, archcanon, CU_PATH)) archcanon[0] = 0;
    }
    if (!(obuf = cu_alloc(OBUF))) { cu_err(20, NULL, "out of memory"); goto done; }
    if (!(zout = cz_create(fh, kind, 0))) { cu_err(10, archive, cz_error()); goto done; }
    for (i = 0; i < nwords && cu_rc < 20 && !werr; i++) {
        if (words[i].isdir) { if (!chdir_(words[i].s)) break; }
        else if (cu_each(words[i].s, addarg, NULL)) break;
    }
    if (!werr && cu_rc < 20) {
        ULONG t;
        ozero(1024);                                 /* the end: two zero blocks */
        t = (ototal + olen) % RECORD;
        if (t) ozero(RECORD - t);
    }
    oflush();
    if (cz_finish(zout) < 0 && !werr) { cu_err(10, archive, cz_error()); werr = 1; }
    zout = NULL;
done:
    if (own) {
        if (!Close(fh) && !werr) cu_fault(10, archive);
        if (werr || cu_rc >= 20) {
            if (moved) { UnLock(CurrentDir(olddir)); moved = 0; }
            DeleteFile((STRPTR)archive);
        }
    }
    return 1;
}

/* ======================================================== the command line ==== */

enum { L_ZSTD = 256, L_EXCL, L_STRIP, L_NOREC, L_REC, L_SKIPOLD };

static int setmode(int m)
{
    if (mode && mode != m) { cu_err(10, NULL, "only one of -c, -x, -t"); return 0; }
    mode = m;
    return 1;
}

static int apply(int c, char *v)
{
    int ok;
    switch (c) {
    case 'c': case 'x': case 't': return setmode(c);
    case 'f': archive = v; return 1;
    case 'v': o_verbose++; return 1;
    case 'z': kind = CZ_GZIP; return 1;
    case 'j': kind = CZ_BZIP2; return 1;
    case 'J': kind = CZ_XZ; return 1;
    case L_ZSTD: kind = CZ_ZSTD; return 1;
    case 'a': o_auto = 1; return 1;
    case 'k': o_keep = 1; return 1;
    case L_SKIPOLD: o_keep = o_quietkeep = 1; return 1;
    case 'O': o_stdout = 1; return 1;
    case 'm': o_nodate = 1; return 1;
    case 'p': return 1;
    case 'C': return addword(v, 1);
    case 'T': {
        static CuIn in;
        char *line;
        int nl;
        LONG n;
        if (!cu_open(&in, v)) return 0;
        while ((n = cu_line(&in, &line, &nl)) >= 0) {
            while (n && (line[n - 1] == '\r' || line[n - 1] == ' ')) line[--n] = 0;
            if (n && !addword(line, 0)) { cu_close(&in); return 0; }
        }
        cu_close(&in);
        return 1;
    }
    case L_EXCL: return addexcl(v);
    case L_STRIP:
        o_strip = cu_num(v, &ok);
        if (!ok || o_strip < 0) { cu_err(10, v, "not a number of parts"); return 0; }
        return 1;
    case L_NOREC: o_norec = 1; return 1;
    case L_REC: o_norec = 0; return 1;
    }
    return 0;
}

static const struct { const char *name; int c, val; } longs[] = {
    { "create", 'c', 0 }, { "extract", 'x', 0 }, { "get", 'x', 0 }, { "list", 't', 0 },
    { "file", 'f', 1 }, { "directory", 'C', 1 }, { "verbose", 'v', 0 },
    { "gzip", 'z', 0 }, { "gunzip", 'z', 0 }, { "ungzip", 'z', 0 }, { "bzip2", 'j', 0 },
    { "xz", 'J', 0 }, { "zstd", L_ZSTD, 0 }, { "auto-compress", 'a', 0 },
    { "keep-old-files", 'k', 0 }, { "skip-old-files", L_SKIPOLD, 0 }, { "to-stdout", 'O', 0 },
    { "preserve-permissions", 'p', 0 }, { "same-permissions", 'p', 0 }, { "touch", 'm', 0 },
    { "files-from", 'T', 1 }, { "exclude", L_EXCL, 1 }, { "strip-components", L_STRIP, 1 },
    { "no-recursion", L_NOREC, 0 }, { "recursion", L_REC, 0 }, { NULL, 0, 0 }
};

static const char shorts[] = "cxtf:vzjJakOmpC:T:";

static int needsval(int c)
{
    const char *s = strchr(shorts, c);
    return s && s[1] == ':';
}

static int badopt(const char *w, const char *why)
{
    cu_err(10, w, why);
    return 0;
}

static int parse(void)
{
    int i = 0, noopts = 0;
    char **av = cu_argv;
    int ac = cu_argc;
    if (ac && av[0][0] != '-') {                     /* the old bundle: tar xzvf f.tgz */
        const char *p = av[0];
        i = 1;
        for (; *p; p++) {
            char t[3] = { '-', *p, 0 };
            if (!strchr(shorts, *p) || *p == ':') return badopt(t, "unknown option");
            if (needsval(*p)) {
                if (i >= ac) return badopt(t, "needs a value");
                if (!apply(*p, av[i++])) return 0;
            } else if (!apply(*p, NULL)) return 0;
        }
    }
    for (; i < ac; i++) {
        char *w = av[i];
        if (!noopts && w[0] == '-' && w[1] == '-' && !w[2]) { noopts = 1; continue; }
        if (!noopts && w[0] == '-' && w[1] == '-') {
            char *eq = strchr(w, '='), *v = NULL;
            int n = eq ? eq - w - 2 : (int)strlen(w + 2), k;
            for (k = 0; longs[k].name; k++) if ((int)strlen(longs[k].name) == n && !strncmp(longs[k].name, w + 2, n)) break;
            if (!longs[k].name) return badopt(w, "unknown option");
            if (longs[k].val) {
                if (eq) v = eq + 1;
                else if (i + 1 < ac) v = av[++i];
                else return badopt(w, "needs a value");
            } else if (eq) return badopt(w, "takes no value");
            if (!apply(longs[k].c, v)) return 0;
        } else if (!noopts && w[0] == '-' && w[1]) {
            int j;
            for (j = 1; w[j]; j++) {
                char t[3] = { '-', w[j], 0 };
                if (!strchr(shorts, w[j]) || w[j] == ':') return badopt(t, "unknown option");
                if (needsval(w[j])) {
                    char *v;
                    if (w[j + 1]) v = w + j + 1;
                    else if (i + 1 < ac) v = av[++i];
                    else return badopt(t, "needs a value");
                    if (!apply(w[j], v)) return 0;
                    break;
                }
                if (!apply(w[j], NULL)) return 0;
            }
        } else if (!addword(w, 0)) return 0;
    }
    return 1;
}

int main(void)
{
    int i;
    struct Process *me = (struct Process *)FindTask(NULL);
    (void)version;
    if (cu_args(USAGE) < 0) return cu_rc;
    if (!(fib = AllocDosObject(DOS_FIB, NULL))) { cu_err(20, NULL, "out of memory"); return cu_rc; }
    if (!parse()) goto out;
    if (!mode) { cu_err(10, NULL, "needs one of -c, -x, -t (tar ? for usage)"); goto out; }
    verr = cu_out;
    if ((mode == 'c' && !strcmp(archive, "-")) || o_stdout) verr = me->pr_CES ? me->pr_CES : Output();
    if (mode == 'c') {
        if (o_auto && kind == CZ_NONE) kind = cz_kind_of_name(archive);
        createarchive();
    } else {
        int own;
        BPTR fh;
        for (i = 0; i < nwords; i++)
            if (!words[i].isdir && !mkpat(words[i].s, &words[i].parsed)) goto out;
        if (!(fh = openin(&own))) goto out;
        for (i = 0; i < nwords; i++)
            if (words[i].isdir && !chdir_(words[i].s)) { if (own) Close(fh); goto out; }
        readarchive(fh, own);
        if (mode == 'x' && cu_rc < 20) { makelinks(); setdirs(); }
        for (i = 0; i < nwords && cu_rc < 20; i++)
            if (!words[i].isdir && !words[i].found) cu_err(5, words[i].s, "not found in the archive");
    }
out:
    cu_flush();
    if (moved) UnLock(CurrentDir(olddir));
    FreeDosObject(DOS_FIB, fib);
    return cu_rc;
}
