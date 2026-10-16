/* Automatically generated header (sfdc 1.11e)! Do not edit! */

#ifndef CLIB_TUI_PROTOS_H
#define CLIB_TUI_PROTOS_H

/*
**   $VER: tui_protos.h 2.0 $Id: tui.sfd 2.0 $
**
**   C prototypes. For use with 32 bit integers only.
**
**   Copyright (c) 2001 Amiga, Inc.
**       All Rights Reserved
*/

#include <exec/types.h>
#include <dos/dos.h>
#include <libraries/tui.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* tui.library: the toolkit for programs inside the terminal. */
/* The order below is the library's jump table: never move a line, */
/* only add new ones at the end (and raise the version). */
/* --- version 1 --- */
LONG TuiPossible(BPTR in);
struct Tui * TuiOpen(BPTR in);
VOID TuiClose(struct Tui * t);
VOID TuiSuspend(struct Tui * t);
VOID TuiResume(struct Tui * t);
VOID TuiKeyAfter(struct Tui * t, CONST_STRPTR msg);
VOID TuiPut(struct Tui * t, LONG col, LONG row, CONST_STRPTR s, LONG len, LONG fg, LONG bg, LONG attr);
VOID TuiPutC(struct Tui * t, LONG col, LONG row, ULONG c, LONG fg, LONG bg, LONG attr);
VOID TuiFill(struct Tui * t, LONG col, LONG row, LONG w, ULONG c, LONG fg, LONG bg, LONG attr);
VOID TuiClear(struct Tui * t);
VOID TuiCursor(struct Tui * t, LONG col, LONG row, LONG on);
VOID TuiFlush(struct Tui * t);
VOID TuiRedraw(struct Tui * t);
LONG TuiResized(struct Tui * t);
VOID TuiTitle(struct Tui * t, CONST_STRPTR title);
LONG TuiWait(struct Tui * t, TuiEv * e, LONG us, ULONG extra, ULONG * got);
VOID TuiBeep(struct Tui * t);
ULONG TuiBox(struct Tui * t, LONG which);
VOID TuiDirty(struct Tui * t, LONG col, LONG row, LONG w, LONG h);
VOID TuiRaw(struct Tui * t, LONG col, LONG row, CONST_STRPTR s, LONG n);

/* --- version 2 --- */
LONG TuiPutUTF8(struct Tui * t, LONG col, LONG row, CONST_STRPTR s, LONG len, LONG fg, LONG bg, LONG attr);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CLIB_TUI_PROTOS_H */
