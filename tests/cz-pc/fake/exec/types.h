/* fake exec/types.h: just enough of the Amiga's types for lib/cu.h and
 * lib/cz*.c to build on Linux (tests/cz-pc, not part of CUtils) */
#ifndef EXEC_TYPES_H
#define EXEC_TYPES_H
#include <stdint.h>
typedef int32_t LONG;
typedef uint32_t ULONG;
typedef int16_t WORD;
typedef uint16_t UWORD;
typedef int8_t BYTE;
typedef uint8_t UBYTE;
typedef void *APTR;
typedef char *STRPTR;
typedef long BPTR;
typedef short BOOL;
#define TRUE 1
#define FALSE 0
#endif
