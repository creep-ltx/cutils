/* cu_long.c - paths longer than dos.library takes (cu.h). A name given to
 * Lock() or Open() is a BSTR: 255 bytes at most, and a deep tree (or a tar
 * member from Linux) has longer paths. Such a path is locked part by part,
 * each piece relative to the lock before it. */
#include "cu.h"

#define DOSMAX 240               /* a piece handed to dos.library */

BPTR cu_lockdir(const char *path)
{
    static char seg[DOSMAX + 4];
    const char *p = path;
    BPTR l = 0, nl, old;
    if (strlen(path) < DOSMAX) return Lock((STRPTR)path, SHARED_LOCK);
    while (*p) {
        const char *e, *cut = NULL;
        int k;
        if (*p == '/') { strcpy(seg, "/"); k = 1; cut = p + 1; }          /* up: a piece of its own */
        else {
            for (e = p; *e && e - p < DOSMAX; e++) if (*e == '/' || *e == ':') cut = e;
            if (!*e) cut = e;
            else if (!cut || cut == p) { if (l) UnLock(l); SetIoErr(ERROR_LINE_TOO_LONG); return 0; }
            k = cut - p + (*cut == ':');                                     /* a volume keeps its colon */
            memcpy(seg, p, k);
            seg[k] = 0;
            if (*cut) cut++;
        }
        if (l) { old = CurrentDir(l); nl = Lock(seg, SHARED_LOCK); CurrentDir(old); UnLock(l); }
        else nl = Lock(seg, SHARED_LOCK);
        if (!(l = nl)) return 0;
        p = cut;
    }
    return l;
}

static BPTR eold;
static int ein;

const char *cu_enter(const char *path)
{
    char *dir;
    int n;
    BPTR l;
    if (ein || strlen(path) < DOSMAX) return path;
    n = (const char *)PathPart((STRPTR)path) - path;
    if (!(dir = cu_alloc(n + 1))) return path;
    memcpy(dir, path, n);
    dir[n] = 0;
    l = cu_lockdir(dir);
    cu_free(dir);
    if (!l) return path;
    eold = CurrentDir(l);
    ein = 1;
    return (const char *)FilePart((STRPTR)path);
}

void cu_leave(void)
{
    if (ein) {
        LONG e = IoErr();
        UnLock(CurrentDir(eold));
        ein = 0;
        SetIoErr(e);
    }
}
