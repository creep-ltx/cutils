/* exectap - an experiment for top: can exec tell us exactly when each task
 * runs, instead of top's 50-a-second sampling?
 *
 *   exectap [SECONDS=n] [NOLVO] [NOHOOK]
 *
 * For SECONDS (5) it listens in three ways at once and then compares:
 *  1. tc_Launch / tc_Switch: exec calls a task's tc_Launch when the task
 *     gets the processor and tc_Switch when it loses it, if TF_LAUNCH /
 *     TF_SWITCH are set in its tc_Flags (exec/tasks.h, documented since
 *     1.0). exectap sets them on every task that exists now (skipping
 *     any that already use them), and each call writes the task and an
 *     EClock time into a ring. That gives each task's exact time.
 *  2. The private exec vectors Switch (-54) and Dispatch (-60), patched
 *     only to count calls: does this ROM still go through them? (NOLVO
 *     leaves them alone.)
 *  3. top's own method: a vertical blank server that notes ThisTask.
 * Everything is put back on exit, also on Ctrl-C. Not a CUtils command:
 * a probe whose answer decides how top measures. */
#include <exec/execbase.h>
#include <exec/interrupts.h>
#include <exec/tasks.h>
#include <hardware/intbits.h>
#include <devices/timer.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/timer.h>
#include <string.h>
#include <stdio.h>

const char version[] = "$VER: exectap 0.1 (16.10.2026)";

struct Device *TimerBase;

#define NEV  16384               /* events in the ring (16 bytes each) */
#define NVB  1024

typedef struct {
    struct Device *timer;        /* 0 */
    volatile ULONG count;        /* 4: events ever written */
    volatile ULONG lvo_switch;   /* 8 */
    volatile ULONG lvo_dispatch; /* 12 */
    APTR old_switch;             /* 16 */
    APTR old_dispatch;           /* 20 */
    ULONG pad[2];                /* 24 */
    struct { ULONG task, hi, lo, spare; } ev[NEV];   /* 32: task | 1 = off */
} Tap;

typedef struct {
    struct ExecBase *sys;        /* 0 */
    volatile ULONG count;        /* 4 */
    APTR slot[NVB];              /* 8 */
} Ring;

typedef char ck_this[offsetof(struct ExecBase, ThisTask) == 276 ? 1 : -1];
typedef char ck_ev[offsetof(Tap, ev) == 32 ? 1 : -1];

/* The hooks find the Tap through tap_ptr, a long beside the code, set
 * once before any hook is installed (then the caches are cleared). They
 * keep every register: exec's state at that moment is not documented. */
__asm__(
    "	.text\n"
    "	.even\n"
    "	.globl _tap_ptr\n"
    "_tap_ptr:	.long 0\n"
    "	.globl _tap_on\n"
    "_tap_on:\n"
    "	movem.l	d0-d1/a0-a1/a6,-(sp)\n"
    "	moveq	#0,d1\n"
    "	bra.s	2f\n"
    "	.globl _tap_off\n"
    "_tap_off:\n"
    "	movem.l	d0-d1/a0-a1/a6,-(sp)\n"
    "	moveq	#1,d1\n"
    "2:	movea.l	_tap_ptr(pc),a1\n"
    "	move.l	4(a1),d0\n"
    "	andi.l	#16383,d0\n"
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
    "	movem.l	(sp)+,d0-d1/a0-a1/a6\n"
    "	rts\n"
    /* the vector patches: count, then go on to the old code with every
     * register as it came (the old address is pushed and rts'd to) */
    "	.globl _tap_sw\n"
    "_tap_sw:\n"
    "	subq.l	#4,sp\n"
    "	move.l	a0,-(sp)\n"
    "	movea.l	_tap_ptr(pc),a0\n"
    "	addq.l	#1,8(a0)\n"
    "	move.l	16(a0),4(sp)\n"
    "	movea.l	(sp)+,a0\n"
    "	rts\n"
    "	.globl _tap_di\n"
    "_tap_di:\n"
    "	subq.l	#4,sp\n"
    "	move.l	a0,-(sp)\n"
    "	movea.l	_tap_ptr(pc),a0\n"
    "	addq.l	#1,12(a0)\n"
    "	move.l	20(a0),4(sp)\n"
    "	movea.l	(sp)+,a0\n"
    "	rts\n"
    /* top's sampler, as in src/top.c */
    "	.globl _tap_vb\n"
    "_tap_vb:\n"
    "	movea.l	(a1),a0\n"
    "	move.l	276(a0),d0\n"
    "	beq.s	1f\n"
    "	movea.l	d0,a0\n"
    "	cmpi.b	#2,15(a0)\n"
    "	beq.s	1f\n"
    "	moveq	#0,d0\n"
    "1:	move.l	4(a1),d1\n"
    "	move.l	d1,a0\n"
    "	andi.l	#1023,d1\n"
    "	lsl.l	#2,d1\n"
    "	move.l	d0,8(a1,d1.l)\n"
    "	addq.l	#1,a0\n"
    "	move.l	a0,4(a1)\n"
    "	moveq	#0,d0\n"
    "	rts\n");
extern ULONG tap_ptr;
extern void tap_on(void), tap_off(void), tap_sw(void), tap_di(void), tap_vb(void);

/* ---- what we learn ------------------------------------------------- */

#define MAXT 256
typedef struct {
    struct Task *t;
    char name[40];
    int hooked;
    unsigned long long time;     /* EClock ticks it ran */
    ULONG launches, samples;
} St;

static St st[MAXT];
static int nst;
static Tap *tap;
static Ring *ring;
static ULONG drained, vbdrained, lost, vblost;
static ULONG on_without_off, off_mismatch, freq;
static struct Task *cur;
static unsigned long long start, lastoff, first, lastt, gap;
static int seen;
static ULONG vbtotal, vbidle;

static St *find(struct Task *t, int add)
{
    int i;
    for (i = 0; i < nst; i++) if (st[i].t == t) return &st[i];
    if (!add || nst == MAXT) return NULL;
    st[nst].t = t;
    strcpy(st[nst].name, "?");
    return &st[nst++];
}

static void drain(void)
{
    ULONG n = tap->count, vn = ring->count;
    if (n - drained > NEV) { lost += n - drained - NEV; drained = n - NEV; }
    while (drained != n) {
        ULONG *e = &tap->ev[drained & (NEV - 1)].task;
        unsigned long long t = ((unsigned long long)e[1] << 32) | e[2];
        struct Task *tk = (struct Task *)(e[0] & ~1UL);
        St *s;
        drained++;
        if (!seen) { first = t; seen = 1; }
        lastt = t;
        if (!(e[0] & 1)) {
            if (cur) on_without_off++;
            else if (lastoff) gap += t - lastoff;
            cur = tk;
            start = t;
            if ((s = find(tk, 1))) s->launches++;
        } else {
            if (cur == tk) { if ((s = find(tk, 1))) s->time += t - start; }
            else off_mismatch++;
            cur = NULL;
            lastoff = t;
        }
    }
    if (vn - vbdrained > NVB) { vblost += vn - vbdrained - NVB; vbdrained = vn - NVB; }
    while (vbdrained != vn) {
        struct Task *tk = ring->slot[vbdrained++ & (NVB - 1)];
        St *s;
        vbtotal++;
        if (!tk) vbidle++;
        else if ((s = find(tk, 1))) s->samples++;
    }
}

static void hook_list(struct List *l, int *skipped)
{
    struct Node *n;
    for (n = l->lh_Head; n->ln_Succ; n = n->ln_Succ) {
        struct Task *t = (struct Task *)n;
        St *s = find(t, 1);
        if (!s) continue;
        strncpy(s->name, n->ln_Name ? n->ln_Name : "(no name)", sizeof s->name - 1);
        if (t->tc_Flags & (TF_SWITCH | TF_LAUNCH)) { (*skipped)++; continue; }
        t->tc_Switch = (APTR)tap_off;
        t->tc_Launch = (APTR)tap_on;
        t->tc_Flags |= TF_SWITCH | TF_LAUNCH;
        s->hooked = 1;
    }
}

static void unhook_list(struct List *l)
{
    struct Node *n;
    for (n = l->lh_Head; n->ln_Succ; n = n->ln_Succ) {
        struct Task *t = (struct Task *)n;
        if ((APTR)t->tc_Launch == (APTR)tap_on) {
            t->tc_Flags &= ~(TF_SWITCH | TF_LAUNCH);
            t->tc_Switch = t->tc_Launch = NULL;
        }
    }
}

static void pct(char *buf, unsigned long long part, unsigned long long whole)
{
    ULONG v = whole ? (ULONG)(part * 10000 / whole) : 0;
    sprintf(buf, "%3lu.%02lu", (unsigned long)(v / 100), (unsigned long)(v % 100));
}

int main(void)
{
    struct ExecBase *sys = SysBase;
    struct RDArgs *ra;
    LONG args[3] = { 0, 0, 0 }, secs = 5;
    struct timerequest tr;
    struct Interrupt vbi;
    int skipped = 0, i, rc = 0, keep = 0;
    ULONG disp0, idle0, disp1, idle1;
    struct EClockVal ec;
    unsigned long long total, hooked_time = 0;
    char a[16], b[16];

    if (!(ra = ReadArgs("SECONDS/N,NOLVO/S,NOHOOK/S", args, NULL))) { PrintFault(IoErr(), "exectap"); return 10; }
    if (args[0]) secs = *(LONG *)args[0];
    if (secs < 1) secs = 1;
    memset(&tr, 0, sizeof tr);
    if (OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *)&tr, 0)) { PutStr("exectap: no timer.device\n"); FreeArgs(ra); return 20; }
    TimerBase = tr.tr_node.io_Device;
    freq = ReadEClock(&ec);
    tap = AllocVec(sizeof *tap, MEMF_PUBLIC | MEMF_CLEAR);
    ring = AllocVec(sizeof *ring, MEMF_PUBLIC | MEMF_CLEAR);
    if (!tap || !ring) { PutStr("exectap: no memory\n"); rc = 20; goto out; }
    tap->timer = TimerBase;
    ring->sys = sys;
    tap_ptr = (ULONG)tap;
    CacheClearU();

    Printf("exec %ld.%ld, EClock %ld Hz, %ld seconds, tc_Launch/tc_Switch %s, Switch/Dispatch vectors %s\n",
           (LONG)sys->LibNode.lib_Version, (LONG)sys->LibNode.lib_Revision, (LONG)freq, secs,
           args[2] ? "off" : "on", args[1] ? "off" : "counted");

    memset(&vbi, 0, sizeof vbi);
    vbi.is_Node.ln_Type = NT_INTERRUPT;
    vbi.is_Node.ln_Pri = -60;
    vbi.is_Node.ln_Name = (char *)"exectap sampler";
    vbi.is_Data = ring;
    vbi.is_Code = tap_vb;
    AddIntServer(INTB_VERTB, &vbi);

    Disable();
    disp0 = sys->DispCount;
    idle0 = sys->IdleCount;
    if (!args[1]) {
        tap->old_switch = SetFunction((struct Library *)sys, -54, (APTR)tap_sw);
        tap->old_dispatch = SetFunction((struct Library *)sys, -60, (APTR)tap_di);
    }
    if (!args[2]) {
        hook_list(&sys->TaskReady, &skipped);
        hook_list(&sys->TaskWait, &skipped);
        /* ourselves: ThisTask is in neither list */
        {
            struct Task *me = sys->ThisTask;
            St *s = find(me, 1);
            if (s) {
                strncpy(s->name, me->tc_Node.ln_Name ? me->tc_Node.ln_Name : "(no name)", sizeof s->name - 1);
                if (me->tc_Flags & (TF_SWITCH | TF_LAUNCH)) skipped++;
                else {
                    me->tc_Switch = (APTR)tap_off;
                    me->tc_Launch = (APTR)tap_on;
                    me->tc_Flags |= TF_SWITCH | TF_LAUNCH;
                    s->hooked = 1;
                }
            }
        }
    }
    Enable();

    for (i = 0; i < secs * 5; i++) {
        Delay(10);
        drain();
        if (SetSignal(0, 0) & SIGBREAKF_CTRL_C) { PutStr("*** Break\n"); break; }
    }

    Disable();
    if (!args[2]) {
        struct Task *me = sys->ThisTask;
        unhook_list(&sys->TaskReady);
        unhook_list(&sys->TaskWait);
        if ((APTR)me->tc_Launch == (APTR)tap_on) {
            me->tc_Flags &= ~(TF_SWITCH | TF_LAUNCH);
            me->tc_Switch = me->tc_Launch = NULL;
        }
    }
    disp1 = sys->DispCount;
    idle1 = sys->IdleCount;
    if (!args[1]) {
        APTR p = SetFunction((struct Library *)sys, -60, tap->old_dispatch);
        if (p != (APTR)tap_di) { SetFunction((struct Library *)sys, -60, p); keep = 1; }
        p = SetFunction((struct Library *)sys, -54, tap->old_switch);
        if (p != (APTR)tap_sw) { SetFunction((struct Library *)sys, -54, p); keep = 1; }
    }
    Enable();
    RemIntServer(INTB_VERTB, &vbi);
    drain();

    Printf("\nexec counters: DispCount +%ld, IdleCount +%ld\n", (LONG)(disp1 - disp0), (LONG)(idle1 - idle0));
    if (!args[1])
        Printf("vector calls:  Switch() %ld, Dispatch() %ld\n", (LONG)tap->lvo_switch, (LONG)tap->lvo_dispatch);
    if (!args[2]) {
        Printf("task hooks:    %ld events, %ld lost; %ld tasks hooked, %ld already had hooks\n",
               (LONG)drained, (LONG)lost, (LONG)(nst - skipped), (LONG)skipped);
        Printf("               on without off %ld, off for another task %ld\n", (LONG)on_without_off, (LONG)off_mismatch);
    }
    total = lastt - first;
    for (i = 0; i < nst; i++) hooked_time += st[i].time;
    if (seen && total) {
        pct(a, gap, total);
        Printf("exact window:  %ld ms; time between a task's off and the next on (idle, unhooked tasks, exec): %s%%\n",
               (LONG)(total * 1000 / freq), a);
    }
    pct(a, vbidle, vbtotal);
    Printf("sampled:       %ld samples, idle %s%%\n\n", (LONG)vbtotal, a);
    Printf("%-32s %8s %8s %8s %7s\n", "TASK", "EXACT%", "SAMPLED%", "SWITCHES", "SAMPLES");
    for (i = 0; i < nst; i++) {
        if (!st[i].time && !st[i].samples) continue;
        pct(a, st[i].time, total);
        pct(b, st[i].samples, vbtotal);
        Printf("%-32.32s %7s%% %7s%% %8ld %7ld%s\n", st[i].name, a, b, (LONG)st[i].launches, (LONG)st[i].samples,
               st[i].hooked ? "" : "  (not hooked)");
    }
    if (keep) {
        /* our patch is under someone else's: unloading would leave exec
         * jumping into freed memory, so stay */
        PutStr("\nexectap: another program patched exec after us: staying in memory (reboot to end)\n");
        Wait(0);
    }

out:
    if (!keep) { if (tap) FreeVec(tap); if (ring) FreeVec(ring); }
    CloseDevice((struct IORequest *)&tr);
    FreeArgs(ra);
    return rc;
}
