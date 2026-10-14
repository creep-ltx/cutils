/* czc.c - the command behind gzip, gunzip, bzip2, bunzip2, unxz and
 * unzstd (czc.h): GNU gzip's ways for one kind each. A file is replaced by
 * the packed (or unpacked) one, which gets its date, protection bits and
 * file note; -c writes to the output instead, -k keeps the input. The
 * input or output (no files, or -) when nothing is named. */
#include "cu.h"
#include "czc.h"

int czc_level, czc_noname;
UBYTE *czc_buf;

const CzcCmd *czc_cmd;
int czc_unpack, czc_stdout, czc_keep, czc_force, czc_verbose, czc_quiet, czc_test, czc_list, czc_recurse;
const char *czc_suffix;
static int listed;
static LONG tot_in, tot_out;

const struct CzcExt czc_exts[] = {
    { CZ_GZIP, ".gz", "" }, { CZ_GZIP, ".tgz", ".tar" },
    { CZ_BZIP2, ".bz2", "" }, { CZ_BZIP2, ".tbz2", ".tar" }, { CZ_BZIP2, ".tbz", ".tar" }, { CZ_BZIP2, ".bz", "" },
    { CZ_XZ, ".xz", "" }, { CZ_XZ, ".txz", ".tar" },
    { CZ_ZSTD, ".zst", "" }, { CZ_ZSTD, ".tzst", ".tar" },
    { 0, NULL, NULL }
};

/* ---- the error stream, for -v (as GNU: the output may be the data) ---- */

void czc_eput(const char *s)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    BPTR fh = me->pr_CES ? me->pr_CES : Output();
    cu_flush();
    Write(fh, (APTR)s, strlen(s));
}

/* " 53.0%": how much smaller the packed one is than the plain one */
static void ratio(char *b, LONG plain, LONG packed)
{
    LONG pm, p = plain, q = packed;
    int i = 0, neg;
    char t[12];
    while (p > 1000000 || q > 1000000) p >>= 1, q >>= 1;
    pm = p ? (p - q) * 1000 / p : 0;
    if ((neg = pm < 0)) pm = -pm;
    t[i++] = '%';
    t[i++] = '0' + pm % 10; pm /= 10;
    t[i++] = '.';
    do { t[i++] = '0' + pm % 10; pm /= 10; } while (pm);
    if (neg) t[i++] = '-';
    while (i < 6) t[i++] = ' ';
    while (i) *b++ = t[--i];
    *b = 0;
}

void czc_say(const char *name, LONG plain, LONG packed, const char *out)
{
    char r[16];
    if (!czc_verbose) return;
    ratio(r, plain, packed);
    czc_eput(name ? name : "-"); czc_eput(":\t"); czc_eput(r);
    if (out) { czc_eput(czc_keep ? " -- created " : " -- replaced with "); czc_eput(out); }
    czc_eput("\n");
}

void czc_warn(int rc, const char *what, const char *why)
{
    if (czc_quiet) { if (cu_rc < rc) cu_rc = rc; return; }
    cu_err(rc, what, why);
}

/* ---- names ---- */

const char *czc_packsuffix(void)
{
    int i;
    if (czc_suffix) return czc_suffix;
    for (i = 0; czc_exts[i].ext; i++)
        if (czc_exts[i].kind == czc_cmd->kind) return czc_exts[i].ext;
    return "";
}

int czc_ends(const char *s, const char *e)
{
    int n = strlen(s), k = strlen(e);
    return k && n > k && !stricmp(s + n - k, e);
}

/* the name without its suffix (.tgz gives .tar): 1, or 0 when it has none of this kind's */
static int plainname(const char *path, char *out)
{
    int i, n = strlen(path);
    if (czc_suffix && czc_ends(path, czc_suffix)) {
        n -= strlen(czc_suffix);
        memcpy(out, path, n); out[n] = 0;
        return 1;
    }
    for (i = 0; czc_exts[i].ext; i++)
        if (czc_exts[i].kind == czc_cmd->kind && czc_ends(path, czc_exts[i].ext)) {
            int k = n - strlen(czc_exts[i].ext);
            if (k + strlen(czc_exts[i].to) >= CU_PATH) return 0;
            memcpy(out, path, k);
            strcpy(out + k, czc_exts[i].to);
            return 1;
        }
    return 0;
}

/* the new file gets the old one's protection bits, date and file note */
void czc_clone(const char *to, struct FileInfoBlock *fib)
{
    SetProtection((STRPTR)to, fib->fib_Protection);
    SetFileDate((STRPTR)to, &fib->fib_Date);
    if (fib->fib_Comment[0]) SetComment((STRPTR)to, fib->fib_Comment);
}

/* the new file opened, unless it is there already (-f: replaced) */
BPTR czc_create(const char *out)
{
    BPTR fh, l;
    if ((l = Lock((STRPTR)out, SHARED_LOCK))) {
        UnLock(l);
        if (!czc_force) { czc_warn(5, out, "already exists; not overwritten"); return 0; }
        SetProtection((STRPTR)out, 0);       /* a delete-protected one too, as -f means */
    }
    if (!(fh = Open((STRPTR)out, MODE_NEWFILE))) cu_fault(10, out);
    return fh;
}

void czc_removein(const char *path)
{
    if (!czc_keep && !czc_stdout && !DeleteFile((STRPTR)path)) cu_fault(5, path);
}

/* ---- unpacking, testing, listing ---- */

static const char *notformat(void)
{
    return czc_cmd->kind == CZ_GZIP ? "not in gzip format" : czc_cmd->kind == CZ_BZIP2 ? "not in bzip2 format"
         : czc_cmd->kind == CZ_XZ ? "not in xz format" : "not in zstd format";
}

/* the stream opened as this command's kind: NULL when not (said) */
static CzIn *openkind(const char *name, BPTR in, int *plain)
{
    int found;
    CzIn *z = cz_open(in, CZ_AUTO, &found);
    *plain = 0;
    if (z && found == CZ_NONE && czc_force && czc_stdout && !czc_test && !czc_list) { *plain = 1; return z; }
    if (z && found == czc_cmd->kind) return z;
    if (z) cz_close(z);
    if (found == czc_cmd->kind || (!z && found == CZ_NONE)) cu_err(10, name, cz_error());
    else cu_err(10, name, notformat());
    return NULL;
}

/* a number right aligned in w columns (cu_putnum pads 11 at most) */
static void pad(LONG v, int w)
{
    LONG t = v;
    int d = 1;
    while (t >= 10) t /= 10, d++;
    while (w-- > d) cu_putc(' ');
    cu_putnum(v, 0);
}

static void listline(LONG packed, LONG plain, const char *name)
{
    char r[16];
    if (!listed++) cu_puts("         compressed        uncompressed  ratio uncompressed_name\n");
    ratio(r, plain, packed);
    pad(packed, 19); pad(plain, 20); cu_puts(" "); cu_puts(r); cu_puts(" "); cu_puts(name); cu_putc('\n');
    tot_in += packed; tot_out += plain;
}

static void unpackfile(const char *path, struct FileInfoBlock *fib)
{
    char out[CU_PATH];
    const char *name = path ? path : "-";
    BPTR in, fh = 0;
    CzIn *z;
    LONG n, insize = fib ? fib->fib_Size : 0, outsize = 0;
    int named = path && plainname(path, out), plain, bad = 0;
    if (path && !named && !czc_stdout && !czc_test && !czc_list) { czc_warn(5, path, "unknown suffix -- ignored"); return; }
    if (!path) in = Input();
    else if (!(in = Open((STRPTR)path, MODE_OLDFILE))) { cu_fault(5, path); return; }
    if (czc_list && czc_cmd->kind == CZ_GZIP && path) {
        /* gzip keeps the length at the end: no need to unpack (the last member's, as GNU) */
        UBYTE t[4];
        int found;
        if ((z = cz_open(in, CZ_AUTO, &found))) cz_close(z);
        if (found != CZ_GZIP) { cu_err(10, name, notformat()); Close(in); return; }
        if (Seek(in, -4, OFFSET_END) < 0 || Read(in, t, 4) != 4) { cu_err(10, name, "truncated"); Close(in); return; }
        Close(in);
        listline(insize, t[0] | t[1] << 8 | t[2] << 16 | (LONG)t[3] << 24, named ? out : name);
        return;
    }
    if (!(z = openkind(name, in, &plain))) { if (path) Close(in); return; }
    if (!czc_test && !czc_list) {
        if (czc_stdout || !path) fh = Output();
        else if (!(fh = czc_create(out))) { cz_close(z); Close(in); return; }
    }
    cu_flush();
    while ((n = cz_read(z, czc_buf, CZC_BUF)) > 0) {
        outsize += n;
        if (fh && Write(fh, czc_buf, n) != n) { cu_fault(10, czc_stdout || !path ? "output" : out); bad = 1; break; }
        if (cu_break()) { bad = 1; break; }
    }
    if (n < 0) { cu_err(10, name, cz_error()); bad = 1; }
    if (!bad && !plain && cz_trailing(z)) czc_warn(5, name, "decompression OK, trailing garbage ignored");
    cz_close(z);
    if (path) Close(in);
    if (!path) insize = 0;
    if (czc_list) {
        if (!bad) listline(insize, outsize, named ? out : name);
        return;
    }
    if (czc_test) {
        if (!bad && czc_verbose) { czc_eput(name); czc_eput(":\t OK\n"); }
        return;
    }
    if (fh && !czc_stdout && path) {
        Close(fh);
        if (bad) { DeleteFile((STRPTR)out); return; }
        czc_clone(out, fib);
        czc_removein(path);
    }
    if (!bad) czc_say(path, outsize, insize, czc_stdout || !path ? NULL : out);
}

/* ---- one name ---- */

static void file(const char *path, struct FileInfoBlock *fib)
{
    if (czc_unpack || czc_test || czc_list) unpackfile(path, fib);
    else czc_cmd->pack(czc_cmd, path, fib);
}

static struct FileInfoBlock *fib;

static int walked(CuEnt *e, void *u)
{
    (void)u;
    if (!e->depth || cu_isdir(e->type)) return 0;
    if (CU_LINK(e->type)) return 0;
    if (cu_lstat(e->path, fib)) file(e->path, fib);
    else cu_fault(5, e->path);
    return cu_rc >= 20 ? CU_STOP : 0;
}

static int one(const char *path, void *u)
{
    char alt[CU_PATH];
    (void)u;
    if (!cu_lstat(path, fib)) {
        /* gunzip foo finds foo.gz, as GNU */
        int i, found = 0;
        if (czc_unpack || czc_test || czc_list)
            for (i = 0; czc_exts[i].ext && !found; i++)
                if (czc_exts[i].kind == czc_cmd->kind && strlen(path) + strlen(czc_exts[i].ext) < CU_PATH) {
                    strcpy(alt, path); strcat(alt, czc_exts[i].ext);
                    if (cu_lstat(alt, fib)) found = 1, path = alt;
                }
        if (!found) { cu_fault(5, path); return 0; }
    }
    if (CU_LINK(fib->fib_DirEntryType)) czc_warn(5, path, "is a link -- ignored");
    else if (fib->fib_DirEntryType > 0) {
        if (czc_recurse) cu_walk(path, 0, -1, walked, NULL);
        else czc_warn(5, path, "is a drawer -- ignored");
    } else file(path, fib);
    return cu_rc >= 20;
}

/* --long options as GNU's, turned into their letters */
static void longopts(void)
{
    static const char *const map[] = {
        "--stdout", "-c", "--to-stdout", "-c", "--decompress", "-d", "--uncompress", "-d", "--compress", "-z",
        "--keep", "-k", "--force", "-f", "--verbose", "-v", "--quiet", "-q", "--test", "-t", "--list", "-l",
        "--recursive", "-r", "--no-name", "-n", "--name", "-N", "--fast", "-1", "--best", "-9", "--small", "-s",
        "--rm", "-R", NULL
    };
    int i, k;
    for (i = 0; i < cu_argc; i++) {
        char *w = cu_argv[i];
        if (w[0] != '-' || !w[1] || (w[1] == '-' && !w[2])) break;
        if (w[1] != '-') continue;
        for (k = 0; map[k]; k += 2)
            if (!strcmp(w, map[k])) { cu_argv[i] = (char *)map[k + 1]; break; }
    }
}

int czc_main(const CzcCmd *c)
{
    char *v;
    int ch, i;
    czc_cmd = c;
    if (cu_args(c->usage) < 0) return cu_rc;
    longopts();
    czc_unpack = !c->packs;
    czc_keep = c->keeps;
    while ((ch = cu_opt("123456789cdzkfvqtlrnNsRS:", &v)) >= 0) switch (ch) {
        case 'c': czc_stdout = 1; break;
        case 'd': czc_unpack = 1; break;
        case 'z':
            if (!c->pack) { cu_err(10, "-z", "this command only unpacks"); return cu_rc; }
            czc_unpack = 0; break;
        case 'k': czc_keep = 1; break;
        case 'R': czc_keep = 0; break;
        case 'f': czc_force = 1; break;
        case 'v': czc_verbose = 1; czc_quiet = 0; break;
        case 'q': czc_quiet = 1; czc_verbose = 0; break;
        case 't': czc_test = 1; break;
        case 'l': czc_list = 1; break;
        case 'r': czc_recurse = 1; break;
        case 'n': czc_noname = 1; break;
        case 'N': case 's': break;           /* GNU's -N (unpack: the stored name) and bzip2's -s: taken, not needed */
        case 'S':
            if (!*v || strchr(v, '/') || strchr(v, ':')) { cu_err(10, v, "not a suffix"); return cu_rc; }
            czc_suffix = v; break;
        case '?': return cu_rc;
        default: czc_level = ch - '0'; break;
    }
    if (!c->pack) czc_unpack = 1;
    if (!(czc_buf = cu_alloc(CZC_BUF)) || !(fib = AllocDosObject(DOS_FIB, NULL))) { cu_err(10, NULL, "out of memory"); return cu_rc; }
    /* as GNU: no packed bytes to a window, none read from one */
    if (!czc_force && !czc_test && !czc_list) {
        int toout = czc_stdout || !cu_argc || (cu_argc == 1 && !strcmp(cu_argv[0], "-"));
        int fromin = !cu_argc;
        for (i = 0; i < cu_argc; i++) if (!strcmp(cu_argv[i], "-")) fromin = 1;
        if (!czc_unpack && toout && IsInteractive(Output())) {
            cu_err(10, NULL, "compressed data not written to a terminal (-f forces it)");
            return cu_rc;
        }
        if (czc_unpack && fromin && IsInteractive(Input())) {
            cu_err(10, NULL, "compressed data not read from a terminal (-f forces it)");
            return cu_rc;
        }
    }
    if (!cu_argc) file(NULL, NULL);
    for (i = 0; i < cu_argc && cu_rc < 20; i++)
        if (!strcmp(cu_argv[i], "-")) file(NULL, NULL);
        else cu_each(cu_argv[i], one, NULL);
    if (czc_list && listed > 1) listline(tot_in, tot_out, "(totals)");
    cu_flush();
    FreeDosObject(DOS_FIB, fib);
    return cu_rc;
}
