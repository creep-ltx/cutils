/* file - what a file is, from what is inside it.
 *
 *   file [-b] file | pattern ...
 *
 * One line each, "name: what". Known:
 *   AmigaOS: executables (hunk files; a library, device or resource when
 *     a ROMTag says so; "pure" with the p bit), object files and link
 *     libraries, Workbench icons (.info: disk, drawer, tool, project ...,
 *     with OS 3.5 colour images), IFF (ILBM pictures with width, height
 *     and colours, HAM and EHB; 8SVX sounds with their rate; ANIM, SMUS,
 *     FTXT, AIFF, PREF and the rest by name), AmigaGuide, disk images
 *     (ADF by size and bootblock: OFS, FFS, international, cache; hard
 *     disk images with an RDB), PowerPacker and XPK data, DMS
 *   music: ProTracker-style MODs (M.K. and friends, with the channels and
 *     title), OctaMED, Extended Module (XM), Scream Tracker 3, Impulse
 *     Tracker, MIDI, WAV, MP3
 *   archives: LhA, LZX, Zip, gzip, bzip2, xz, 7-Zip, tar
 *   pictures: PNG, GIF, JPEG and BMP with their size
 *   others: PDF, ELF, MS-DOS/Windows executables, PostScript
 *   text: ASCII, ISO-8859-1 (Latin-1, the Amiga's own) or UTF-8, with CRLF
 *     line ends or escape sequences said; "AmigaDOS script" with the s
 *     bit, ARexx (a first line comment and the s bit or .rexx)
 *   drawer, volume, soft link (and where to), empty, else data
 *   -b  only what it is, without "name: "
 *
 * Return code 0, 5 when a file could not be read or a pattern matched
 * nothing (said), 10 bad arguments, 20 Ctrl-C. */
#include "cu.h"

const char *cu_name = "file";
static const char version[] = "$VER: file 0.1b1 (14.10.2026)";

#define HEAD 4096

static int o_brief;
static UBYTE *head;              /* the first HEAD bytes */
static LONG got, fsize, prot;
static BPTR fh;
static struct FileInfoBlock *fib;
static char out[200];

static ULONG be32(const UBYTE *p) { return (ULONG)p[0] << 24 | (ULONG)p[1] << 16 | p[2] << 8 | p[3]; }
static UWORD be16(const UBYTE *p) { return p[0] << 8 | p[1]; }
static ULONG le32(const UBYTE *p) { return (ULONG)p[3] << 24 | (ULONG)p[2] << 16 | p[1] << 8 | p[0]; }
static UWORD le16(const UBYTE *p) { return p[1] << 8 | p[0]; }

static void add(const char *s) { int n = strlen(out); strncpy(out + n, s, sizeof out - n - 1); out[sizeof out - 1] = 0; }
static void addn(ULONG v)
{
    char t[12];
    int i = 11;
    t[i] = 0;
    do t[--i] = '0' + v % 10, v /= 10; while (v);
    add(t + i);
}
static void addsize(ULONG w, ULONG h) { add(", "); addn(w); add(" x "); addn(h); }
static int is(LONG at, const char *s) { int n = strlen(s); return at + n <= got && !memcmp(head + at, s, n); }

/* more of the file, at an offset (for JPEG segments, hunks past HEAD) */
static LONG readat(LONG at, UBYTE *to, LONG n)
{
    if (Seek(fh, at, OFFSET_BEGINNING) < 0) return 0;
    n = Read(fh, to, n);
    return n < 0 ? 0 : n;
}

/* ---- AmigaOS ---- */

static void hunk(void)
{
    ULONG nres, first, last, i, at, type = 0;
    static UBYTE *buf;
    LONG n, codelen;
    const char *kind = "AmigaOS executable";
    if (!buf && !(buf = cu_alloc(16384))) goto said;
    n = readat(0, buf, 16384);
    if (n < 24) goto said;
    nres = be32(buf + 4);
    if (nres) goto said;                  /* resident library names: never in practice */
    first = be32(buf + 12); last = be32(buf + 16);
    at = 20 + (last - first + 1) * 4;
    if (last < first || at + 8 > (ULONG)n) goto said;
    type = be32(buf + at) & 0x3fffffff;
    if (type != 0x3e9) goto said;         /* the first hunk: HUNK_CODE */
    codelen = be32(buf + at + 4) * 4;
    at += 8;
    /* a ROMTag: 0x4AFC, then rt_MatchTag pointing at itself (its offset, before relocation) */
    for (i = 0; i + 26 <= (ULONG)codelen && at + i + 26 <= (ULONG)n; i += 2)
        if (buf[at + i] == 0x4a && buf[at + i + 1] == 0xfc && be32(buf + at + i + 2) == i) {
            int t = buf[at + i + 12];
            kind = t == 9 ? "AmigaOS library" : t == 3 ? "AmigaOS device" : t == 8 ? "AmigaOS resource" : "AmigaOS executable (with a ROMTag)";
            break;
        }
said:
    add(kind);
    if (prot & FIBF_PURE) add(", pure");
}

static void icon(void)
{
    static const char *const types[] = { "", "disk", "drawer", "tool", "project", "trashcan", "device", "Kickstart", "AppIcon" };
    int t = got > 0x30 ? head[0x30] : 0;
    LONG i;
    add("Workbench icon");
    if (t >= 1 && t <= 8) { add(" ("); add(types[t]); add(")"); }
    for (i = 0x4e; i + 12 <= got; i += 2)
        if (!memcmp(head + i, "FORM", 4) && !memcmp(head + i + 8, "ICON", 4)) { add(", OS 3.5 colour image"); break; }
    if (i + 12 > got && fsize > got) {   /* the colour part is usually at the end */
        UBYTE *tail = head;              /* done with the head by now */
        LONG from = fsize > HEAD ? fsize - HEAD : 0, n = readat(from, tail, HEAD), k;
        for (k = 0; k + 12 <= n; k += 2)
            if (!memcmp(tail + k, "FORM", 4) && !memcmp(tail + k + 8, "ICON", 4)) { add(", OS 3.5 colour image"); break; }
    }
}

/* the chunk id inside a FORM (from at, to end): its data offset, or -1 */
static LONG chunk(LONG at, LONG end, const char *id)
{
    while (at + 8 <= end && at + 8 <= got) {
        ULONG len = be32(head + at + 4);
        if (!memcmp(head + at, id, 4)) return at + 8;
        at += 8 + len + (len & 1);
    }
    return -1;
}

static void ilbm(LONG form)          /* form: where "FORM" is */
{
    LONG end = form + 8 + be32(head + form + 4), b = chunk(form + 12, end, "BMHD"), c;
    int planes;
    ULONG camg = 0;
    if (b < 0 || b + 20 > got) return;
    planes = head[b + 8];
    addsize(be16(head + b), be16(head + b + 2));
    if ((c = chunk(form + 12, end, "CAMG")) >= 0 && c + 4 <= got) camg = be32(head + c);
    if (camg & 0x800) { add(", HAM"); addn(planes); }
    else if (camg & 0x80) add(", EHB (64 colours)");
    else if (planes == 24 || planes == 32) { add(", "); addn(planes); add("-bit"); }
    else if (planes) { add(", "); addn(planes); add(planes == 1 ? " plane (" : " planes ("); addn(1UL << planes); add(" colours)"); }
}

static void iff(void)
{
    char id[5];
    memcpy(id, head + 8, 4); id[4] = 0;
    if (!strcmp(id, "ILBM") || !strcmp(id, "PBM ") || !strcmp(id, "ACBM")) {
        add(!strcmp(id, "PBM ") ? "IFF PBM picture" : !strcmp(id, "ACBM") ? "IFF ACBM picture" : "IFF ILBM picture");
        ilbm(0);
    } else if (!strcmp(id, "8SVX") || !strcmp(id, "16SV")) {
        LONG v = chunk(12, fsize, "VHDR");
        add(id[0] == '8' ? "IFF 8SVX sound" : "IFF 16SV sound");
        if (v >= 0 && v + 20 <= got) {
            add(", "); addn(be16(head + v + 12)); add(" Hz");
            if (head[v + 14] > 1) { add(", "); addn(head[v + 14]); add(" octaves"); }
        }
    } else if (!strcmp(id, "ANIM")) {
        add("IFF ANIM animation");
        if (got >= 24 && !memcmp(head + 12, "FORM", 4) && !memcmp(head + 20, "ILBM", 4)) ilbm(12);
    } else if (!strcmp(id, "AIFF") || !strcmp(id, "AIFC")) add("AIFF sound");
    else if (!strcmp(id, "SMUS")) add("IFF SMUS music");
    else if (!strcmp(id, "FTXT")) add("IFF FTXT text");
    else if (!strcmp(id, "PREF")) add("IFF preferences");
    else if (!strcmp(id, "DR2D")) add("IFF DR2D drawing");
    else if (!strcmp(id, "CTLG")) add("IFF catalog (locale)");
    else if (!strcmp(id, "ICON")) add("IFF colour icon");
    else if (!strcmp(id, "WAVE")) add("IFF WAVE");
    else { add("IFF data, FORM "); add(id); }
}

static void adf(void)
{
    static const char *const fs[] = { "OFS", "FFS", "OFS international", "FFS international",
                                      "OFS dircache", "FFS dircache", "OFS long names", "FFS long names" };
    int v = head[3];
    if (fsize == 901120) add("Amiga disk image (ADF, 880 KB), ");
    else if (fsize == 1802240) add("Amiga disk image (ADF, 1760 KB), ");
    else add("Amiga bootblock, ");
    add(fs[v]);
    if (got >= 16 && be32(head + 12)) add(", bootable");
}

/* ---- the others ---- */

static void jpeg(void)
{
    LONG at = 2;
    UBYTE b[10];
    add("JPEG image");
    while (at < fsize) {
        if (readat(at, b, 9) < 9 || b[0] != 0xff) return;
        if (b[1] >= 0xc0 && b[1] <= 0xcf && b[1] != 0xc4 && b[1] != 0xc8 && b[1] != 0xcc) {
            addsize(be16(b + 7), be16(b + 5));
            if (b[1] == 0xc2) add(", progressive");
            return;
        }
        if (b[1] == 0xd8 || b[1] == 0x01 || (b[1] >= 0xd0 && b[1] <= 0xd7)) { at += 2; continue; }
        at += 2 + be16(b + 2);
    }
}

static void png(void)
{
    static const char *const ct[] = { "grey", "", "RGB", "palette", "grey+alpha", "", "RGBA" };
    add("PNG image");
    if (got >= 26 && !memcmp(head + 12, "IHDR", 4)) {
        int c = head[25];
        addsize(be32(head + 16), be32(head + 20));
        add(", "); addn(head[24]); add("-bit");
        if (c <= 6 && ct[c][0]) { add(" "); add(ct[c]); }
    }
}

static int mod(void)
{
    char t[5];
    int ch = 0, i;
    if (got < 1084) return 0;
    memcpy(t, head + 1080, 4); t[4] = 0;
    if (!strcmp(t, "M.K.") || !strcmp(t, "M!K!") || !strcmp(t, "FLT4") || !strcmp(t, "4CHN")) ch = 4;
    else if (!strcmp(t, "FLT8") || !strcmp(t, "8CHN") || !strcmp(t, "OKTA") || !strcmp(t, "CD81")) ch = 8;
    else if (t[1] == 'C' && t[2] == 'H' && t[3] == 'N' && t[0] >= '1' && t[0] <= '9') ch = t[0] - '0';
    else if (t[2] == 'C' && (t[3] == 'H' || t[3] == 'N') && t[0] >= '1' && t[0] <= '3' && t[1] >= '0' && t[1] <= '9') ch = (t[0] - '0') * 10 + t[1] - '0';
    if (!ch) return 0;
    add(ch == 4 ? "ProTracker module (" : "tracker module (");
    add(t); add("), "); addn(ch); add(" channels");
    for (i = 0; i < 20 && head[i] >= 32 && head[i] < 127; i++) ;
    if (i && (i == 20 || !head[i])) {
        char title[21];
        memcpy(title, head, i);
        while (i && title[i - 1] == ' ') i--;
        title[i] = 0;
        if (i) { add(", \""); add(title); add("\""); }
    }
    return 1;
}

static int utf8len(const UBYTE *p, LONG n)   /* a whole UTF-8 sequence here: its length, else 0 */
{
    int k, i;
    if (p[0] >= 0xc2 && p[0] <= 0xdf) k = 2;
    else if (p[0] >= 0xe0 && p[0] <= 0xef) k = 3;
    else if (p[0] >= 0xf0 && p[0] <= 0xf4) k = 4;
    else return 0;
    if (n < k) return k;                 /* cut off by the end of HEAD: give it the benefit */
    for (i = 1; i < k; i++) if ((p[i] & 0xc0) != 0x80) return 0;
    return k;
}

static int text(const char *name)
{
    LONG i, high = 0, crlf = 0, lf = 0;
    int utf8 = 1, esc = 0, k;
    for (i = 0; i < got; i++) {
        UBYTE c = head[i];
        if (c >= 0x80) {
            high = 1;
            if (utf8 && (k = utf8len(head + i, got - i))) { i += k - 1; continue; }
            utf8 = 0;
            if (c < 0xa0) return 0;      /* C1 controls: not Latin-1 text */
            continue;
        }
        if (c == '\n') { lf++; if (i && head[i - 1] == '\r') crlf++; continue; }
        if (c == 0x1b) { esc = 1; continue; }
        if (c < 32 && c != '\t' && c != '\r' && c != '\f' && c != '\b' && c != 7) return 0;
        if (c == 127) return 0;
    }
    {
        int n = strlen(name);
        int rexx = (n > 5 && !strcmp(name + n - 5, ".rexx")) || (n > 3 && !strcmp(name + n - 3, ".rx"));
        if (is(0, "/*") && (rexx || (prot & FIBF_SCRIPT))) add("ARexx script, ");
        else if (prot & FIBF_SCRIPT) add("AmigaDOS script, ");
    }
    if (got >= 9 && (head[0] == '@') && (head[1] | 0x20) == 'd' && (head[2] | 0x20) == 'a' && (head[3] | 0x20) == 't'
        && (head[4] | 0x20) == 'a' && (head[5] | 0x20) == 'b' && (head[6] | 0x20) == 'a' && (head[7] | 0x20) == 's' && (head[8] | 0x20) == 'e')
        add("AmigaGuide document, ");
    else if (is(0, "%!PS")) add("PostScript document, ");
    else if (is(0, "#!")) {
        int j;
        add("script for ");
        for (j = 2; j < got && j < 60 && head[j] == ' '; j++) ;
        for (; j < got && j < 60 && head[j] > ' '; j++) { char t[2] = { head[j], 0 }; add(t); }
        add(", ");
    }
    add(!high ? "ASCII text" : utf8 ? "UTF-8 text" : "ISO-8859-1 text");
    if (crlf && crlf == lf) add(", with CRLF line ends");
    else if (crlf) add(", with some CRLF line ends");
    if (esc) add(", with escape sequences");
    return 1;
}

static void sniff(const char *name)
{
    ULONG m = got >= 4 ? be32(head) : 0;
    if (m == 0x3f3) hunk();
    else if (m == 0x3e7) add("AmigaOS object file");
    else if (m == 0x3fa) add("AmigaOS link library");
    else if (got >= 2 && be16(head) == 0xe310 && got >= 4 && be16(head + 2) == 1) icon();
    else if (is(0, "FORM") && got >= 12) iff();
    else if (is(0, "CAT ") || is(0, "LIST")) add("IFF data, CAT or LIST");
    else if (got >= 4 && is(0, "DOS") && head[3] <= 7) adf();
    else if (is(0, "RDSK")) add("Amiga hard disk image (RDB)");
    else if (is(0, "PP20")) add("PowerPacker data");
    else if (is(0, "XPKF")) add("XPK packed data");
    else if (is(0, "DMS!")) add("DMS disk archive");
    else if (got >= 7 && head[2] == '-' && head[3] == 'l' && (head[4] == 'h' || head[4] == 'z') && head[6] == '-') {
        char t[6];
        memcpy(t, head + 2, 5); t[5] = 0;
        add("LhA archive ("); add(t); add(")");
    } else if (is(0, "LZX")) add("LZX archive");
    else if (is(0, "PK\3\4") || is(0, "PK\5\6")) add("Zip archive");
    else if (got >= 2 && head[0] == 0x1f && head[1] == 0x8b) add("gzip compressed data");
    else if (is(0, "BZh")) add("bzip2 compressed data");
    else if (is(0, "\xfd" "7zXZ")) add("xz compressed data");
    else if (is(0, "7z\xbc\xaf\x27\x1c")) add("7-Zip archive");
    else if (is(257, "ustar")) add("tar archive");
    else if (is(0, "\x89PNG\r\n\x1a\n")) png();
    else if (is(0, "GIF87a") || is(0, "GIF89a")) { add("GIF image"); if (got >= 10) addsize(le16(head + 6), le16(head + 8)); }
    else if (got >= 3 && head[0] == 0xff && head[1] == 0xd8 && head[2] == 0xff) jpeg();
    else if (is(0, "BM") && got >= 30 && le32(head + 14) >= 12) {
        LONG h = (LONG)le32(head + 22);
        add("BMP image"); addsize(le32(head + 18), h < 0 ? -h : h); add(", "); addn(le16(head + 28)); add("-bit");
    } else if (is(0, "%PDF-")) {
        int j;
        add("PDF document, version ");
        for (j = 5; j < got && j < 9 && head[j] > ' '; j++) { char t[2] = { head[j], 0 }; add(t); }
    } else if (is(0, "\x7f" "ELF")) add("ELF executable");
    else if (is(0, "MZ")) add("MS-DOS or Windows executable");
    else if (is(0, "MThd")) add("MIDI music");
    else if (is(0, "RIFF") && is(8, "WAVE")) add("WAV sound");
    else if (is(0, "RIFF") && is(8, "AVI ")) add("AVI video");
    else if (is(0, "ID3") || (got >= 2 && head[0] == 0xff && (head[1] & 0xe0) == 0xe0 && (head[1] & 0x06))) add("MP3 audio");
    else if (is(0, "MMD0") || is(0, "MMD1") || is(0, "MMD2") || is(0, "MMD3")) add("OctaMED module");
    else if (is(0, "Extended Module:")) add("Extended Module (XM)");
    else if (is(44, "SCRM")) add("Scream Tracker 3 module");
    else if (is(0, "IMPM")) add("Impulse Tracker module");
    else if (mod()) ;
    else if (!text(name)) add("data");
}

static int one(const char *path, void *u)
{
    (void)u;
    out[0] = 0;
    if (!cu_lstat(path, fib)) { cu_fault(5, path); return cu_break(); }
    if (fib->fib_DirEntryType == ST_SOFTLINK) {
        static char dir[CU_PATH], t[CU_PATH];
        int n = (const char *)PathPart((STRPTR)path) - path;
        BPTR l;
        add("soft link");
        if (n < CU_PATH) {
            memcpy(dir, path, n); dir[n] = 0;
            if ((l = Lock(dir, SHARED_LOCK))) {
                struct MsgPort *port = ((struct FileLock *)BADDR(l))->fl_Task;
                LONG r = ReadLink(port, l, FilePart((STRPTR)path), t, sizeof t);
                if (r != 0 && r != -1 && r != -2) { add(" to "); add(t); }
                UnLock(l);
            }
        }
    } else if (fib->fib_DirEntryType == ST_ROOT) add("volume");
    else if (fib->fib_DirEntryType > 0) add("drawer");
    else if (!fib->fib_Size) add("empty");
    else {
        fsize = fib->fib_Size;
        prot = fib->fib_Protection;
        if (!(fh = Open((STRPTR)path, MODE_OLDFILE))) { cu_fault(5, path); return cu_break(); }
        got = Read(fh, head, HEAD);
        if (got < 0) { cu_fault(5, path); Close(fh); return cu_break(); }
        sniff((const char *)FilePart((STRPTR)path));
        Close(fh);
    }
    if (!o_brief) { cu_puts(path); cu_puts(": "); }
    cu_puts(out);
    cu_putc('\n');
    return cu_break();
}

int main(void)
{
    char *v;
    int c, i;
    (void)version;
    if (cu_args("file [-b] file | pattern ...") < 0) return cu_rc;
    while ((c = cu_opt("b", &v)) >= 0) switch (c) {
        case 'b': o_brief = 1; break;
        default: return cu_rc;
    }
    if (!cu_argc) { cu_err(10, NULL, "no file named (file ? for usage)"); return cu_rc; }
    if (!(fib = AllocDosObject(DOS_FIB, NULL)) || !(head = cu_alloc(HEAD))) { cu_err(20, NULL, "out of memory"); return cu_rc; }
    for (i = 0; i < cu_argc && cu_rc < 20; i++) cu_each(cu_argv[i], one, NULL);
    FreeDosObject(DOS_FIB, fib);
    cu_flush();
    return cu_rc;
}
