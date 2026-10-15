/* wget.h - what wget.c and wget_tls.c (https on AmiSSL 5) share.
 * The TLS part is a file of its own so the AmiSSL headers stay out of
 * wget.c; AmiSSL itself is only opened for the first https address, so
 * plain http works on a machine without it. */
#ifndef WGET_H
#define WGET_H

/* Open AmiSSL and make the one client context. verify 0: no certificate
 * checks (--no-check-certificate); cafile/cadir: --ca-certificate and
 * --ca-directory (NULL: AmiSSL:Certs and AmiSSL:UserCerts). NULL when
 * fine, else what is wrong, in words. */
const char *tls_init(struct Library *socketbase, int *errnoptr, int verify,
                     const char *cafile, const char *cadir);
void tls_end(void);

/* A TLS connection on the (non-blocking) socket fd, for host (SNI and
 * the name check). NULL: out of memory. */
void *tls_new(long fd, const char *host);
/* One step of the handshake: 1 done, 0 wants reading, 2 wants writing,
 * -1 failed (why: the reason, in words, at most n bytes). */
int tls_handshake(void *t, char *why, int n);
/* >0 bytes, 0 the other end closed, -1 failed, -2 wants reading,
 * -3 wants writing. */
long tls_read(void *t, char *buf, long n);
long tls_write(void *t, const char *buf, long n);
void tls_free(void *t);
const char *tls_error(void);      /* why the last read or write failed */

#endif
