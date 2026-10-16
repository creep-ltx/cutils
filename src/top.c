/* top - the tasks that use the processor, refreshed every second.
 *
 *   top [-b] [-d SECONDS] [-n COUNT]
 *
 * The classic Unix top, inside the terminal (a CTerm tab or any console
 * window). At the top:
 *   top - 21:45:20 up 1:04, 8 shells        the time, the time since the
 *                                           start, the Shells (as uptime)
 *   Tasks: 42 total, 1 running, 2 ready, 39 waiting
 *   %Cpu: 12.0 busy, 88.0 idle              over the last interval (on a
 *                                           PiStorm with Emu68 also the
 *                                           68k instructions per second)
 *   Chip: ... total, used, free, largest    chip and fast memory, in K/M
 *   Fast: ... total, used, free, largest
 * then every task: CLI (the Shell's number), ADDRESS, PRI, STATE (run,
 * ready, wait), CPU% (its share of the processor over the interval),
 * TIME+ (its processor time since top started), STACK (its size), FREE
 * (the stack still free), COMMAND (what the Shell runs now, else [the
 * task's name], as ps shows it).
 *
 *   -b batch: plain text, no keys (also when the output is not a window:
 *      a file or a pipe)   -d SECONDS between refreshes (1; tenths too:
 *      -d 0.5)   -n COUNT refreshes, then end (batch: 1)
 *
 * Keys: P sort by CPU% (the default), M by stack size, N by name, T by
 * state; c the task's name instead of the command; e every task or only
 * the Shells; Up/Down move the bar, PgUp/PgDn/Home/End too; k break
 * (Ctrl-C) a task: a CLI number, a 0x address, or Return for the one
 * under the bar; space refresh now; d or s the delay; h help; q, Esc or
 * Ctrl-C quit.
 *
 * How CPU% is measured: the Amiga keeps no account of the time each task
 * uses, so top asks Exec to call it at every task switch (the tc_Launch /
 * tc_Switch hooks, below) and notes the EClock time: CPU% and TIME+ (the
 * time since top started) are exact. A task with hooks of its own, and
 * every task in a second top while the first holds the hooks, is sampled
 * instead: an interrupt server on the vertical blank (50 times a second)
 * notes the running task, and a task's share is its samples out of all
 * (its TIME+ is -). The sampler also paces the refreshes.
 *
 * Return code 0, 10 bad arguments, 20 Ctrl-C in batch mode, no memory,
 * no timer.device, or the terminal could not be opened. */
#include <stdlib.h>
#include <exec/execbase.h>
#include <exec/interrupts.h>
#include <exec/semaphores.h>
#include <hardware/intbits.h>
#include <devices/timer.h>
#include <libraries/configvars.h>
#include <proto/timer.h>
#include <proto/expansion.h>
#include "cu.h"
#include "tui.h"

const char *cu_name = "top";
static const char version[] = "$VER: top 0.1b3 (16.10.2026)";

struct Device *TimerBase;
struct ExpansionBase *ExpansionBase;

#define MAXT 256
#define RING 1024                /* samples kept: 20 s at 50 Hz */

/* ---- the sampler -----------------------------------------------------
 * The ring is all the interrupt server knows (is_Data, in a1): no
 * globals, no a4, no library calls. It is written in assembly so no
 * compiler choice can reach for a register it must not touch: a server
 * may use d0/d1/a0/a1/a5/a6 and must return with Z set (moveq #0,d0)
 * so the servers after it run. */
typedef struct {
    struct ExecBase *sys;        /* 0 */
    volatile ULONG count;        /* 4: samples ever taken */
    APTR slot[RING];             /* 8: the running task, NULL idle */
} Ring;

/* the offsets the assembly uses, checked by the compiler */
typedef char ck_this[offsetof(struct ExecBase, ThisTask) == 276 ? 1 : -1];
typedef char ck_state[offsetof(struct Task, tc_State) == 15 ? 1 : -1];
typedef char ck_slot[offsetof(Ring, slot) == 8 ? 1 : -1];

__asm__(
    "	.text\n"
    "	.even\n"
    "	.globl _top_vbsample\n"
    "_top_vbsample:\n"
    "	movea.l	(a1),a0\n"              /* SysBase */
    "	move.l	276(a0),d0\n"           /* ThisTask */
    "	beq.s	1f\n"
    "	movea.l	d0,a0\n"
    "	cmpi.b	#2,15(a0)\n"            /* tc_State == TS_RUN? */
    "	beq.s	1f\n"
    "	moveq	#0,d0\n"                /* not running: the CPU idles */
    "1:	move.l	4(a1),d1\n"
    "	move.l	d1,a0\n"
    "	andi.l	#1023,d1\n"
    "	lsl.l	#2,d1\n"
    "	move.l	d0,8(a1,d1.l)\n"
    "	addq.l	#1,a0\n"
    "	move.l	a0,4(a1)\n"
    "	moveq	#0,d0\n"
    "	rts\n");
extern void top_vbsample(void);

static Ring *ring;
static struct Interrupt *vbi;
static ULONG last;               /* ring->count at the last refresh */
static int hz = 50;

static int sampler_on(void)
{
    if (!(ring = AllocVec(sizeof *ring, MEMF_PUBLIC | MEMF_CLEAR))) return 0;
    if (!(vbi = AllocVec(sizeof *vbi, MEMF_PUBLIC | MEMF_CLEAR))) { FreeVec(ring); ring = NULL; return 0; }
    ring->sys = SysBase;
    vbi->is_Node.ln_Type = NT_INTERRUPT;
    vbi->is_Node.ln_Pri = -60;       /* after the system's own */
    vbi->is_Node.ln_Name = (char *)"top sampler";
    vbi->is_Data = ring;
    vbi->is_Code = top_vbsample;
    AddIntServer(INTB_VERTB, vbi);
    last = ring->count;
    if (SysBase->VBlankFrequency) hz = SysBase->VBlankFrequency;
    return 1;
}

static void sampler_off(void)
{
    if (vbi) { RemIntServer(INTB_VERTB, vbi); FreeVec(vbi); vbi = NULL; }
    if (ring) { FreeVec(ring); ring = NULL; }
}

/* ---- exact times: exec's own task hooks ---------------------------------
 * When TF_LAUNCH / TF_SWITCH are set in a task's tc_Flags, exec calls its
 * tc_Launch as the task gets the processor and tc_Switch as it loses it
 * (exec/tasks.h). top sets both on every task, with one pair of hooks
 * that note ThisTask and an EClock time into a ring: each task's time is
 * then exact, not sampled (Knowledge amiga/task-cpu-time.md: checked on
 * 3.1, 3.2.1 and Emu68). New tasks are hooked at every refresh; a task
 * that already had hooks of its own keeps them and is sampled.
 *
 * The hooks do not live in top. A task that ends while hooked leaves its
 * flags and our pointers in freed memory, and a task later made in that
 * memory without clearing them would call into a top long gone (dos
 * clears them for Run and NewShell; not every program that makes a task
 * may). So the hook code is copied once per boot into a block that is
 * never freed (152 bytes), found again by the name of the semaphore at
 * its head; with no top running, the hooks only return.
 * The semaphore is also the lock: one top at a time owns the hooks, a
 * second one samples. */
#define NEV 8192                 /* events in the ring, 16 bytes each */
#define KEEPNAME "CUtils top hooks 2"   /* a new layout: a new name */

typedef struct {
    struct Device *timer;        /* 0 */
    volatile ULONG count;        /* 4: events ever written */
    ULONG pad[6];
    struct { ULONG task, hi, lo, spare; } ev[NEV];   /* 32: task | 1 = off */
} Tap;
typedef char ck_ev[offsetof(Tap, ev) == 32 ? 1 : -1];

/* The stub copied into the block: the ring's address (NULL: no top),
 * then the hooks, position independent. They keep every register. */
__asm__(
    "	.text\n"
    "	.even\n"
    "	.globl _top_stub\n"
    "_top_stub:\n"
    "_top_tapptr:	.long 0\n"
    "_top_tapon:\n"                /* tc_Launch's hook: stub + 4 */
    "	movem.l	d0-d1/a0-a1/a6,-(sp)\n"
    "	moveq	#0,d1\n"
    "	bra.s	2f\n"
    "_top_tapoff:\n"
    "	movem.l	d0-d1/a0-a1/a6,-(sp)\n"
    "	moveq	#1,d1\n"
    "2:	move.l	_top_tapptr(pc),d0\n"
    "	beq.s	3f\n"                  /* no top: nothing to note */
    "	movea.l	d0,a1\n"
    "	move.l	4(a1),d0\n"
    "	andi.l	#8191,d0\n"
    "	lsl.l	#4,d0\n"
    "	lea	32(a1,d0.l),a0\n"
    "	movea.l	4.w,a6\n"
    "	move.l	276(a6),d0\n"          /* ThisTask */
    "	or.l	d1,d0\n"
    "	move.l	d0,(a0)+\n"
    "	move.l	a1,-(sp)\n"
    "	movea.l	(a1),a6\n"             /* TimerBase */
    "	jsr	-60(a6)\n"             /* ReadEClock(a0) */
    "	movea.l	(sp)+,a1\n"
    "	addq.l	#1,4(a1)\n"
    "3:	movem.l	(sp)+,d0-d1/a0-a1/a6\n"
    "	rts\n"
    "_top_stubend:\n"
    /* where the stub is, its size, where in it tc_Switch's hook is: asked
     * of code, because -resident32 reaches every C object through a4 */
    "	.globl _top_stubat\n"
    "_top_stubat:\n"
    "	lea	_top_stub(pc),a0\n"
    "	move.l	a0,d0\n"
    "	rts\n"
    "	.globl _top_stubsize\n"
    "_top_stubsize:\n"
    "	move.l	#_top_stubend-_top_stub,d0\n"
    "	rts\n"
    "	.globl _top_offat\n"
    "_top_offat:\n"
    "	move.l	#_top_tapoff-_top_stub,d0\n"
    "	rts\n");
extern char *top_stubat(void);
extern ULONG top_stubsize(void), top_offat(void);

typedef struct {
    struct SignalSemaphore sem;  /* named KEEPNAME, held by the top that hooks */
    char name[24];
    ULONG code[1];               /* the stub; code[0] is the ring's address */
} Keep;

#define NACC 512                 /* a hash of the tasks' times, 2 x MAXT */
typedef struct {
    APTR addr;                   /* NULL: free */
    unsigned long long iv;       /* EClock ticks in this interval */
    unsigned long long sum;      /* since top started: TIME+ */
} Acc;

static Tap *tap;
static Keep *keep;
static APTR onfn, offfn;         /* the hooks in the block */
static Acc acc[NACC], acc2[NACC];
static APTR foreign[MAXT];       /* tasks with hooks of their own */
static int nforeign, taplost;
static ULONG tapread, efreq;
static APTR cur;                 /* the task the last "on" was for */
static unsigned long long curstart, lastsnap;

static unsigned long long eclock(void)
{
    struct EClockVal ev;
    efreq = ReadEClock(&ev);
    return ((unsigned long long)ev.ev_hi << 32) | ev.ev_lo;
}

static Acc *accfor(Acc *h, APTR a, int add)
{
    int i = ((ULONG)a >> 4) & (NACC - 1), n;
    for (n = 0; n < NACC; n++, i = (i + 1) & (NACC - 1)) {
        if (h[i].addr == a) return &h[i];
        if (!h[i].addr) {
            if (!add) return NULL;
            h[i].addr = a;
            h[i].iv = h[i].sum = 0;
            return &h[i];
        }
    }
    return NULL;
}

static void hookone(struct Task *t)
{
    if ((APTR)t->tc_Launch == onfn) return;
    if (t->tc_Flags & (TF_SWITCH | TF_LAUNCH)) { if (nforeign < MAXT) foreign[nforeign++] = t; return; }
    t->tc_Switch = offfn;
    t->tc_Launch = onfn;
    t->tc_Flags |= TF_SWITCH | TF_LAUNCH;
}

static void unhookone(struct Task *t)
{
    if ((APTR)t->tc_Launch != onfn) return;
    t->tc_Flags &= ~(TF_SWITCH | TF_LAUNCH);
    t->tc_Switch = t->tc_Launch = NULL;
}

/* every task not hooked yet (ThisTask is in neither list); top itself
 * runs now, so it is "on" */
static void hookall(void)
{
    struct Node *n;
    nforeign = 0;
    Disable();
    for (n = SysBase->TaskReady.lh_Head; n->ln_Succ; n = n->ln_Succ) hookone((struct Task *)n);
    for (n = SysBase->TaskWait.lh_Head; n->ln_Succ; n = n->ln_Succ) hookone((struct Task *)n);
    hookone(SysBase->ThisTask);
    Enable();
}

/* the block, made once per boot; NULL: no memory */
static Keep *keepblock(void)
{
    ULONG size = top_stubsize();
    Keep *k;
    Forbid();
    if (!(k = (Keep *)FindSemaphore((STRPTR)KEEPNAME))
        && (k = AllocMem(sizeof *k + size, MEMF_PUBLIC | MEMF_CLEAR))) {
        strcpy(k->name, KEEPNAME);
        k->sem.ss_Link.ln_Name = k->name;
        k->sem.ss_Link.ln_Pri = -128;
        CopyMem(top_stubat(), k->code, size);
        k->code[0] = 0;
        CacheClearU();
        AddSemaphore(&k->sem);   /* never removed: tasks may point here */
    }
    Permit();
    return k;
}

static int tap_on(void)
{
    if (!(keep = keepblock())) return 0;
    if (!AttemptSemaphore(&keep->sem)) { keep = NULL; return 0; }   /* another top */
    if (!(tap = AllocVec(sizeof *tap, MEMF_PUBLIC | MEMF_CLEAR))) { ReleaseSemaphore(&keep->sem); keep = NULL; return 0; }
    onfn = (char *)keep->code + 4;
    offfn = (char *)keep->code + top_offat();
    tap->timer = TimerBase;
    lastsnap = curstart = eclock();
    cur = SysBase->ThisTask;
    tapread = 0;
    keep->code[0] = (ULONG)tap;
    CacheClearU();               /* read pc-relative: Emu68's JIT may keep it */
    hookall();
    return 1;
}

static void tap_off(void)
{
    struct Node *n;
    if (!tap) return;
    Disable();
    keep->code[0] = 0;           /* the hooks note nothing from now on */
    for (n = SysBase->TaskReady.lh_Head; n->ln_Succ; n = n->ln_Succ) unhookone((struct Task *)n);
    for (n = SysBase->TaskWait.lh_Head; n->ln_Succ; n = n->ln_Succ) unhookone((struct Task *)n);
    unhookone(SysBase->ThisTask);
    Enable();
    CacheClearU();
    /* no hook can be running now (exec calls them only while switching)
     * and none still has the old address: the ring can go */
    FreeVec(tap);
    tap = NULL;
    ReleaseSemaphore(&keep->sem);
    keep = NULL;
}

/* the events since the last call into the tasks' times */
static void tap_drain(void)
{
    ULONG n;
    if (!tap) return;
    n = tap->count;
    if (n - tapread > NEV) { taplost = 1; tapread = n - NEV; cur = NULL; }
    while (tapread != n) {
        ULONG *e = &tap->ev[tapread++ & (NEV - 1)].task;
        unsigned long long t = ((unsigned long long)e[1] << 32) | e[2];
        APTR tk = (APTR)(e[0] & ~1UL);
        if (!(e[0] & 1)) { cur = tk; curstart = t; }
        else {
            if (cur == tk && t >= curstart) {
                Acc *a = accfor(acc, tk, 1);
                if (a) { a->iv += t - curstart; a->sum += t - curstart; }
            }
            cur = NULL;
        }
    }
}

/* ---- Emu68 (PiStorm): 68k instructions per second ---------------------
 * Its counters are custom MOVEC registers, an illegal instruction on a
 * real 68k: only after Emu68's devicetree board is found. */
#define MOVEC_D0(rc, out) \
    __asm__ volatile(".short 0x4e7a, %c1\n\tmove.l %%d0,%0" : "=g"(out) : "i"(rc) : "d0")

static int emu68;
static unsigned long long pinsn, pcnt;
static ULONG mips = (ULONG)-1;

static void emu68_read(unsigned long long *insn, unsigned long long *cnt, ULONG *frq)
{
    APTR ssp = SuperState();
    ULONG hi, lo, hi2, f;
    MOVEC_D0(0xe0, f);
    do { MOVEC_D0(0xe2, hi); MOVEC_D0(0xe1, lo); MOVEC_D0(0xe2, hi2); } while (hi != hi2);
    *cnt = ((unsigned long long)hi << 32) | lo;
    do { MOVEC_D0(0xe4, hi); MOVEC_D0(0xe3, lo); MOVEC_D0(0xe4, hi2); } while (hi != hi2);
    *insn = ((unsigned long long)hi << 32) | lo;
    if (ssp) UserState(ssp);
    *frq = f;
}

static void emu68_probe(void)
{
    if ((ExpansionBase = (struct ExpansionBase *)OpenLibrary("expansion.library", 0))) {
        emu68 = FindConfigDev(NULL, 0x6d73, 0x21) != NULL;
        CloseLibrary((struct Library *)ExpansionBase);
        ExpansionBase = NULL;
    }
    if (emu68) { ULONG f; emu68_read(&pinsn, &pcnt, &f); }
}

static void emu68_measure(void)
{
    unsigned long long insn, cnt, dt;
    ULONG f;
    if (!emu68) return;
    emu68_read(&insn, &cnt, &f);
    dt = cnt - pcnt;
    if (dt && f) mips = (ULONG)((insn - pinsn) / 1000 * f / dt / 1000);   /* no overflow for hours */
    pinsn = insn;
    pcnt = cnt;
}

/* ---- a snapshot ------------------------------------------------------ */

typedef struct {
    CuTask *t;
    const char *name;            /* as shown: the command or [the task] */
    const char *key;             /* for sorting by name: without [ or a path */
    ULONG samples;
    int pct;                     /* tenths of a percent */
    int timed;                   /* time is exact (else CPU% is sampled) */
    unsigned long long time;     /* EClock ticks since top started */
} Row;

static CuTask tasks[MAXT];
static Row base[MAXT], rows[MAXT];   /* every task; those shown, sorted */
static int ntasks, nrows;
static ULONG total, idle;        /* samples in the interval */
static int exact = -1;           /* busy tenths of a percent, exact; -1 none */
static int o_name, o_shells, sortkey = 'P';
static char names[MAXT][76];

static const char *shown(CuTask *t, char *buf)
{
    int i, n = 0;
    const char *s = t->name;
    if (!o_name && t->cmd[0]) s = t->cmd;
    else buf[n++] = '[';
    for (i = 0; s[i] && n < 70; i++) {
        unsigned char c = s[i];
        buf[n++] = c < 32 || (c >= 0x7f && c < 0xa0) ? '?' : c;
    }
    if (buf[0] == '[') buf[n++] = ']';
    buf[n] = 0;
    return buf;
}

static int bysort(const void *a, const void *b)
{
    const Row *x = a, *y = b;
    int d = 0;
    switch (sortkey) {
    case 'M': d = (y->t->stack > x->t->stack) - (y->t->stack < x->t->stack); break;
    case 'T': d = x->t->state - y->t->state; if (!d) d = y->pct - x->pct; break;
    case 'N': break;
    default: d = y->pct - x->pct; if (!d) d = x->t->state - y->t->state; break;
    }
    if (d) return d;
    return stricmp(x->key, y->key);
}

/* the hooks' times since the last refresh into CPU% and TIME+ */
static void exact_times(void)
{
    unsigned long long now, wall, busy = 0;
    int i, k;
    tap_drain();
    now = eclock();
    if (cur) {                   /* running now: top itself */
        Acc *a = accfor(acc, cur, 1);
        if (a) { a->iv += now - curstart; a->sum += now - curstart; }
        curstart = now;
    }
    wall = now - lastsnap;
    lastsnap = now;
    for (i = 0; i < NACC; i++) if (acc[i].addr) busy += acc[i].iv;
    exact = wall ? (int)((busy * 1000 + wall / 2) / wall) : 0;
    if (exact > 1000) exact = 1000;
    memset(acc2, 0, sizeof acc2);
    for (k = 0; k < ntasks; k++) {
        Acc *a = accfor(acc, base[k].t->addr, 0), *b;
        for (i = 0; i < nforeign; i++) if (foreign[i] == base[k].t->addr) break;
        if (i < nforeign) continue;          /* its own hooks: sampled */
        base[k].timed = 1;
        if (!a) continue;
        base[k].time = a->sum;
        base[k].pct = wall ? (int)((a->iv * 1000 + wall / 2) / wall) : 0;
        if (base[k].pct > 1000) base[k].pct = 1000;
        if ((b = accfor(acc2, a->addr, 1))) b->sum = a->sum;   /* tasks gone are dropped */
    }
    memcpy(acc, acc2, sizeof acc);
    hookall();                   /* the tasks started since */
}

/* the tasks now, and the samples since the last refresh shared out */
static void snapshot(void)
{
    ULONG end, from, i;
    int k;
    ntasks = cu_tasks(tasks, MAXT);
    for (k = 0; k < ntasks; k++) { base[k].t = &tasks[k]; base[k].name = names[k]; base[k].samples = 0; }
    end = ring->count;
    from = end - last > RING ? end - RING : last;
    total = end - from;
    idle = 0;
    for (i = from; i != end; i++) {
        APTR a = ring->slot[i & (RING - 1)];
        if (!a) { idle++; continue; }
        for (k = 0; k < ntasks; k++) if (tasks[k].addr == a) { base[k].samples++; break; }
    }
    last = end;
    for (k = 0; k < ntasks; k++) {
        base[k].pct = total ? (int)((base[k].samples * 1000 + total / 2) / total) : 0;
        base[k].timed = 0;
        base[k].time = 0;
    }
    if (tap) exact_times();
    emu68_measure();
}

/* the rows to show, sorted */
static void arrange(void)
{
    int k;
    for (k = 0; k < ntasks; k++) {
        shown(&tasks[k], names[k]);
        base[k].key = names[k][0] == '[' ? names[k] + 1 : (const char *)FilePart((STRPTR)names[k]);
    }
    nrows = 0;
    for (k = 0; k < ntasks; k++) if (!o_shells || base[k].t->cli) rows[nrows++] = base[k];
    qsort(rows, nrows, sizeof *rows, bysort);
}

/* ---- text into a line ------------------------------------------------- */

static char L[600];
static int Ln;

static void ls(const char *s) { while (*s && Ln < (int)sizeof L - 1) L[Ln++] = *s++; L[Ln] = 0; }
static void lc(int c) { if (Ln < (int)sizeof L - 1) L[Ln++] = c; L[Ln] = 0; }
static void lnum(long v, int w)
{
    char b[16];
    int n = 0, neg = v < 0;
    unsigned long u = neg ? -v : v;
    do b[n++] = '0' + u % 10; while ((u /= 10));
    if (neg) b[n++] = '-';
    while (n < w--) lc(' ');
    while (n) lc(b[--n]);
}
static void lpct(int t, int w) { lnum(t / 10, w - 2); lc('.'); lc('0' + t % 10); }
static void lpad(const char *s, int w) { int n = strlen(s); ls(s); while (n++ < w) lc(' '); }
static void lhex(ULONG v) { int i; for (i = 0; i < 8; i++) lc("0123456789abcdef"[(v >> (28 - 4 * i)) & 15]); }
static void ltwo(ULONG v) { lc('0' + v / 10 % 10); lc('0' + v % 10); }
/* bytes as K or M (G), one decimal under 10 */
static void lmem(ULONG v)
{
    const char *u = "KMG";
    int k = 0;
    ULONG whole = v >> 10, tenth = (v & 1023) * 10 / 1024;
    while (whole >= 1024 && k < 2) { tenth = (whole % 1024) * 10 / 1024; whole /= 1024; k++; }
    if (k && whole < 10) { lnum(whole, 0); lc('.'); lc('0' + tenth); }
    else lnum(whole, 0);
    lc(u[k]);
}

/* ---- the header -------------------------------------------------------- */

static void line_top(void)
{
    struct EClockVal ev;
    struct DateStamp now;
    ULONG freq, secs, days;
    unsigned long long t;
    int k, shells = 0;
    freq = ReadEClock(&ev);
    t = ((unsigned long long)ev.ev_hi << 32) | ev.ev_lo;
    secs = freq ? (ULONG)(t / freq) : 0;
    days = secs / 86400;
    DateStamp(&now);
    for (k = 0; k < ntasks; k++) if (tasks[k].cli) shells++;
    Ln = 0;
    ls("top - ");
    ltwo(now.ds_Minute / 60); lc(':'); ltwo(now.ds_Minute % 60); lc(':'); ltwo(now.ds_Tick / TICKS_PER_SECOND);
    ls(" up ");
    if (days) { lnum(days, 0); ls(days == 1 ? " day, " : " days, "); }
    lnum(secs / 3600 % 24, 0); lc(':'); ltwo(secs / 60 % 60);
    ls(", "); lnum(shells, 0); ls(shells == 1 ? " shell" : " shells");
}

static void line_tasks(void)
{
    int k, st[3] = { 0, 0, 0 };
    for (k = 0; k < ntasks; k++) st[tasks[k].state]++;
    Ln = 0;
    ls("Tasks: "); lnum(ntasks, 0); ls(" total, ");
    lnum(st[0], 0); ls(" running, ");
    lnum(st[1], 0); ls(" ready, ");
    lnum(st[2], 0); ls(" waiting");
}

static void line_cpu(void)
{
    Ln = 0;
    ls("%Cpu: ");
    if (exact >= 0) { lpct(exact, 0); ls(" busy, "); lpct(1000 - exact, 0); ls(" idle"); }
    else if (!total) ls("measuring");
    else {
        int busy = (int)(((total - idle) * 1000 + total / 2) / total);
        lpct(busy, 0); ls(" busy, "); lpct(1000 - busy, 0); ls(" idle");
    }
    if (emu68 && mips != (ULONG)-1) { ls(", "); lnum(mips, 0); ls(" MIPS"); }
}

static void line_mem(int fast)
{
    ULONG type = fast ? MEMF_FAST : MEMF_CHIP;
    ULONG tot = AvailMem(type | MEMF_TOTAL), av = AvailMem(type), big = AvailMem(type | MEMF_LARGEST);
    Ln = 0;
    ls(fast ? "Fast: " : "Chip: ");
    lmem(tot); ls(" total, ");
    lmem(tot - av); ls(" used, ");
    lmem(av); ls(" free, ");
    lmem(big); ls(" largest");
}

static void line_titles(void)
{
    Ln = 0;
    ls("CLI "); lpad("ADDRESS", 8); ls("  PRI "); lpad("STATE", 5);
    ls("  CPU%    TIME+  STACK   FREE COMMAND");
}

/* TIME+: minutes:seconds.hundredths, as Unix top (over 99 minutes no
 * hundredths); - when the task is sampled */
static void ltime(Row *r)
{
    ULONG h, m;
    if (!r->timed || !efreq) { ls("       -"); return; }
    h = (ULONG)(r->time * 100 / efreq);
    m = h / 6000;
    if (m < 100) { lnum(m, 2); lc(':'); ltwo(h / 100 % 60); lc('.'); ltwo(h % 100); }
    else { lnum(m, 5); lc(':'); ltwo(h / 100 % 60); }
}

static void line_row(Row *r)
{
    CuTask *t = r->t;
    Ln = 0;
    if (t->cli) lnum(t->cli, 3); else ls("  -");
    lc(' ');
    lhex((ULONG)t->addr);
    lc(' ');
    lnum(t->pri, 4);
    lc(' ');
    lpad(t->state == 0 ? "run" : t->state == 1 ? "ready" : "wait", 5);
    lc(' ');
    lpct(r->pct, 5);
    lc(' ');
    ltime(r);
    lc(' ');
    lnum(t->stack, 6);
    lc(' ');
    if (t->free >= 0) lnum(t->free, 6); else ls("     ?");
    lc(' ');
    ls(r->name);
}

#define HEADER 5                 /* lines before the message line */

static void header(int i)
{
    switch (i) {
    case 0: line_top(); break;
    case 1: line_tasks(); break;
    case 2: line_cpu(); break;
    case 3: line_mem(0); break;
    default: line_mem(1); break;
    }
}

static int hasfast(void) { return AvailMem(MEMF_FAST | MEMF_TOTAL) != 0; }

/* ---- batch mode ------------------------------------------------------- */

/* until the ring has counted to target; 0: Ctrl-C */
static int sleepto(ULONG target)
{
    while ((LONG)(target - ring->count) > 0) {
        if (cu_break()) return 0;
        Delay(2);
        tap_drain();
    }
    return 1;
}

static int batch(int count, int delay10, int cols)
{
    int i, k;
    for (i = 0; i < count; i++) {
        if (!sleepto(last + (ULONG)delay10 * hz / 10)) return 0;
        snapshot();
        arrange();
        if (i) cu_putc('\n');
        for (k = 0; k < HEADER; k++) {
            if (k == 4 && !hasfast()) continue;
            header(k);
            cu_put(L, Ln < cols ? Ln : cols); cu_putc('\n');
        }
        cu_putc('\n');
        line_titles(); cu_put(L, Ln < cols ? Ln : cols); cu_putc('\n');
        for (k = 0; k < nrows; k++) { line_row(&rows[k]); cu_put(L, Ln < cols ? Ln : cols); cu_putc('\n'); }
        cu_flush();
    }
    return 1;
}

/* ---- the interactive view ------------------------------------------- */

static int sel, first;           /* the bar's row, the first row shown */
static APTR seladdr;
static char msg[160];

/* L on a row, the rest of the row blank */
static void put(int row, int attr)
{
    int n = Ln < tui_cols ? Ln : tui_cols;
    tui_put(0, row, L, n, TC_DEFAULT, TC_DEFAULT, attr);
    tui_fill(n, row, tui_cols - n, ' ', TC_DEFAULT, TC_DEFAULT, attr);
}

static int listrows(void) { int n = tui_rows - (HEADER + 2); return n < 1 ? 1 : n; }

static void keepsel(void)
{
    int k, page = listrows();
    if (seladdr) for (k = 0; k < nrows; k++) if (rows[k].t->addr == seladdr) { sel = k; break; }
    if (sel >= nrows) sel = nrows - 1;
    if (sel < 0) sel = 0;
    if (sel < first) first = sel;
    if (sel >= first + page) first = sel - page + 1;
    if (first > nrows - page) first = nrows - page;
    if (first < 0) first = 0;
    seladdr = nrows ? rows[sel].t->addr : NULL;
}

static void draw(const char *prompt)
{
    int k, r;
    for (k = 0; k < HEADER && k < tui_rows; k++) {
        int lab = 0;
        header(k);
        if (k == 4 && !hasfast()) Ln = 0;
        put(k, 0);
        while (lab < Ln && L[lab] != ' ' && lab < 7) lab++;   /* the label bold */
        if (k == 0) lab = 3;
        tui_put(0, k, L, lab < tui_cols ? lab : tui_cols, TC_DEFAULT, TC_DEFAULT, TA_BOLD);
    }
    if (HEADER < tui_rows) {
        Ln = 0;
        ls(prompt ? prompt : msg);
        put(HEADER, prompt ? TA_BOLD : 0);
        if (prompt) tui_cursor(Ln < tui_cols ? Ln : tui_cols - 1, HEADER, 1);
        else tui_cursor(0, tui_rows - 1, 0);
    }
    if (HEADER + 1 < tui_rows) { line_titles(); put(HEADER + 1, TA_REV); }
    for (r = 0; HEADER + 2 + r < tui_rows; r++) {
        int at = 0, fg = TC_DEFAULT, bg = TC_DEFAULT, n;
        Ln = 0;
        if (first + r < nrows) {
            Row *w = &rows[first + r];
            line_row(w);
            if (w->t->state < 2) at = TA_BOLD;         /* running or ready */
            if (first + r == sel) {
                if (tui_cterm) { fg = 15; bg = 4; }     /* the bar: white on blue */
                else at |= TA_REV;
            }
        }
        n = Ln < tui_cols ? Ln : tui_cols;
        tui_put(0, HEADER + 2 + r, L, n, fg, bg, at);
        tui_fill(n, HEADER + 2 + r, tui_cols - n, ' ', fg, bg, at);
    }
    tui_flush();
}

static const char *const helptext[] = {
    "top 0.1b3 - the tasks that use the processor",
    "",
    "  P          sort by CPU% (the default)",
    "  M          sort by stack size",
    "  N          sort by name",
    "  T          sort by state",
    "  c          task name / command",
    "  e          every task / only the Shells",
    "  Up Down    move the bar (Shift: a page; PgUp PgDn Home End)",
    "  k          break (Ctrl-C) a task",
    "  space      refresh now",
    "  d s        the delay between refreshes",
    "  h          this help",
    "  q Esc      quit (Ctrl-C too)",
    "",
    "CPU% and TIME+ are exact: exec tells top each time a task",
    "gets and loses the processor. TIME+ counts from top's start.",
    "",
    "Any key goes back.",
    NULL
};

static int help(void)
{
    TuiEv e;
    int i;
    for (;;) {
        tui_clear();
        for (i = 0; helptext[i] && i < tui_rows; i++)
            tui_put(1, i, helptext[i], strlen(helptext[i]), TC_DEFAULT, TC_DEFAULT, i == 0 ? TA_BOLD : 0);
        tui_cursor(0, tui_rows - 1, 0);
        tui_flush();
        if (!tui_wait(&e, -1, 0, NULL)) continue;
        if (e.type == TE_RESIZE) { tui_resized(); continue; }
        if (e.type == TE_KEY) return e.key != (TM_CTRL | 'c');
    }
}

/* a line typed on the message row; 0: Esc */
static int ask(const char *prompt, char *buf, int size)
{
    TuiEv e;
    int n = 0;
    char st[200];
    buf[0] = 0;
    for (;;) {
        strcpy(st, prompt);
        strcat(st, buf);
        draw(st);
        if (!tui_wait(&e, -1, 0, NULL)) continue;
        if (e.type == TE_RESIZE) { tui_resized(); keepsel(); continue; }
        if (e.type != TE_KEY) continue;
        if (e.key == K_ENTER) return 1;
        if (e.key == K_ESC || e.key == (TM_CTRL | 'c')) return 0;
        if (e.key == K_BS || e.key == K_DEL) { if (n) buf[--n] = 0; continue; }
        if (e.key >= 32 && e.key < 127 && n < size - 1) { buf[n++] = e.key; buf[n] = 0; }
    }
}

static void killask(void)
{
    char b[24], p[140], nm[80];
    CuTask *t = NULL;
    int k;
    if (!nrows) return;
    strcpy(p, "Break which task? CLI number, 0x address, Return: ");
    strcat(p, shown(rows[sel].t, nm));
    strcat(p, " > ");
    if (!ask(p, b, sizeof b)) { msg[0] = 0; return; }
    if (!b[0]) t = rows[sel].t;
    else if (b[0] == '0' && (b[1] == 'x' || b[1] == 'X')) {
        ULONG a = 0;
        for (k = 2; b[k]; k++) {
            int c = b[k], v = c >= '0' && c <= '9' ? c - '0' : (c | 32) >= 'a' && (c | 32) <= 'f' ? (c | 32) - 'a' + 10 : -1;
            if (v < 0) break;
            a = a << 4 | v;
        }
        for (k = 0; k < ntasks; k++) if ((ULONG)tasks[k].addr == a) t = &tasks[k];
    } else {
        int ok;
        long n = cu_num(b, &ok);
        if (ok) for (k = 0; k < ntasks; k++) if (tasks[k].cli == n) t = &tasks[k];
    }
    if (!t) { strcpy(msg, "no such task"); return; }
    if (cu_signal(t->addr, SIGBREAKF_CTRL_C)) { strcpy(msg, "Ctrl-C sent to "); strcat(msg, shown(t, nm)); }
    else strcpy(msg, "that task is gone");
}

/* tenths of seconds from "1", "0.5", "2.5"; 0: not a delay */
static int tenths(const char *s)
{
    int v = 0, d = 0, frac = 0;
    for (; *s; s++) {
        if (*s >= '0' && *s <= '9') {
            if (frac) { if (frac++ == 1) d = *s - '0'; }
            else if ((v = v * 10 + *s - '0') > 3600) return 0;
        } else if ((*s == '.' || *s == ',') && !frac) frac = 1;
        else return 0;
    }
    v = v * 10 + d;
    return v;
}

static void delayask(int *delay10)
{
    char b[16], p[60];
    int d;
    Ln = 0; ls("Delay in seconds (now "); lpct(*delay10 * 10, 0); Ln -= 2; L[Ln] = 0; ls(") > ");
    strcpy(p, L);
    if (!ask(p, b, sizeof b) || !b[0]) { msg[0] = 0; return; }
    if ((d = tenths(b)) < 1) { strcpy(msg, "not a delay: "); strcat(msg, b); return; }
    *delay10 = d;
    msg[0] = 0;
}

static int interactive(int count, int delay10)
{
    TuiEv e;
    ULONG next;
    int refreshes = 0;
    tui_title("top");
    snapshot();
    arrange();
    keepsel();
    draw(NULL);
    next = last + (ULONG)delay10 * hz / 10;
    for (;;) {
        LONG left = (LONG)(next - ring->count);
        if (left <= 0) {
            snapshot();
            arrange();
            keepsel();
            draw(NULL);
            if (count && ++refreshes >= count) return 1;
            next = last + (ULONG)delay10 * hz / 10;
            continue;
        }
        if (left > hz / 2) left = hz / 2;    /* the hooks' ring read twice a second */
        tap_drain();
        if (!tui_wait(&e, left * (1000000 / hz), 0, NULL)) continue;
        if (e.type == TE_RESIZE) { tui_resized(); keepsel(); draw(NULL); continue; }
        if (e.type == TE_MOUSE) {
            if (e.mact == TB_WHEELUP) sel -= 3;
            else if (e.mact == TB_WHEELDOWN) sel += 3;
            else if (e.mact == TB_PRESS && e.row >= HEADER + 2 && first + e.row - HEADER - 2 < nrows) sel = first + e.row - HEADER - 2;
            else continue;
            seladdr = NULL;
            keepsel();
            draw(NULL);
            continue;
        }
        if (e.type != TE_KEY) continue;
        {
            int k = e.key, page = listrows(), moved = 0, again = 0;
            msg[0] = 0;
            if (k == 'q' || k == 'Q' || k == K_ESC || k == (TM_CTRL | 'c')) return 1;
            else if (k == 'P' || k == 'p') sortkey = 'P';
            else if (k == 'M' || k == 'm') sortkey = 'M';
            else if (k == 'N' || k == 'n') sortkey = 'N';
            else if (k == 'T' || k == 't') sortkey = 'T';
            else if (k == 'c' || k == 'C') o_name = !o_name;
            else if (k == 'e' || k == 'E') { o_shells = !o_shells; strcpy(msg, o_shells ? "only the Shells" : "every task"); }
            else if (k == K_UP) { sel--; moved = 1; }
            else if (k == K_DOWN) { sel++; moved = 1; }
            else if (k == K_PGUP || k == (TM_SHIFT | K_UP)) { sel -= page; moved = 1; }
            else if (k == K_PGDN || k == (TM_SHIFT | K_DOWN)) { sel += page; moved = 1; }
            else if (k == K_HOME) { sel = 0; moved = 1; }
            else if (k == K_END) { sel = nrows - 1; moved = 1; }
            else if (k == 'k' || k == 'K') killask();
            else if (k == ' ') again = 1;
            else if (k == 'd' || k == 's') { delayask(&delay10); again = 1; }
            else if (k == 'h' || k == 'H' || k == '?' || k == K_HELP) { if (!help()) return 1; }
            else continue;
            if (moved) seladdr = NULL;
            if (again) {
                snapshot();
                next = last + (ULONG)delay10 * hz / 10;
            }
            /* a new order or filter: the same samples, arranged again */
            arrange();
            keepsel();
            draw(NULL);
        }
    }
}

int main(void)
{
    struct timerequest *tr = NULL;
    char *v;
    int c, o_batch = 0, count = 0, delay10 = 10, cols = 1000;
    (void)version;
    if (cu_args("top [-b] [-d SECONDS] [-n COUNT]") < 0) return cu_rc;
    while ((c = cu_opt("bd:n:", &v)) >= 0) switch (c) {
        case 'b': o_batch = 1; break;
        case 'd': if ((delay10 = tenths(v)) < 1) { cu_err(10, v, "not a delay in seconds"); return cu_rc; } break;
        case 'n': { int ok; long n = cu_num(v, &ok); if (!ok || n < 1) { cu_err(10, v, "not a count"); return cu_rc; } count = n; } break;
        default: return cu_rc;
    }
    if (cu_argc) { cu_err(10, cu_argv[0], "unexpected argument"); return cu_rc; }
    if (!o_batch && !tui_possible()) o_batch = 1;
    if (o_batch) {
        char b[16];
        if (!count) count = 1;
        if (IsInteractive(Output())) {
            cols = 80;
            if (GetVar("COLUMNS", b, sizeof b, 0) > 0 && atoi(b) > 20) cols = atoi(b);
        }
    }
    if (!(tr = AllocVec(sizeof *tr, MEMF_PUBLIC | MEMF_CLEAR))) { cu_err(20, NULL, "no memory"); return cu_rc; }
    if (OpenDevice(TIMERNAME, UNIT_ECLOCK, (struct IORequest *)tr, 0)) { FreeVec(tr); cu_err(20, NULL, "no timer.device"); return cu_rc; }
    TimerBase = tr->tr_node.io_Device;
    if (!sampler_on()) cu_err(20, NULL, "no memory");
    else {
        tap_on();                /* else everything is sampled */
        emu68_probe();
        if (o_batch) batch(count, delay10, cols);
        else if (!tui_open()) cu_err(20, NULL, "can not open the terminal");
        else {
            interactive(count, delay10);
            tui_close();
        }
        tap_off();
        sampler_off();
    }
    CloseDevice((struct IORequest *)tr);
    FreeVec(tr);
    cu_flush();
    return cu_rc;
}
