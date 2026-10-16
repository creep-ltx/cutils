/* cu_cpu.c - how busy the processor is, and each task's share of it (top,
 * ctop). Grew out of top 0.1b3 (16.10.2026), moved unchanged.
 *
 * Three parts:
 *  - a vertical-blank interrupt server that notes the running task 50
 *    times a second: the pace of the refreshes, and the fallback;
 *  - exec's own task hooks (tc_Launch / tc_Switch) on every task, with an
 *    EClock time at every switch: exact CPU% and time (below);
 *  - Emu68's counters on a PiStorm: 68k instructions per second.
 * The caller opens timer.device and sets TimerBase first (ReadEClock). */
#include <stdlib.h>
#include <string.h>
#include <exec/execbase.h>
#include <exec/interrupts.h>
#include <exec/semaphores.h>
#include <hardware/intbits.h>
#include <devices/timer.h>
#include <libraries/configvars.h>
#include <proto/exec.h>
#include <proto/timer.h>
#include <proto/expansion.h>
#include "cu.h"

extern struct Device *TimerBase;
struct ExpansionBase *ExpansionBase;

#define MAXT CU_MAXTASKS
#define RING 1024                /* samples kept: 20 s at 50 Hz */

/* ---- the sampler: the vertical blank notes ThisTask ---------------------
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
    "	.globl _cu_vbsample\n"
    "_cu_vbsample:\n"
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
extern void cu_vbsample(void);

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
    vbi->is_Code = cu_vbsample;
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
 * (exec/tasks.h). We set both on every task, with one pair of hooks
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
    "	.globl _cu_stub\n"
    "_cu_stub:\n"
    "_cu_tapptr:	.long 0\n"
    "_cu_tapon:\n"                /* tc_Launch's hook: stub + 4 */
    "	movem.l	d0-d1/a0-a1/a6,-(sp)\n"
    "	moveq	#0,d1\n"
    "	bra.s	2f\n"
    "_cu_tapoff:\n"
    "	movem.l	d0-d1/a0-a1/a6,-(sp)\n"
    "	moveq	#1,d1\n"
    "2:	move.l	_cu_tapptr(pc),d0\n"
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
    "_cu_stubend:\n"
    /* where the stub is, its size, where in it tc_Switch's hook is: asked
     * of code, because -resident32 reaches every C object through a4 */
    "	.globl _cu_stubat\n"
    "_cu_stubat:\n"
    "	lea	_cu_stub(pc),a0\n"
    "	move.l	a0,d0\n"
    "	rts\n"
    "	.globl _cu_stubsize\n"
    "_cu_stubsize:\n"
    "	move.l	#_cu_stubend-_cu_stub,d0\n"
    "	rts\n"
    "	.globl _cu_offat\n"
    "_cu_offat:\n"
    "	move.l	#_cu_tapoff-_cu_stub,d0\n"
    "	rts\n");
extern char *cu_stubat(void);
extern ULONG cu_stubsize(void), cu_offat(void);

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
    ULONG size = cu_stubsize();
    Keep *k;
    Forbid();
    if (!(k = (Keep *)FindSemaphore((STRPTR)KEEPNAME))
        && (k = AllocMem(sizeof *k + size, MEMF_PUBLIC | MEMF_CLEAR))) {
        strcpy(k->name, KEEPNAME);
        k->sem.ss_Link.ln_Name = k->name;
        k->sem.ss_Link.ln_Pri = -128;
        CopyMem(cu_stubat(), k->code, size);
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
    offfn = (char *)keep->code + cu_offat();
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

/* ---- for the programs ---------------------------------------------------- */

int cu_cpuopen(void)
{
    if (!sampler_on()) return 0;
    tap_on();                    /* else everything is sampled */
    emu68_probe();
    return 1;
}

void cu_cpuclose(void)
{
    tap_off();
    sampler_off();
}

void cu_cpudrain(void) { tap_drain(); }
ULONG cu_cpuclock(void) { return ring ? ring->count : 0; }
int cu_cpurate(void) { return hz; }
int cu_cpuexact(void) { return tap != NULL; }
ULONG cu_cpufreq(void) { return efreq; }
int cu_emu68(void) { return emu68; }
ULONG cu_emu68mips(void) { emu68_measure(); return mips; }

/* the hooks' times since the last call into s[]; the busy share */
static int exact_times(const CuTask *t, int n, CuShare *s)
{
    unsigned long long now, wall, busy = 0;
    int i, k, ex;
    tap_drain();
    now = eclock();
    if (cur) {                   /* running now: the caller itself */
        Acc *a = accfor(acc, cur, 1);
        if (a) { a->iv += now - curstart; a->sum += now - curstart; }
        curstart = now;
    }
    wall = now - lastsnap;
    lastsnap = now;
    for (i = 0; i < NACC; i++) if (acc[i].addr) busy += acc[i].iv;
    ex = wall ? (int)((busy * 1000 + wall / 2) / wall) : 0;
    if (ex > 1000) ex = 1000;
    memset(acc2, 0, sizeof acc2);
    for (k = 0; k < n; k++) {
        Acc *a = accfor(acc, t[k].addr, 0), *b;
        for (i = 0; i < nforeign; i++) if (foreign[i] == t[k].addr) break;
        if (i < nforeign) continue;          /* its own hooks: sampled */
        s[k].timed = 1;
        if (!a) continue;
        s[k].time = a->sum;
        s[k].pct = wall ? (int)((a->iv * 1000 + wall / 2) / wall) : 0;
        if (s[k].pct > 1000) s[k].pct = 1000;
        if ((b = accfor(acc2, a->addr, 1))) b->sum = a->sum;   /* tasks gone are dropped */
    }
    memcpy(acc, acc2, sizeof acc);
    hookall();                   /* the tasks started since */
    return ex;
}

int cu_cpumeasure(const CuTask *t, int n, CuShare *s)
{
    ULONG end, from, i, total, idle = 0;
    int k;
    static ULONG cnt[MAXT];
    if (!ring) return -1;
    for (k = 0; k < n; k++) cnt[k] = 0;
    end = ring->count;
    from = end - last > RING ? end - RING : last;
    total = end - from;
    for (i = from; i != end; i++) {
        APTR a = ring->slot[i & (RING - 1)];
        if (!a) { idle++; continue; }
        for (k = 0; k < n; k++) if (t[k].addr == a) { cnt[k]++; break; }
    }
    last = end;
    for (k = 0; k < n; k++) {
        s[k].pct = total ? (int)((cnt[k] * 1000 + total / 2) / total) : 0;
        s[k].timed = 0;
        s[k].time = 0;
    }
    if (tap) return exact_times(t, n, s);
    return total ? (int)(((total - idle) * 1000 + total / 2) / total) : -1;
}
