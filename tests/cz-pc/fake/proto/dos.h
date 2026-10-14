/* fake proto/dos.h: the dos calls lib/cz*.c and czt.c use, done with
 * POSIX in shim.c (tests/cz-pc) */
#ifndef PROTO_DOS_H
#define PROTO_DOS_H
#include <dos/dos.h>
#include <strings.h>
LONG Read(BPTR fh, APTR buf, LONG n);
LONG Write(BPTR fh, APTR buf, LONG n);
BPTR Open(STRPTR name, LONG mode);
void Close(BPTR fh);
BPTR Output(void);
LONG Seek(BPTR fh, LONG pos, LONG mode);
#define stricmp strcasecmp
#endif
