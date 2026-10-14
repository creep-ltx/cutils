/* shim.c - the dos calls and cu_* parts lib/cz*.c needs, done with POSIX,
 * for the Linux build of czt (tests/cz-pc; not part of CUtils). A BPTR is
 * a file descriptor plus one (0 stays "none"). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "cu.h"

int cu_rc;

LONG Read(BPTR fh, APTR buf, LONG n) { return read((int)fh - 1, buf, n); }

LONG Write(BPTR fh, APTR buf, LONG n)
{
    LONG done = 0;
    while (done < n) {
        ssize_t r = write((int)fh - 1, (char *)buf + done, n - done);
        if (r <= 0) return -1;
        done += r;
    }
    return done;
}

BPTR Open(STRPTR name, LONG mode)
{
    int fd = mode == MODE_NEWFILE ? open(name, O_WRONLY | O_CREAT | O_TRUNC, 0644) : open(name, O_RDONLY);
    return fd < 0 ? 0 : fd + 1;
}

void Close(BPTR fh) { if (fh) close((int)fh - 1); }

LONG Seek(BPTR fh, LONG pos, LONG mode)
{
    off_t was = lseek((int)fh - 1, 0, SEEK_CUR);
    if (lseek((int)fh - 1, pos, mode == OFFSET_BEGINNING ? SEEK_SET : mode == OFFSET_END ? SEEK_END : SEEK_CUR) < 0) return -1;
    return was;
}

BPTR Output(void) { return 2; }

/* CZ_MEMLIMIT=bytes: any single block bigger fails, as on a small Amiga
 * (vamos cannot say no: its AllocMem stops the run instead) */
void *cu_alloc(ULONG n)
{
    static long limit = -1;
    if (limit < 0) limit = getenv("CZ_MEMLIMIT") ? atol(getenv("CZ_MEMLIMIT")) : 0;
    if (limit && n > (ULONG)limit) return NULL;
    return malloc(n ? n : 1);
}
void cu_free(void *p) { free(p); }

void cu_err(int rc, const char *what, const char *why)
{
    if (cu_rc < rc) cu_rc = rc;
    fprintf(stderr, "%s: %s: %s\n", cu_name, what ? what : "", why ? why : "");
}
