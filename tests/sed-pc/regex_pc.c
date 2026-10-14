/* lib/regex.c with the stand-in cu.h (its own #include "cu.h" then finds
 * lib/cu.h, which the guard skips) */
#include "cu.h"
#include "../../lib/regex.c"
