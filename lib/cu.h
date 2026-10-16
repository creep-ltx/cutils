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
extern int cu_silent;            /* 1: cu_err says nothing (grep -s), the rc still counts */

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
/* the console's width in columns, asked with the window bounds report
 * (CSI 0 q, written to out, the answer read from in in raw mode): CON:,
 * RAW: and CTerm all answer. 0 when in is not a console or none came. */
int cu_termwidth(BPTR in, BPTR out);

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

/* ---- cu_lines.c: many lines in memory (sort) ----
 * cu_lines_add() copies a line (n bytes, a 0 put after it) into 64 KB
 * chunks and its pointer into l->v (grown by doubling): no malloc per
 * line. pre bytes (rounded to long words) are kept free before the text,
 * for the caller's own (sort's key positions). Returns the text, NULL:
 * out of memory. cu_lines_free() gives it all back.
 * cu_msort(): a stable merge sort of n pointers, bottom-up (no
 * recursion); cmp <0, 0, >0. Ctrl-C ends it early (the order then
 * half done). 0: no memory for its second array. */
typedef struct {
    char **v;                    /* the lines */
    LONG n, max;
    void *chunks;
    char *mem;                   /* the chunk being filled */
    LONG used, size;
} CuLines;
char *cu_lines_add(CuLines *l, const char *s, LONG n, LONG pre);
void cu_lines_free(CuLines *l);
int cu_msort(char **v, LONG n, int (*cmp)(const char *a, const char *b));

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

/* ---- cu_date.c: the calendar, dates from text ----
 * cu_days(): days since 1.1.1978 (the Amiga's ds_Days) of a Gregorian
 * date; cu_civil() back. cu_parsedate(): 2026-10-14, "2026-10-14 13:45",
 * 2026-10-14T13:45:10, a time alone (13:45, today), or the Amiga's own
 * "14-Oct-26 13:45:10", Today, Yesterday, a weekday (StrToDate); 0 when
 * it is none of them. cu_parsetime(): H:MM[:SS] into ds_Minute/ds_Tick.
 * cu_isodate(): "2026-10-14 18:36:41" into 20 bytes. cu_unixtime():
 * seconds since 1970 (the Amiga's clock taken as UTC). */
extern const char *const cu_months[12];   /* "January" ... */
extern const char *const cu_wdays[7];     /* "Sunday" ... (ds_Days % 7 == 0 is a Sunday) */
LONG cu_days(LONG y, LONG m, LONG d);
void cu_civil(LONG days, LONG *y, LONG *m, LONG *d);
int cu_mdays(LONG y, LONG m);
int cu_parsetime(const char *p, struct DateStamp *ds);
int cu_parsedate(const char *s, struct DateStamp *ds);
void cu_isodate(const struct DateStamp *ds, char *buf);
ULONG cu_unixtime(const struct DateStamp *ds);
void cu_dsfix(struct DateStamp *ds);   /* a stamp after 2046 (negative days from DateStamp()) made right */

/* ---- cu_prot.c ----
 * cu_protstr(): the protection bits as List shows them, "----rwed" (9
 * bytes): h s p a when set, r w e d when allowed (those four are kept
 * inverted: a set bit forbids). */
void cu_protstr(LONG prot, char *b);

/* ---- regex.c ---- */
#define RE_GROUPS 10
#define RE_ICASE 1               /* upper and lower case the same */
#define RE_LONGEST 2             /* POSIX: the longest of the leftmost matches (else the first by the pattern's order) */
#define RE_MULTILINE 4           /* ^ and $ also next to a newline in the text */
void *re_comp(const char *re, int flags);
int re_exec(void *prog, const char *s, int len, int from, int *caps);   /* from: where the search starts; ^ only at s[0] */
void re_free(void *prog);

/* ---- cu_tree.c: files and trees, for cp mv rm (links never followed) ----
 * cu_isroot(): a volume root ("dh0:", "dh0:/", ""), never deleted or moved.
 * cu_lstat(): Examine() a path without following a soft link: a link
 * gives fib_DirEntryType ST_SOFTLINK and only its name. 0: IoErr() says why.
 * cu_scan(): a whole drawer read into a list first (never change a drawer
 * under a live ExNext); 0 and a message (rc 10) on a failure, *ok = 0.
 * cu_copyfile(): the bytes in 128 KB blocks, then protection, date and
 * filenote (Copy CLONE); a failed or broken copy deletes the partial
 * target. 1 fine, else 0, said (rc 10, or 20 for Ctrl-C).
 * cu_clonedir(): CreateDir(dst) with src's protection and filenote;
 * 0: IoErr() says why, nothing said.
 * cu_copytree(): src to dst with a work list on the heap (no recursion),
 * a drawer before its contents. dir() is called for each drawer pair (1:
 * go into it, 0: skip it, -1: stop all), file() for each other entry,
 * links included (1 fine, 0 failed, -1 stop all). Returns 1 when all was
 * fine.
 * cu_deltree(): the tree under top, deepest first: every drawer scanned
 * whole, then del() for its files and links, then (in reverse order) for
 * the drawers. Returns 1 when every del() did.
 * cu_skip()/cu_skipped(): names put aside (rc 5), listed at the end
 * under "title:" on the error stream. */
#define CU_LINK(t) ((t) == ST_SOFTLINK || (t) == ST_LINKDIR)
#define CU_ISDIR(t) ((t) > 0 && !CU_LINK(t))
typedef struct CuDirEnt {
    struct CuDirEnt *next;
    LONG type, prot;
    struct DateStamp date;
    char *note;                  /* NULL: none */
    char name[1];
} CuDirEnt;
int cu_isroot(const char *path);
int cu_lstat(const char *path, struct FileInfoBlock *fib);
CuDirEnt *cu_scan(const char *dir, int *ok);
void cu_freeents(CuDirEnt *e);
int cu_copyfile(const char *src, const char *dst, LONG prot, const struct DateStamp *ds, const char *note);
int cu_clonedir(const char *src, const char *dst);
int cu_copytree(const char *src, const char *dst,
                int (*dir)(const char *s, const char *d, void *u),
                int (*file)(const char *s, const char *d, CuDirEnt *e, void *u), void *u);
int cu_deltree(const char *top, int (*del)(const char *path, LONG type, void *u), void *u);
void cu_skip(const char *path);
void cu_skipped(const char *title);
void cu_trimslash(char *path);   /* "dir/" -> "dir" ("RAM:/" stays) */


/* ---- cu_long.c: paths longer than dos.library takes (255 bytes) ----
 * cu_lockdir(): a shared lock on a path of any length (short ones: plain
 * Lock), locked part by part; 0 and IoErr() when it fails.
 * cu_enter(): for a long path, CurrentDir() moved to its drawer and its
 * last part returned (a short path comes back as it is); do the one dos
 * call with that, then cu_leave() moves back (IoErr() kept). Not nested. */
BPTR cu_lockdir(const char *path);
const char *cu_enter(const char *path);
void cu_leave(void);

/* ---- cu_tasks.c: every task, copied out under Disable() (ps, kill) ---- */
typedef struct {
    APTR addr;
    int process;                 /* a process (else a plain exec task) */
    int pri, state;              /* state: 0 running, 1 ready, 2 waiting */
    ULONG stack, sigwait;        /* stack size; the signals it waits for */
    LONG free;                   /* stack headroom in bytes, -1 unknown */
    LONG cli;                    /* CLI number, 0 when none */
    char name[48];
    char cmd[64];                /* the CLI's command in flight, "" if none */
} CuTask;
int cu_tasks(CuTask *v, int max);          /* how many were filled in */
#define CU_MAXTASKS 256

/* ---- cu_cpu.c: how busy the processor is, each task's share (top, ctop) -
 * Open timer.device and set TimerBase first. Exact through exec's task
 * hooks when this program got them (one at a time), else sampled on the
 * vertical blank. */
typedef struct {
    int pct;                     /* tenths of a percent since the last measure */
    int timed;                   /* exact (else pct is sampled, time 0) */
    unsigned long long time;     /* EClock ticks it ran since cu_cpuopen */
} CuShare;
int cu_cpuopen(void);                      /* 0: no memory */
void cu_cpuclose(void);                    /* everything out again: call on every exit */
void cu_cpudrain(void);                    /* the hooks' ring read: twice a second or more */
int cu_cpumeasure(const CuTask *t, int n, CuShare *s);  /* busy tenths; -1 nothing yet */
ULONG cu_cpuclock(void);                   /* vertical blanks counted: the pace */
int cu_cpurate(void);                      /* ... a second (50, 60 NTSC) */
int cu_cpuexact(void);                     /* the hooks are ours */
ULONG cu_cpufreq(void);                    /* EClock ticks a second (for time) */
int cu_emu68(void);                        /* a PiStorm with Emu68 */
ULONG cu_emu68mips(void);                  /* 68k MIPS since the last call, -1 unknown */

/* ---- cu_pi.c: the Pi under a PiStorm (Emu68), where there is one ----- */
typedef struct {
    char emu68[64];              /* "Emu68 1.1.0 (...)", "" unknown */
    char model[64];              /* "Raspberry Pi Compute Module 4 ..." */
    ULONG temp, tempmax;         /* millidegrees C, 0 unknown */
    ULONG armhz, armmax;         /* the ARM clock now and at most, 0 unknown */
    ULONG throttled;             /* the firmware's flags (bit 0 under-voltage, 2 throttled now, 3 too hot) */
    int hasthrottle;
} CuPi;
int cu_pi(CuPi *p);                        /* 0: no Pi to ask */
int cu_isemu68(void);                      /* devicetree.resource with /emu68 */
int cu_signal(APTR task, ULONG sigs);      /* 0: that task is gone */

#endif
