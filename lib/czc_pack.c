/* czc_pack.c - one file packed for gzip and bzip2 (czc.h): its own file,
 * so gunzip, bunzip2, unxz and unzstd link no packer. */
#include "cu.h"
#include "czc.h"

typedef struct {                 /* one file on its way */
    const char *path;            /* NULL: the input */
    const char *out;             /* the new file, NULL: to the output */
    BPTR in, fh;                 /* fh: where it goes */
    LONG insize, outsize;
    LONG size;                   /* the input's size, -1 unknown (a pipe) */
    ULONG mtime;                 /* the input's date, Unix seconds */
} CzcFile;

static int pack(const CzcCmd *c, CzcFile *f)
{
    const char *name = f->path ? f->path : "-";
    int level = czc_level ? czc_level : c->level;
    CzOut *z;
    LONG n, pos;
    /* bzip2: a block bigger than the file only costs memory (8 times the
     * block when packing, 7.6 MB at -9): a file that fits a smaller block
     * gets it, the same packed bytes but the level digit (bzip2 -d then
     * needs less memory too). 80 000: the block holds the bytes after
     * bzip2's first run-length pass, which can grow them by a quarter. */
    if (c->kind == CZ_BZIP2 && f->size >= 0 && f->size / 80000 + 1 < level) level = f->size / 80000 + 1;
    z = cz_create(f->fh, c->kind, level);
    if (!z) { cu_err(10, name, cz_error()); return 0; }
    if (!czc_noname && f->path) cz_set_name(z, FilePart((STRPTR)f->path), f->mtime);
    while ((n = Read(f->in, czc_buf, CZC_BUF)) > 0) {
        f->insize += n;
        if (cz_write(z, czc_buf, n) < 0) {
            cu_err(10, f->out ? f->out : "output", cz_error());
            cz_finish(z);
            return 0;
        }
        if (cu_break()) { cz_finish(z); return 0; }
    }
    if (n < 0) { cu_fault(10, name); cz_finish(z); return 0; }
    if (cz_finish(z) < 0) { cu_err(10, f->out ? f->out : "output", cz_error()); return 0; }
    pos = Seek(f->fh, 0, OFFSET_CURRENT);   /* the packed size, when it is a file */
    f->outsize = pos > 0 ? pos : 0;
    return 1;
}

void czc_packfile(const CzcCmd *c, const char *path, struct FileInfoBlock *fib)
{
    char out[CU_PATH];
    CzcFile f;
    const char *suf = czc_packsuffix();
    int ok;
    memset(&f, 0, sizeof f);
    f.path = path;
    f.size = path ? fib->fib_Size : -1;
    if (path) {
        int i;
        for (i = 0; czc_exts[i].ext; i++)
            if (czc_exts[i].kind == c->kind && czc_ends(path, czc_exts[i].ext) && !czc_force && !czc_stdout) {
                char t[40] = "already has ";
                strcat(t, czc_exts[i].ext);
                strcat(t, " suffix -- unchanged");
                czc_warn(5, path, t);
                return;
            }
        if (czc_suffix && czc_ends(path, czc_suffix) && !czc_force && !czc_stdout) { czc_warn(5, path, "already has the suffix -- unchanged"); return; }
        if (!(f.in = Open((STRPTR)path, MODE_OLDFILE))) { cu_fault(5, path); return; }
        f.mtime = cu_unixtime(&fib->fib_Date);
    } else f.in = Input();
    if (czc_stdout || !path) f.fh = Output();
    else {
        if (strlen(path) + strlen(suf) >= CU_PATH) { cu_err(10, path, "name too long"); Close(f.in); return; }
        strcpy(out, path); strcat(out, suf);
        f.out = out;
        if (!(f.fh = czc_create(out))) { Close(f.in); return; }
    }
    cu_flush();
    ok = pack(c, &f);
    if (path) Close(f.in);
    if (f.out) {
        Close(f.fh);
        if (!ok) { DeleteFile((STRPTR)out); return; }
        czc_clone(out, fib);
        czc_removein(path);
    }
    if (ok) czc_say(path, f.insize, f.outsize, f.out);
}
