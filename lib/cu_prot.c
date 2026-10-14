/* cu_prot.c - protection bits as text (cu.h): stat, chmod, file. */
#include "cu.h"

/* "hsparwed" as List shows it: h s p a when set, r w e d when allowed */
void cu_protstr(LONG prot, char *b)
{
    static const char l[] = "hsparwed";
    int i;
    for (i = 0; i < 8; i++) {
        int on = (prot >> (7 - i)) & 1;
        b[i] = (i < 4 ? on : !on) ? l[i] : '-';
    }
    b[8] = 0;
}
