/* tui.library: the Resident tag, open/close/expunge, and the jump table.
 * Written by hand, not with libnix's libinit (which orders the table by
 * how the linker meets the functions): the table below IS the order in
 * tui.sfd, and that order may never change.
 *
 * Every call runs on the calling program's task, so Input(), Output()
 * and signals are that program's; all state is in its struct Tui (core.c).
 * Built -nostartfiles with no C library: memcpy and friends are here. */
#include <exec/resident.h>
#include <exec/libraries.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <libraries/tui.h>
#include <clib/tui_protos.h>
#include "version.h"

#define REG(r, a) a __asm(#r)

struct ExecBase *SysBase;
struct DosLibrary *DOSBase;
struct IntuitionBase *IntuitionBase;

struct TuiBase {
    struct Library lib;
    BPTR seglist;
};

/* run as a program it does nothing (the first code in the file: start.s
 * puts a moveq #-1,d0 / rts before it) */

static const char libname[] = "tui.library";
static const char libid[] = "tui.library " TUI_VERSTR " (" TUI_DATE ")\r\n";
const char verstag[] = "$VER: tui.library " TUI_VERSTR " (" TUI_DATE ")";

static struct Library *initlib(REG(d0, struct TuiBase *b), REG(a0, BPTR seg), REG(a6, struct ExecBase *sys));
static struct Library *libopen(REG(a6, struct TuiBase *b));
static BPTR libclose(REG(a6, struct TuiBase *b));
static BPTR libexpunge(REG(a6, struct TuiBase *b));
static LONG libnull(void);

/* the calls: registers as in tui.sfd, on to core.c */
static LONG L_Possible(REG(d1, BPTR in)) { return TuiPossible(in); }
static struct Tui *L_Open(REG(d1, BPTR in)) { return TuiOpen(in); }
static void L_Close(REG(a0, struct Tui *t)) { TuiClose(t); }
static void L_Suspend(REG(a0, struct Tui *t)) { TuiSuspend(t); }
static void L_Resume(REG(a0, struct Tui *t)) { TuiResume(t); }
static void L_KeyAfter(REG(a0, struct Tui *t), REG(a1, CONST_STRPTR m)) { TuiKeyAfter(t, m); }
static void L_Put(REG(a0, struct Tui *t), REG(d0, LONG col), REG(d1, LONG row), REG(a1, CONST_STRPTR s),
                  REG(d2, LONG len), REG(d3, LONG fg), REG(d4, LONG bg), REG(d5, LONG at))
{ TuiPut(t, col, row, s, len, fg, bg, at); }
static void L_PutC(REG(a0, struct Tui *t), REG(d0, LONG col), REG(d1, LONG row), REG(d2, ULONG c),
                   REG(d3, LONG fg), REG(d4, LONG bg), REG(d5, LONG at))
{ TuiPutC(t, col, row, c, fg, bg, at); }
static void L_Fill(REG(a0, struct Tui *t), REG(d0, LONG col), REG(d1, LONG row), REG(d2, LONG w),
                   REG(d3, ULONG c), REG(d4, LONG fg), REG(d5, LONG bg), REG(d6, LONG at))
{ TuiFill(t, col, row, w, c, fg, bg, at); }
static void L_Clear(REG(a0, struct Tui *t)) { TuiClear(t); }
static void L_Cursor(REG(a0, struct Tui *t), REG(d0, LONG col), REG(d1, LONG row), REG(d2, LONG on))
{ TuiCursor(t, col, row, on); }
static void L_Flush(REG(a0, struct Tui *t)) { TuiFlush(t); }
static void L_Redraw(REG(a0, struct Tui *t)) { TuiRedraw(t); }
static LONG L_Resized(REG(a0, struct Tui *t)) { return TuiResized(t); }
static void L_Title(REG(a0, struct Tui *t), REG(a1, CONST_STRPTR s)) { TuiTitle(t, s); }
static LONG L_Wait(REG(a0, struct Tui *t), REG(a1, TuiEv *e), REG(d0, LONG us), REG(d1, ULONG extra),
                   REG(a2, ULONG *got))
{ return TuiWait(t, e, us, extra, got); }
static void L_Beep(REG(a0, struct Tui *t)) { TuiBeep(t); }
static ULONG L_Box(REG(a0, struct Tui *t), REG(d0, LONG which)) { return TuiBox(t, which); }
static void L_Dirty(REG(a0, struct Tui *t), REG(d0, LONG col), REG(d1, LONG row), REG(d2, LONG w),
                    REG(d3, LONG h))
{ TuiDirty(t, col, row, w, h); }
static void L_Raw(REG(a0, struct Tui *t), REG(d0, LONG col), REG(d1, LONG row), REG(a1, CONST_STRPTR s),
                  REG(d2, LONG n))
{ TuiRaw(t, col, row, s, n); }
static LONG L_PutUTF8(REG(a0, struct Tui *t), REG(d0, LONG col), REG(d1, LONG row), REG(a1, CONST_STRPTR s),
                      REG(d2, LONG len), REG(d3, LONG fg), REG(d4, LONG bg), REG(d5, LONG at))
{ return TuiPutUTF8(t, col, row, s, len, fg, bg, at); }

/* in tui.sfd's order, after exec's four: never reorder, only add */
static const APTR functable[] = {
    (APTR)libopen, (APTR)libclose, (APTR)libexpunge, (APTR)libnull,
    /* version 1 */
    (APTR)L_Possible, (APTR)L_Open, (APTR)L_Close, (APTR)L_Suspend,
    (APTR)L_Resume, (APTR)L_KeyAfter, (APTR)L_Put, (APTR)L_PutC,
    (APTR)L_Fill, (APTR)L_Clear, (APTR)L_Cursor, (APTR)L_Flush,
    (APTR)L_Redraw, (APTR)L_Resized, (APTR)L_Title, (APTR)L_Wait,
    (APTR)L_Beep, (APTR)L_Box, (APTR)L_Dirty, (APTR)L_Raw,
    /* version 2 */
    (APTR)L_PutUTF8,
    (APTR)-1
};

static const ULONG inittab[4] = {
    sizeof(struct TuiBase), (ULONG)functable, 0, (ULONG)initlib
};

const struct Resident romtag = {
    RTC_MATCHWORD, (struct Resident *)&romtag, (APTR)(&romtag + 1),
    RTF_AUTOINIT, TUI_VERSION, NT_LIBRARY, 0,
    (char *)libname, (char *)libid, (APTR)inittab
};

static struct Library *initlib(REG(d0, struct TuiBase *b), REG(a0, BPTR seg), REG(a6, struct ExecBase *sys))
{
    SysBase = sys;
    b->lib.lib_Node.ln_Type = NT_LIBRARY;
    b->lib.lib_Node.ln_Name = (char *)libname;
    b->lib.lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
    b->lib.lib_Version = TUI_VERSION;
    b->lib.lib_Revision = TUI_REVISION;
    b->lib.lib_IdString = (APTR)libid;
    b->seglist = seg;
    DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 37);
    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 37);
    if (!DOSBase || !IntuitionBase) {
        if (DOSBase) CloseLibrary((struct Library *)DOSBase);
        if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
        FreeMem((UBYTE *)b - b->lib.lib_NegSize, b->lib.lib_NegSize + b->lib.lib_PosSize);
        return NULL;
    }
    return &b->lib;
}

static struct Library *libopen(REG(a6, struct TuiBase *b))
{
    b->lib.lib_OpenCnt++;
    b->lib.lib_Flags &= ~LIBF_DELEXP;
    return &b->lib;
}

static BPTR libexpunge(REG(a6, struct TuiBase *b))
{
    BPTR seg;
    if (b->lib.lib_OpenCnt) {
        b->lib.lib_Flags |= LIBF_DELEXP;   /* when the last one closes */
        return 0;
    }
    seg = b->seglist;
    Remove(&b->lib.lib_Node);
    CloseLibrary((struct Library *)IntuitionBase);
    CloseLibrary((struct Library *)DOSBase);
    FreeMem((UBYTE *)b - b->lib.lib_NegSize, b->lib.lib_NegSize + b->lib.lib_PosSize);
    return seg;
}

static BPTR libclose(REG(a6, struct TuiBase *b))
{
    if (--b->lib.lib_OpenCnt == 0 && (b->lib.lib_Flags & LIBF_DELEXP))
        return libexpunge(b);
    return 0;
}

static LONG libnull(void)
{
    return 0;
}

/* no C library in here: what gcc and core.c call */
void *memcpy(void *d, const void *s, unsigned long n)
{
    UBYTE *a = d;
    const UBYTE *b = s;
    while (n--) *a++ = *b++;
    return d;
}

void *memmove(void *d, const void *s, unsigned long n)
{
    UBYTE *a = d;
    const UBYTE *b = s;
    if (a < b) while (n--) *a++ = *b++;
    else { a += n; b += n; while (n--) *--a = *--b; }
    return d;
}

void *memset(void *d, int c, unsigned long n)
{
    UBYTE *a = d;
    while (n--) *a++ = (UBYTE)c;
    return d;
}

int memcmp(const void *x, const void *y, unsigned long n)
{
    const UBYTE *a = x, *b = y;
    for (; n; n--, a++, b++)
        if (*a != *b) return *a - *b;
    return 0;
}

unsigned long strlen(const char *s)
{
    const char *e = s;
    while (*e) e++;
    return e - s;
}
