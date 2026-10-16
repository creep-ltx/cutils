/* cu_pi.c - the Raspberry Pi under a PiStorm (ctop): its temperature,
 * clocks and throttling through mailbox.resource, Emu68's version and
 * the Pi's model from devicetree.resource. Grew out of Wasabi's health.c
 * (2026-10-16). Each part only where it exists: on a real 68k or in
 * FS-UAE both resources are missing and cu_pi() says so.
 *
 * The firmware is reached only through mailbox.resource (in Emu68's ROM
 * since 1.1): it owns the mailbox under a semaphore, so this never
 * collides with VideoCore.card or Emu68 itself mid-transaction. */
#include <string.h>
#include <exec/types.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include "cu.h"

#define __NOLIBBASE__
#define DEVICETREE_BASE_NAME cu_dt
#define MAILBOX_BASE_NAME cu_mb
#include <proto/devicetree.h>
#include <proto/mailbox.h>
#undef __NOLIBBASE__

static APTR cu_dt, cu_mb;        /* resources: never closed */
static int probed;

static void dtstr(const char *path, const char *prop, char *out, int n)
{
    APTR key, p;
    const char *v;
    ULONG len;
    out[0] = 0;
    if (!cu_dt || !(key = DT_OpenKey((CONST_STRPTR)path))) return;
    p = DT_FindProperty(key, (CONST_STRPTR)prop);
    if (p && (v = (const char *)DT_GetPropValue(p))) {
        len = DT_GetPropLen(p);
        if (len > (ULONG)n - 1) len = n - 1;
        memcpy(out, v, len);
        out[len] = 0;
        for (len = 0; out[len]; len++) if (out[len] == '\n' || out[len] == '\r') out[len] = ' ';
    }
    DT_CloseKey(key);
}

/* one property tag, one u32 in, two out; static: MB_RawCommand flushes
 * the caches over it and the stack may be small */
static ULONG mbuf[8];

static int mbget(ULONG tag, ULONG arg, ULONG *v0, ULONG *v1)
{
    if (!cu_mb) return 0;
    mbuf[0] = 8 * 4;
    mbuf[1] = 0;
    mbuf[2] = tag;
    mbuf[3] = 8;
    mbuf[4] = 4;
    mbuf[5] = arg;
    mbuf[6] = 0;
    mbuf[7] = 0;
    MB_RawCommand(mbuf);
    if (mbuf[1] != 0x80000000UL || !(mbuf[4] & 0x80000000UL)) return 0;
    if (v0) *v0 = mbuf[5];
    if (v1) *v1 = mbuf[6];
    return 1;
}

static void probe(void)
{
    APTR key;
    if (probed) return;
    probed = 1;
    cu_dt = OpenResource("devicetree.resource");
    if (cu_dt && (key = DT_OpenKey((CONST_STRPTR)"/emu68"))) DT_CloseKey(key);
    else cu_dt = NULL;           /* a devicetree without Emu68: not ours */
    cu_mb = OpenResource("mailbox.resource");
}

/* Emu68 as Emu68CP and Wasabi find it: devicetree.resource with an
 * /emu68 node (its custom MOVEC counters are an illegal instruction
 * anywhere else) */
int cu_isemu68(void)
{
    probe();
    return cu_dt != NULL;
}

int cu_pi(CuPi *p)
{
    ULONG a, b;
    memset(p, 0, sizeof *p);
    probe();
    if (!cu_dt && !cu_mb) return 0;
    if (cu_dt) {
        dtstr("/emu68", "idstring", p->emu68, sizeof p->emu68);
        if (!strncmp(p->emu68, "$VER: ", 6)) memmove(p->emu68, p->emu68 + 6, strlen(p->emu68 + 6) + 1);
        dtstr("/", "model", p->model, sizeof p->model);
    }
    if (mbget(0x00030006, 0, &a, &b)) p->temp = b;              /* millidegrees */
    if (mbget(0x0003000a, 0, &a, &b)) p->tempmax = b;
    if (mbget(0x00030046, 0, &a, NULL)) { p->throttled = a; p->hasthrottle = 1; }
    if (mbget(0x00030047, 3, &a, &b)) p->armhz = b;             /* measured */
    else if (mbget(0x00030002, 3, &a, &b)) p->armhz = b;
    if (mbget(0x00030004, 3, &a, &b)) p->armmax = b;
    return 1;
}
