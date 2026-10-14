/* cu.h - what every CUtils command shares (lib/cu.c): the command line in
 * Unix bundles, Amiga patterns and *, buffered output, error messages on
 * the error stream, Ctrl-C, lines read fast, paths joined. dos.library
 * only, no stdio: the commands stay small and pure (-resident32). */
#ifndef CU_H
#define CU_H

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dosasl.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>

#ifndef FIBF_HIDDEN
#define FIBF_HIDDEN (1 << 7)     /* the h bit: not in the NDK's dos.h */
#endif

#define CU_PATH 512              /* longest path a command builds */

extern const char *cu_name;      /* the command's name, for messages */
extern int cu_rc;                /* the worst so far: 0, 5, 10, 20 */
extern BPTR cu_out;              /* Output() */

/* ---- memory (AllocVec, any memory) ---- */
void *cu_alloc(ULONG n);
void cu_free(void *p);
char *cu_strdup(const char *s);

/* ---- the command line ----
 * cu_args() splits GetArgStr() into words as the Shell quotes them ("a b",
 * *" *n *e and ** inside quotes) and returns the count; a lone ? prints
 * usage and ends with 0. cu_opt() then walks Unix flags:
 *     while ((c = cu_opt("n:v", &val)) >= 0) switch (c) { ... }
 * A letter followed by : takes a value (-n5, -n 5). -- ends the flags, a
 * lone - is a word (standard input). -NUM gives c '#' with val the number
 * text (head -5). An unknown flag: a message, rc 10, c is '?'.
 * After it, cu_argc/cu_argv are the words left. */
extern int cu_argc;
extern char **cu_argv;
int cu_args(const char *usage);
int cu_opt(const char *spec, char **val);
long cu_num(const char *s, int *ok);   /* decimal, with k/M/G (1024s) */

/* ---- output ---- */
void cu_put(const char *s, LONG n);
void cu_puts(const char *s);
void cu_putc(int c);
void cu_putnum(LONG v, int width);     /* right aligned, width 0: as long as it is */
void cu_flush(void);

/* ---- messages: "name: what: why" on the error stream, sets cu_rc ---- */
void cu_err(int rc, const char *what, const char *why);
void cu_fault(int rc, const char *what);   /* why = IoErr()'s text */
int cu_break(void);                         /* Ctrl-C: 1 (message once, rc 20) */

/* ---- patterns ---- */
int cu_haswild(const char *s);
/* Each name an argument stands for: the argument itself, or (a pattern
 * that is not a name as typed) every match in its drawer, sorted. fn gets
 * the path; it returns 0 to go on, else the walk stops. A pattern that
 * matches nothing: a message (rc 5). Returns 0, or what fn returned. */
int cu_each(const char *arg, int (*fn)(const char *path, void *u), void *u);
/* An Amiga pattern from * or #? (case blind), for matching names. */
int cu_pattern(const char *pat, char *parsed, int size);
int cu_match(const char *parsed, const char *name);
/* Natural, case blind order: "file9" before "file10" (<0, 0, >0). */
int cu_natcmp(const char *a, const char *b);

/* ---- paths ---- */
int cu_join(char *buf, const char *dir, const char *name);   /* 0: too long */
long cu_stackleft(void);

/* ---- reading text ----
 * A file (or "-" / NULL: Input()) read in big blocks; cu_line() gives one
 * line at a time, without its \n (*nl says whether it had one), long lines
 * grown on the heap. Returns the length, or -1 at the end. */
typedef struct {
    BPTR fh;
    int own;                     /* close it at the end */
    char *buf, *line;
    LONG pos, len, size, lsize;
    int eof, err;
} CuIn;
int cu_open(CuIn *in, const char *path);   /* 0: cu_fault already said why */
LONG cu_line(CuIn *in, char **line, int *nl);
LONG cu_read(CuIn *in, char *to, LONG n);  /* raw bytes, through the buffer */
void cu_close(CuIn *in);

/* ---- cu_walk.c: a tree, walked without recursion ----
 * cu_walk() calls fn for the start path (depth 0), then for everything
 * below it, each drawer's entries in natural order, a drawer before its
 * contents. The walk keeps its own list on the heap (a few hundred bytes
 * of the Shell's stack whatever the depth) and holds no locks while fn
 * runs: each drawer is read whole first, so fn may delete or rename.
 * Links (soft, and hard links to drawers) are never gone into.
 * fn returns 0 to go on, CU_SKIP (from a drawer's first call) not to go
 * into it, CU_STOP to end the walk. maxdepth < 0: no limit; else drawers
 * at that depth are not gone into. CU_WALK_POST: a second call (post 1)
 * for every drawer after its contents (also one not gone into). A drawer
 * that cannot be read: a message (rc 5), and on. Ctrl-C ends it (rc 20).
 * Returns 0, or CU_STOP when fn or Ctrl-C stopped it. */
#define CU_SKIP 1
#define CU_STOP 2
#define CU_WALK_POST 1
typedef struct {
    const char *path;            /* the start as given, names joined with / */
    const char *name;            /* the last part (the start: its own name) */
    LONG type, size, prot;       /* as in a FileInfoBlock */
    struct DateStamp date;
    int depth, post;
} CuEnt;
int cu_isdir(LONG type);         /* a drawer one may go into: not a link */
int cu_walk(const char *start, int flags, int maxdepth, int (*fn)(CuEnt *e, void *u), void *u);

/* ---- cu_colours.c: kinds of files and their colours ----
 * ENV:FileColours (else ENVARC:, written with the defaults when missing),
 * shared by ls, Ahiru and Maki: "KIND CTERM,PLAIN" SGR numbers, ".EXT KIND".
 * cu_colours_load() once, then cu_kind() for an entry and cu_sgr() for the
 * escape sequence that colours it ("" for none). */
enum { CU_K_TEXT, CU_K_DRAWER, CU_K_LINK, CU_K_PROGRAM, CU_K_SCRIPT, CU_K_ARCHIVE,
       CU_K_PICTURE, CU_K_SOUND, CU_K_SOURCE, CU_K_ICON, CU_K_HIDDEN, CU_K_COUNT };
void cu_colours_load(void);
int cu_kind(const char *name, LONG type, LONG prot);
const char *cu_sgr(int kind, int cterm);   /* cterm: CTerm's numbers, else a plain window's */
int cu_iscterm(BPTR fh);                   /* the handle is a CTerm console */

/* ---- regex.c ---- */
#define RE_GROUPS 10
void *re_comp(const char *re, int nocase);
int re_exec(void *prog, const char *s, int len, int *caps);
void re_free(void *prog);

#endif
