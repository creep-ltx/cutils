/* a Linux stand-in for cu.h and the dos calls diff.c uses, to compare
 * its output with GNU diff on many random files (not part of CUtils) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>

typedef int LONG;
typedef unsigned int ULONG;
typedef unsigned char UBYTE;
typedef char *STRPTR;
typedef void *APTR;
typedef long BPTR;
#define TRUE 1
#define FALSE 0
#define SHARED_LOCK -2
#define MODE_OLDFILE 1005
#define DOS_FIB 2
#define TICKS_PER_SECOND 50
#define ST_SOFTLINK 3
#define ST_LINKDIR 4
#define CU_PATH 512
#define CU_LINK(t) ((t) == ST_SOFTLINK || (t) == ST_LINKDIR)
#define CU_ISDIR(t) ((t) > 0 && !CU_LINK(t))
#define BADDR(x) ((void *)(x))
struct DateStamp { LONG ds_Days, ds_Minute, ds_Tick; };
struct FileInfoBlock { LONG fib_DirEntryType; LONG fib_Size; struct DateStamp fib_Date; };
typedef struct CuDirEnt { struct CuDirEnt *next; LONG type, prot; struct DateStamp date; char *note; char name[1]; } CuDirEnt;

extern const char *cu_name;
extern int cu_rc, cu_argc;
extern char **cu_argv;
void *cu_alloc(ULONG n);
void cu_free(void *p);
char *cu_strdup(const char *s);
int cu_args(const char *usage);
int cu_opt(const char *spec, char **val);
long cu_num(const char *s, int *ok);
void cu_put(const char *s, LONG n);
void cu_puts(const char *s);
void cu_putc(int c);
void cu_putnum(LONG v, int width);
void cu_flush(void);
void cu_err(int rc, const char *what, const char *why);
void cu_fault(int rc, const char *what);
int cu_break(void);
int cu_natcmp(const char *a, const char *b);
int cu_join(char *buf, const char *dir, const char *name);
CuDirEnt *cu_scan(const char *dir, int *ok);
void cu_freeents(CuDirEnt *e);
void cu_isodate(const struct DateStamp *ds, char *buf);

BPTR Lock(const char *p, int mode);
void UnLock(BPTR l);
int Examine(BPTR l, struct FileInfoBlock *fib);
void *AllocDosObject(int t, void *x);
void FreeDosObject(int t, void *p);
BPTR Open(const char *p, int mode);
void Close(BPTR f);
LONG Read(BPTR f, void *b, LONG n);
BPTR Input(void);
BPTR Output(void);
int IsInteractive(BPTR f);
void DateStamp(struct DateStamp *d);
char *FilePart(const char *p);
#define main diff_main
