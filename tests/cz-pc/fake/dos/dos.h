/* fake dos/dos.h for the Linux build of the squeezers (tests/cz-pc) */
#ifndef DOS_DOS_H
#define DOS_DOS_H
#include <exec/types.h>
struct DateStamp { LONG ds_Days, ds_Minute, ds_Tick; };
struct FileInfoBlock {
    LONG fib_DiskKey, fib_DirEntryType;
    char fib_FileName[108];
    LONG fib_Protection, fib_EntryType, fib_Size, fib_NumBlocks;
    struct DateStamp fib_Date;
    char fib_Comment[80];
};
#define MODE_OLDFILE 1005
#define MODE_NEWFILE 1006
#define OFFSET_BEGINNING -1
#define OFFSET_CURRENT 0
#define OFFSET_END 1
#define SHARED_LOCK -2
#define ST_SOFTLINK 3
#define ST_LINKDIR 4
#define DOS_FIB 2
#define SIGBREAKF_CTRL_C (1 << 12)
#endif
