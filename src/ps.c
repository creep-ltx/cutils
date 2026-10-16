/* ps - the programs and tasks that run.
 *
 *   ps [-elw] [name | pattern ...]
 *
 * With no flags: every Shell (CLI) process, as Status shows them, with the
 * command it runs now. Names or patterns (#? or *) keep only the tasks
 * whose name or command matches.
 *   -e every task and process on the machine, not only the Shells
 *   -l long: also the stack's size, how much of it is still free, and the
 *      signals the task waits for
 *   -w wide: names and commands not cut to the window
 *
 * The columns: CLI the Shell's number (kill takes it), ADDRESS the task
 * (kill takes 0x... too), PRI its priority, STATE run (the one running:
 * ps itself), ready (waiting for the CPU), wait (waiting for something),
 * COMMAND what the Shell runs now, or [the task's name] when it runs
 * nothing (a Shell at its prompt, a device, a handler).
 *
 * Return code 0, 5 a name matched nothing, 10 bad arguments. */
#include <stdlib.h>
#include "cu.h"

const char *cu_name = "ps";
static const char version[] = "$VER: ps 0.1b2 (16.10.2026)";

#define MAXT 256
#define MAXARGS_PAT 8

static int o_all, o_long, o_wide;

static int byorder(const void *a, const void *b)
{
    const CuTask *x = a, *y = b;
    if (x->cli && y->cli) return x->cli - y->cli;
    if (x->cli != y->cli) return x->cli ? -1 : 1;   /* the Shells first */
    return stricmp(x->name, y->name);
}

static void hex(ULONG v)
{
    char b[11];
    int i;
    b[0] = '0'; b[1] = 'x';
    for (i = 0; i < 8; i++) b[2 + i] = "0123456789abcdef"[(v >> (28 - 4 * i)) & 15];
    b[10] = 0;
    cu_puts(b);
}

static void pad(const char *s, int w)
{
    int n = strlen(s);
    cu_puts(s);
    while (n++ < w) cu_putc(' ');
}

int main(void)
{
    static CuTask t[MAXT];
    char *v, parsed[MAXARGS_PAT][400];
    int c, i, j, n, npat = 0, shown = 0, cols = 80;
    (void)version;
    if (cu_args("ps [-elw] [name | pattern ...]") < 0) return cu_rc;
    while ((c = cu_opt("elwa", &v)) >= 0) switch (c) {
        case 'e': case 'a': o_all = 1; break;
        case 'l': o_long = 1; break;
        case 'w': o_wide = 1; break;
        default: return cu_rc;
    }
    for (i = 0; i < cu_argc && npat < MAXARGS_PAT; i++)
        if (!cu_pattern(cu_argv[i], parsed[npat++], sizeof parsed[0])) { cu_err(10, cu_argv[i], "bad pattern"); return cu_rc; }
    if (cu_argc) o_all = 1;                       /* a name looks among every task */
    {
        char b[16];
        if (GetVar("COLUMNS", b, sizeof b, 0) > 0 && atoi(b) > 20) cols = atoi(b);
        if (o_wide || !IsInteractive(Output())) cols = 1000;
    }
    n = cu_tasks(t, MAXT);
    qsort(t, n, sizeof *t, byorder);
    cu_puts(o_long ? "CLI ADDRESS    PRI STATE  STACK  FREE SIGWAIT    COMMAND\n"
                   : "CLI ADDRESS    PRI STATE  COMMAND\n");
    for (i = 0; i < n; i++) {
        CuTask *e = &t[i];
        char cmd[100];
        int left;
        if (!o_all && !e->cli) continue;
        if (e->cmd[0]) strcpy(cmd, e->cmd);
        else { strcpy(cmd, "["); strncat(cmd, e->name, sizeof cmd - 3); strcat(cmd, "]"); }
        if (npat) {
            int hit = 0;
            for (j = 0; j < npat && !hit; j++)
                hit = cu_match(parsed[j], e->name) || (e->cmd[0] && (cu_match(parsed[j], e->cmd) || cu_match(parsed[j], FilePart(e->cmd))));
            if (!hit) continue;
        }
        if (e->cli) cu_putnum(e->cli, 3); else cu_puts("  -");
        cu_putc(' ');
        hex((ULONG)e->addr);
        cu_putc(' ');
        cu_putnum(e->pri, 4);
        cu_putc(' ');
        pad(e->state == 0 ? "run" : e->state == 1 ? "ready" : "wait", 6);
        left = cols - 30;
        if (o_long) {
            cu_putnum(e->stack, 6);
            if (e->free >= 0) cu_putnum(e->free, 6); else cu_puts("     ?");
            cu_putc(' ');
            hex(e->sigwait);
            cu_putc(' ');
            left -= 25;
        }
        if (left < 10) left = 10;
        cu_put(cmd, (int)strlen(cmd) < left ? (int)strlen(cmd) : left);
        cu_putc('\n');
        shown++;
    }
    if (npat && !shown) cu_err(5, NULL, "nothing matches");
    cu_flush();
    return cu_rc;
}
