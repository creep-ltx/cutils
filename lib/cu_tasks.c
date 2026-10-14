/* cu_tasks.c - every task on the machine, copied out (ps, kill, top).
 *
 * ThisTask plus the TaskReady and TaskWait lists are the scheduler's
 * working state, so the walk happens under Disable() and copies
 * everything out: a task pointer is only trustworthy while interrupts
 * stay off (wasabid's ps does the same, wasabi/wasabid.c). */
#include <stdlib.h>
#include "cu.h"

static void copyname(char *d, int size, const char *s)
{
    int i = 0;
    if (s) for (; s[i] && i < size - 1; i++) d[i] = s[i];
    d[i] = 0;
}

/* headroom: bytes between the stack pointer and the bottom; -1 when the
 * pointer is outside the task's own stack (a program that swapped) */
static LONG headroom(struct Task *t, APTR sp)
{
    if ((char *)sp <= (char *)t->tc_SPLower || (char *)sp > (char *)t->tc_SPUpper) return -1;
    return (LONG)((char *)sp - (char *)t->tc_SPLower);
}

static void add(CuTask *v, int *n, int max, struct Task *t, int state, APTR sp)
{
    CuTask *e;
    if (*n >= max) return;
    e = &v[(*n)++];
    e->addr = t;
    e->process = t->tc_Node.ln_Type == NT_PROCESS;
    e->pri = t->tc_Node.ln_Pri;
    e->state = state;
    e->stack = (ULONG)((char *)t->tc_SPUpper - (char *)t->tc_SPLower);
    e->free = headroom(t, sp ? sp : t->tc_SPReg);
    e->sigwait = t->tc_SigWait;
    e->cli = 0;
    e->cmd[0] = 0;
    copyname(e->name, sizeof e->name, t->tc_Node.ln_Name);
    if (e->process) {
        struct Process *pr = (struct Process *)t;
        struct CommandLineInterface *cli = BADDR(pr->pr_CLI);
        if (cli) {
            UBYTE *b = BADDR(cli->cli_CommandName);
            e->cli = pr->pr_TaskNum;
            if (b && b[0]) {                 /* a BSTR: length, then bytes */
                int k = b[0] < sizeof e->cmd - 1 ? b[0] : sizeof e->cmd - 1;
                memcpy(e->cmd, b + 1, k);
                e->cmd[k] = 0;
            }
        }
    }
}

int cu_tasks(CuTask *v, int max)
{
    struct Task *t;
    int n = 0;
    char here;                               /* its address is our own sp */
    Disable();
    add(v, &n, max, SysBase->ThisTask, 0, &here);
    for (t = (struct Task *)SysBase->TaskReady.lh_Head; t->tc_Node.ln_Succ; t = (struct Task *)t->tc_Node.ln_Succ)
        add(v, &n, max, t, 1, NULL);
    for (t = (struct Task *)SysBase->TaskWait.lh_Head; t->tc_Node.ln_Succ; t = (struct Task *)t->tc_Node.ln_Succ)
        add(v, &n, max, t, 2, NULL);
    Enable();
    return n;
}

int cu_signal(APTR task, ULONG sigs)
{
    struct Task *t;
    int alive = 0;
    Disable();
    if (task == SysBase->ThisTask) alive = 1;
    for (t = (struct Task *)SysBase->TaskReady.lh_Head; !alive && t->tc_Node.ln_Succ; t = (struct Task *)t->tc_Node.ln_Succ)
        if (t == task) alive = 1;
    for (t = (struct Task *)SysBase->TaskWait.lh_Head; !alive && t->tc_Node.ln_Succ; t = (struct Task *)t->tc_Node.ln_Succ)
        if (t == task) alive = 1;
    if (alive) Signal(task, sigs);
    Enable();
    return alive;
}
