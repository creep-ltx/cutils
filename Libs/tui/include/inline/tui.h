/* Automatically generated header (sfdc 1.11e)! Do not edit! */

#ifndef _INLINE_TUI_H
#define _INLINE_TUI_H

#ifndef _SFDC_VARARG_DEFINED
#define _SFDC_VARARG_DEFINED
#ifdef __HAVE_IPTR_ATTR__
typedef APTR _sfdc_vararg __attribute__((iptr));
#else
typedef ULONG _sfdc_vararg;
#endif /* __HAVE_IPTR_ATTR__ */
#endif /* _SFDC_VARARG_DEFINED */

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif /* !__INLINE_MACROS_H */

#ifndef TUI_BASE_NAME
#define TUI_BASE_NAME TuiBase
#endif /* !TUI_BASE_NAME */

#define TuiPossible(___in) \
      LP1(0x1e, LONG, TuiPossible , BPTR, ___in, d1,\
      , TUI_BASE_NAME)

#define TuiOpen(___in) \
      LP1(0x24, struct Tui *, TuiOpen , BPTR, ___in, d1,\
      , TUI_BASE_NAME)

#define TuiClose(___t) \
      LP1NR(0x2a, TuiClose , struct Tui *, ___t, a0,\
      , TUI_BASE_NAME)

#define TuiSuspend(___t) \
      LP1NR(0x30, TuiSuspend , struct Tui *, ___t, a0,\
      , TUI_BASE_NAME)

#define TuiResume(___t) \
      LP1NR(0x36, TuiResume , struct Tui *, ___t, a0,\
      , TUI_BASE_NAME)

#define TuiKeyAfter(___t, ___msg) \
      LP2NR(0x3c, TuiKeyAfter , struct Tui *, ___t, a0, CONST_STRPTR, ___msg, a1,\
      , TUI_BASE_NAME)

#define TuiPut(___t, ___col, ___row, ___s, ___len, ___fg, ___bg, ___attr) \
      LP8NR(0x42, TuiPut , struct Tui *, ___t, a0, LONG, ___col, d0, LONG, ___row, d1, CONST_STRPTR, ___s, a1, LONG, ___len, d2, LONG, ___fg, d3, LONG, ___bg, d4, LONG, ___attr, d5,\
      , TUI_BASE_NAME)

#define TuiPutC(___t, ___col, ___row, ___c, ___fg, ___bg, ___attr) \
      LP7NR(0x48, TuiPutC , struct Tui *, ___t, a0, LONG, ___col, d0, LONG, ___row, d1, ULONG, ___c, d2, LONG, ___fg, d3, LONG, ___bg, d4, LONG, ___attr, d5,\
      , TUI_BASE_NAME)

#define TuiFill(___t, ___col, ___row, ___w, ___c, ___fg, ___bg, ___attr) \
      LP8NR(0x4e, TuiFill , struct Tui *, ___t, a0, LONG, ___col, d0, LONG, ___row, d1, LONG, ___w, d2, ULONG, ___c, d3, LONG, ___fg, d4, LONG, ___bg, d5, LONG, ___attr, d6,\
      , TUI_BASE_NAME)

#define TuiClear(___t) \
      LP1NR(0x54, TuiClear , struct Tui *, ___t, a0,\
      , TUI_BASE_NAME)

#define TuiCursor(___t, ___col, ___row, ___on) \
      LP4NR(0x5a, TuiCursor , struct Tui *, ___t, a0, LONG, ___col, d0, LONG, ___row, d1, LONG, ___on, d2,\
      , TUI_BASE_NAME)

#define TuiFlush(___t) \
      LP1NR(0x60, TuiFlush , struct Tui *, ___t, a0,\
      , TUI_BASE_NAME)

#define TuiRedraw(___t) \
      LP1NR(0x66, TuiRedraw , struct Tui *, ___t, a0,\
      , TUI_BASE_NAME)

#define TuiResized(___t) \
      LP1(0x6c, LONG, TuiResized , struct Tui *, ___t, a0,\
      , TUI_BASE_NAME)

#define TuiTitle(___t, ___title) \
      LP2NR(0x72, TuiTitle , struct Tui *, ___t, a0, CONST_STRPTR, ___title, a1,\
      , TUI_BASE_NAME)

#define TuiWait(___t, ___e, ___us, ___extra, ___got) \
      LP5(0x78, LONG, TuiWait , struct Tui *, ___t, a0, TuiEv *, ___e, a1, LONG, ___us, d0, ULONG, ___extra, d1, ULONG *, ___got, a2,\
      , TUI_BASE_NAME)

#define TuiBeep(___t) \
      LP1NR(0x7e, TuiBeep , struct Tui *, ___t, a0,\
      , TUI_BASE_NAME)

#define TuiBox(___t, ___which) \
      LP2(0x84, ULONG, TuiBox , struct Tui *, ___t, a0, LONG, ___which, d0,\
      , TUI_BASE_NAME)

#define TuiDirty(___t, ___col, ___row, ___w, ___h) \
      LP5NR(0x8a, TuiDirty , struct Tui *, ___t, a0, LONG, ___col, d0, LONG, ___row, d1, LONG, ___w, d2, LONG, ___h, d3,\
      , TUI_BASE_NAME)

#define TuiRaw(___t, ___col, ___row, ___s, ___n) \
      LP5NR(0x90, TuiRaw , struct Tui *, ___t, a0, LONG, ___col, d0, LONG, ___row, d1, CONST_STRPTR, ___s, a1, LONG, ___n, d2,\
      , TUI_BASE_NAME)

#endif /* !_INLINE_TUI_H */
