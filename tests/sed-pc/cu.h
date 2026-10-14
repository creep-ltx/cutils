/* a Linux stand-in for cu.h and the dos calls sed.c uses, to compare it
 * with GNU sed on many scripts (tests/sed-pc/run.sh; not part of CUtils).
 * A BPTR is a file descriptor plus one (0 stays "none"); a lock a strdup'd path. */
#ifndef CU_H
#define CU_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
#define MODE_NEWFILE 1006
#define DOS_FIB 2
#define ST_SOFTLINK 3
#define CU_PATH 512
struct FileInfoBlock { LONG fib_DirEntryType; LONG fib_Size; LONG fib_Protection; char fib_Comment[80]; };
struct Process { BPTR pr_CES; };

extern const char *cu_name;
extern int cu_rc, cu_argc;
extern char **cu_argv;
void *cu_alloc(ULONG n);
void cu_free(void *p);
char *cu_strdup(const char *s);
int cu_args(const char *usage);
long cu_num(const char *s, int *ok);
void cu_put(const char *s, LONG n);
void cu_puts(const char *s);
void cu_putc(int c);
void cu_flush(void);
void cu_err(int rc, const char *what, const char *why);
void cu_fault(int rc, const char *what);
int cu_break(void);
int cu_each(const char *arg, int (*fn)(const char *path, void *u), void *u);
int cu_join(char *buf, const char *dir, const char *name);

#define RE_GROUPS 10
#define RE_ICASE 1
#define RE_LONGEST 2
#define RE_MULTILINE 4
void *re_comp(const char *re, int flags);
int re_exec(void *prog, const char *s, int len, int from, int *caps);
void re_free(void *prog);

BPTR Lock(STRPTR p, LONG mode);
void UnLock(BPTR l);
int Examine(BPTR l, struct FileInfoBlock *fib);
void *AllocDosObject(LONG t, void *x);
void FreeDosObject(LONG t, void *p);
BPTR Open(STRPTR p, LONG mode);
void Close(BPTR f);
LONG Read(BPTR f, void *b, LONG n);
LONG Write(BPTR f, void *b, LONG n);
BPTR Input(void);
BPTR Output(void);
void *FindTask(void *name);
int DeleteFile(STRPTR p);
int Rename(STRPTR a, STRPTR b);
int SetProtection(STRPTR p, LONG prot);
int SetComment(STRPTR p, STRPTR note);
STRPTR PathPart(STRPTR p);
STRPTR FilePart(STRPTR p);
#endif
