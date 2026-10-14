#include "cu.h"
#undef main

int cu_rc, cu_argc;
char **cu_argv;
static int g_argc;
static char **g_argv;

void *cu_alloc(ULONG n) { return malloc(n ? n : 1); }
void cu_free(void *p) { free(p); }
char *cu_strdup(const char *s) { return strdup(s); }
int cu_args(const char *usage) { (void)usage; cu_argc = g_argc - 1; cu_argv = g_argv + 1; return cu_argc; }

static int optind_, optpos;
int cu_opt(const char *spec, char **val)
{
    char *w; const char *s; int c;
    for (;;) {
        if (optind_ >= cu_argc) goto done;
        w = cu_argv[optind_];
        if (!optpos) {
            if (w[0] != '-' || !w[1]) goto done;
            if (w[1] == '-' && !w[2]) { optind_++; goto done; }
            optpos = 1;
        }
        c = w[optpos++];
        if (!c) { optind_++; optpos = 0; continue; }
        s = strchr(spec, c);
        if (!s) { cu_err(10, w, "unknown option"); return '?'; }
        if (s[1] == ':') {
            if (w[optpos]) *val = w + optpos;
            else if (optind_ + 1 < cu_argc) *val = cu_argv[++optind_];
            else return '?';
            optind_++; optpos = 0;
        }
        return c;
    }
done:
    cu_argv += optind_; cu_argc -= optind_;
    return -1;
}
long cu_num(const char *s, int *ok) { char *e; long v = strtol(s, &e, 10); *ok = *s && !*e; return v; }
void cu_put(const char *s, LONG n) { fwrite(s, 1, n, stdout); }
void cu_puts(const char *s) { fputs(s, stdout); }
void cu_putc(int c) { putchar(c); }
void cu_putnum(LONG v, int w) { printf("%*d", w, v); }
void cu_flush(void) { fflush(stdout); }
void cu_err(int rc, const char *what, const char *why) { if (cu_rc < rc) cu_rc = rc; fprintf(stderr, "diff: %s: %s\n", what ? what : "", why); }
void cu_fault(int rc, const char *what) { cu_err(rc, what, "failed"); }
int cu_break(void) { return 0; }
int cu_natcmp(const char *a, const char *b) { return strcasecmp(a, b); }
int cu_join(char *buf, const char *dir, const char *name)
{
    int n = strlen(dir);
    if (buf != dir) strcpy(buf, dir);
    if (n && buf[n - 1] != ':' && buf[n - 1] != '/') buf[n++] = '/';
    strcpy(buf + n, name);
    return 1;
}
CuDirEnt *cu_scan(const char *dir, int *ok)
{
    DIR *d = opendir(dir); struct dirent *e; CuDirEnt *h = NULL;
    *ok = d != NULL;
    if (!d) return NULL;
    while ((e = readdir(d))) {
        char p[1024]; struct stat st; CuDirEnt *x;
        if (e->d_name[0] == '.') continue;
        snprintf(p, sizeof p, "%s/%s", dir, e->d_name);
        lstat(p, &st);
        x = calloc(1, sizeof *x + strlen(e->d_name));
        strcpy(x->name, e->d_name);
        x->type = S_ISDIR(st.st_mode) ? 2 : S_ISLNK(st.st_mode) ? 3 : -3;
        x->next = h; h = x;
    }
    closedir(d);
    return h;
}
void cu_freeents(CuDirEnt *e) { while (e) { CuDirEnt *n = e->next; free(e); e = n; } }
void cu_isodate(const struct DateStamp *ds, char *buf) { (void)ds; strcpy(buf, "DATE"); }

BPTR Lock(const char *p, int mode) { struct stat st; (void)mode; if (stat(p, &st)) return 0; return (BPTR)strdup(p); }
void UnLock(BPTR l) { free((void *)l); }
int Examine(BPTR l, struct FileInfoBlock *fib) { struct stat st; if (stat((char *)l, &st)) return 0; fib->fib_DirEntryType = S_ISDIR(st.st_mode) ? 2 : -3; fib->fib_Size = st.st_size; memset(&fib->fib_Date, 0, sizeof fib->fib_Date); return 1; }
void *AllocDosObject(int t, void *x) { (void)t; (void)x; return calloc(1, sizeof(struct FileInfoBlock)); }
void FreeDosObject(int t, void *p) { (void)t; free(p); }
BPTR Open(const char *p, int mode) { (void)mode; FILE *f = fopen(p, "rb"); return (BPTR)f; }
void Close(BPTR f) { fclose((FILE *)f); }
LONG Read(BPTR f, void *b, LONG n) { return fread(b, 1, n, (FILE *)f); }
BPTR Input(void) { return (BPTR)stdin; }
BPTR Output(void) { return (BPTR)stdout; }
int IsInteractive(BPTR f) { (void)f; return 0; }
void DateStamp(struct DateStamp *d) { memset(d, 0, sizeof *d); }
char *FilePart(const char *p) { const char *s = strrchr(p, '/'); return (char *)(s ? s + 1 : p); }

int diff_main(void);
int main(int argc, char **argv) { g_argc = argc; g_argv = argv; return diff_main(); }
