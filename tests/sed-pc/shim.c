/* shim.c - the cu_* parts and dos calls sed.c needs, done with POSIX, for
 * the Linux build (tests/sed-pc; not part of CUtils). */
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "cu.h"

int cu_rc, cu_argc;
char **cu_argv;
static int g_argc;
static char **g_argv;

void *cu_alloc(ULONG n) { return malloc(n ? n : 1); }
void cu_free(void *p) { free(p); }
char *cu_strdup(const char *s) { return strdup(s); }
int cu_args(const char *usage) { (void)usage; cu_argc = g_argc - 1; cu_argv = g_argv + 1; return cu_argc; }
long cu_num(const char *s, int *ok) { char *e; long v = strtol(s, &e, 10); *ok = *s && !*e; return v; }
void cu_put(const char *s, LONG n) { fwrite(s, 1, n, stdout); }
void cu_puts(const char *s) { fputs(s, stdout); }
void cu_putc(int c) { putchar(c); }
void cu_flush(void) { fflush(stdout); }
void cu_err(int rc, const char *what, const char *why)
{
    if (cu_rc < rc) cu_rc = rc;
    fflush(stdout);
    fprintf(stderr, "%s", cu_name);
    if (what) fprintf(stderr, ": %s", what);
    if (why) fprintf(stderr, ": %s", why);
    fprintf(stderr, "\n");
}
void cu_fault(int rc, const char *what) { cu_err(rc, what, strerror(errno)); }
int cu_break(void) { return 0; }
int cu_each(const char *arg, int (*fn)(const char *path, void *u), void *u) { return fn(arg, u); }
int cu_join(char *buf, const char *dir, const char *name)
{
    int n = strlen(dir);
    if (buf != dir) strcpy(buf, dir);
    if (n && buf[n - 1] != ':' && buf[n - 1] != '/') buf[n++] = '/';
    strcpy(buf + n, name);
    return 1;
}

BPTR Lock(STRPTR p, LONG mode) { struct stat st; (void)mode; if (stat(p, &st)) return 0; return (BPTR)strdup(p); }
void UnLock(BPTR l) { free((void *)l); }
int Examine(BPTR l, struct FileInfoBlock *fib)
{
    struct stat st;
    if (stat((char *)l, &st)) return 0;
    fib->fib_DirEntryType = S_ISDIR(st.st_mode) ? 2 : -3;
    fib->fib_Size = st.st_size;
    fib->fib_Protection = st.st_mode & 0777;     /* the Unix mode stands in for the bits */
    fib->fib_Comment[0] = 0;
    return 1;
}
void *AllocDosObject(LONG t, void *x) { (void)t; (void)x; return calloc(1, sizeof(struct FileInfoBlock)); }
void FreeDosObject(LONG t, void *p) { (void)t; free(p); }
BPTR Open(STRPTR p, LONG mode)
{
    int fd = mode == MODE_NEWFILE ? open(p, O_WRONLY | O_CREAT | O_TRUNC, 0644) : open(p, O_RDONLY);
    return fd < 0 ? 0 : fd + 1;
}
void Close(BPTR f) { if (f) close((int)f - 1); }
LONG Read(BPTR f, void *b, LONG n) { return read((int)f - 1, b, n); }
LONG Write(BPTR f, void *b, LONG n)
{
    LONG done = 0;
    if (f == 2) fflush(stdout);
    while (done < n) {
        ssize_t r = write((int)f - 1, (char *)b + done, n - done);
        if (r <= 0) return -1;
        done += r;
    }
    return done;
}
BPTR Input(void) { return 1; }
BPTR Output(void) { return 2; }
static struct Process me = { 3 };
void *FindTask(void *name) { (void)name; return &me; }
int DeleteFile(STRPTR p) { return unlink(p) == 0; }
int Rename(STRPTR a, STRPTR b) { struct stat st; if (!stat(b, &st)) { errno = EEXIST; return 0; } return rename(a, b) == 0; }
int SetProtection(STRPTR p, LONG prot) { return chmod(p, prot) == 0; }
int SetComment(STRPTR p, STRPTR note) { (void)p; (void)note; return 1; }
STRPTR PathPart(STRPTR p) { char *s = strrchr(p, '/'); return s ? s : p; }
STRPTR FilePart(STRPTR p) { char *s = strrchr(p, '/'); return s ? s + 1 : p; }

int sed_main(void);
int main(int argc, char **argv) { g_argc = argc; g_argv = argv; return sed_main(); }
