/* czc.h - the command shared by gzip, gunzip, bzip2, bunzip2, unxz and
 * unzstd (lib/czc.c): GNU gzip's flags and ways, for one kind each. A
 * command fills a CzcCmd and calls czc_main(). Packing is in its own file
 * (czc_pack.c, reached through .pack = czc_packfile), so the unpack-only
 * commands link no packer. */
#ifndef CZC_H
#define CZC_H

#include "cz.h"

typedef struct CzcCmd CzcCmd;

struct CzcCmd {
    int kind;                    /* CZ_GZIP ... */
    int packs;                   /* packs unless -d (gzip, bzip2); else always unpacks */
    int keeps;                   /* keeps the input by default (zstd's way; --rm deletes it) */
    int level;                   /* the default level */
    const char *usage;
    /* one file packed: czc_packfile; NULL: this command only unpacks */
    void (*pack)(const CzcCmd *c, const char *path, struct FileInfoBlock *fib);
};

int czc_main(const CzcCmd *c);
void czc_packfile(const CzcCmd *c, const char *path, struct FileInfoBlock *fib);

/* ---- inside: what czc.c shares with czc_pack.c ---- */
#define CZC_BUF 32768
struct CzcExt { int kind; const char *ext, *to; };
extern const struct CzcExt czc_exts[];      /* the suffixes, .tgz to .tar */
extern const CzcCmd *czc_cmd;
extern int czc_unpack, czc_stdout, czc_keep, czc_force, czc_verbose, czc_quiet, czc_test, czc_list, czc_recurse;
extern int czc_level, czc_noname;
extern const char *czc_suffix;
extern UBYTE *czc_buf;                      /* CZC_BUF bytes */
void czc_eput(const char *s);               /* on the error stream */
void czc_say(const char *name, LONG plain, LONG packed, const char *out);   /* -v */
void czc_warn(int rc, const char *what, const char *why);                   /* not with -q */
const char *czc_packsuffix(void);
int czc_ends(const char *s, const char *e);
void czc_clone(const char *to, struct FileInfoBlock *fib);   /* protection, date, note */
BPTR czc_create(const char *out);           /* 0: there already (said), or failed (said) */
void czc_removein(const char *path);        /* unless -k / -c */

#endif
