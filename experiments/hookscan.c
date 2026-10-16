/* hookscan - every task with TF_SWITCH / TF_LAUNCH set, and where its
 * hooks point. After top has ended there should be none: a hooked task
 * then calls into whatever the hook memory has become. */
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <proto/exec.h>
#include <proto/dos.h>

const char version[] = "$VER: hookscan 0.1 (16.10.2026)";

#define MAX 128
int main(void)
{
    struct { APTR t, sw, la; UBYTE fl; char name[32]; } v[MAX];
    struct Node *n;
    struct List *l[2];
    int c = 0, i, k, total = 0;
    l[0] = &SysBase->TaskReady;
    l[1] = &SysBase->TaskWait;
    Disable();
    for (k = 0; k < 2; k++)
        for (n = l[k]->lh_Head; n->ln_Succ; n = n->ln_Succ) {
            struct Task *t = (struct Task *)n;
            total++;
            if (!(t->tc_Flags & (TF_SWITCH | TF_LAUNCH)) || c == MAX) continue;
            v[c].t = t; v[c].sw = t->tc_Switch; v[c].la = t->tc_Launch; v[c].fl = t->tc_Flags;
            for (i = 0; i < 31 && n->ln_Name && n->ln_Name[i]; i++) v[c].name[i] = n->ln_Name[i];
            v[c].name[i] = 0;
            c++;
        }
    Enable();
    Printf("%ld tasks, %ld with switch/launch hooks\n", (LONG)total, (LONG)c);
    for (i = 0; i < c; i++)
        Printf("  %08lx flags %02lx switch %08lx launch %08lx %s\n", (ULONG)v[i].t, (ULONG)v[i].fl,
               (ULONG)v[i].sw, (ULONG)v[i].la, v[i].name);
    return 0;
}
