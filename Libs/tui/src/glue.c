/* glue - the old tui_* calls that are more than a macro (include/tui.h):
 * the one terminal a program has, and tui.library opened when it is
 * first needed (load on demand) and closed with the terminal.
 * Built twice: over the library, and -DTUI_STATIC over libtui.a. */
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include "tui.h"

struct Tui *tui_h;
const char *tui_keymsg;
static BPTR conin;

#ifndef TUI_STATIC
struct Library *TuiBase;
#define STR_(x) #x
#define VERS STR_(TUIVERSION)      /* the version this program was built against */
static int told;

/* tui.library, opened once; missing: said once, on the error stream */
static int haslib(void)
{
    if (TuiBase) return 1;
    TuiBase = OpenLibrary(TUINAME, TUIVERSION);
    if (!TuiBase && !told) {
        struct Process *me = (struct Process *)FindTask(NULL);
        BPTR err = me->pr_CES ? me->pr_CES : Output();
        static const char m[] = "needs tui.library (version " VERS " or newer) in LIBS:\n";
        told = 1;
        if (err) Write(err, (APTR)m, sizeof(m) - 1);
    }
    return TuiBase != NULL;
}

static void droplib(void)
{
    if (TuiBase && !tui_h) {
        CloseLibrary(TuiBase);
        TuiBase = NULL;
    }
}
#else
#define haslib() 1
#define droplib()
#endif

void tui_console(BPTR in)
{
    conin = in;
}

int tui_possible(void)
{
    struct FileHandle *fh;
    BPTR in = conin ? conin : Input(), o = Output();
    int yes;
    /* not a console: no need for the library (a pipe, a file) */
    if (!in || !o || !IsInteractive(in) || !IsInteractive(o)) return 0;
    fh = (struct FileHandle *)BADDR(in);
    if (!fh->fh_Type) return 0;
    if (!haslib()) return 0;
    yes = TuiPossible(conin);
    droplib();
    return yes;
}

int tui_open(void)
{
    if (tui_h) return 1;
    if (!haslib()) return 0;
    tui_h = TuiOpen(conin);
    if (!tui_h) droplib();
    return tui_h != NULL;
}

void tui_close(void)
{
    if (!tui_h) return;
    TuiClose(tui_h);
    tui_h = NULL;
    droplib();
}
