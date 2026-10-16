/* Automatically generated header (sfdc 1.11e)! Do not edit! */

#ifndef PROTO_TUI_H
#define PROTO_TUI_H

#include <clib/tui_protos.h>

#if defined(_CONST_BASES)
# ifndef __CONSTLIBBASEDECL__
# define __CONSTLIBBASEDECL__ const
# endif /* __CONSTLIBBASEDECL__ */
# ifndef __SEGMENTLIBBASEDECL__
# define __SEGMENTLIBBASEDECL__  __attribute__((__section__(".data")))
# endif /* __SEGMENTLIBBASEDECL__ */
#endif /* _CONST_BASES */
#ifdef __amigaos4__
# include <interfaces/tui.h>
# ifndef __NOGLOBALIFACE__
   extern struct TuiIFace *ITui;
# endif /* __NOGLOBALIFACE__*/
#endif /* !__amigaos4__ */
#ifndef __NOLIBBASE__
  extern struct Library *
# ifdef __CONSTLIBBASEDECL__
   __CONSTLIBBASEDECL__
# endif /* __CONSTLIBBASEDECL__ */
  TuiBase
# ifdef __SEGMENTLIBBASEDECL__
 __SEGMENTLIBBASEDECL__
# endif /* __SEGMENTLIBBASEDECL__ */
;
#endif /* !__NOLIBBASE__ */

#ifndef _NO_INLINE
# if defined(__GNUC__)
#  ifdef __AROS__
#   include <defines/tui.h>
#  else
#   include <inline/tui.h>
#  endif
# else
#  include <pragmas/tui_pragmas.h>
# endif
#endif /* _NO_INLINE */

#endif /* !PROTO_TUI_H */
