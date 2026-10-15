/* wget_tls.c - https for wget (wget.h says what each call does): OpenSSL
 * 3 as AmiSSL 5's shared library, opened only when the first https
 * address comes up. Opened as in Knowledge/amiga/ssh-libssh2-amissl.md.
 * The certificates: AmiSSL's own store, AmiSSL:Certs (hashed names, as
 * the AmiSSL installer leaves them; OpenSSL's default path in AmiSSL),
 * plus AmiSSL:UserCerts, the place AmiSSL's documentation gives for
 * certificates added by hand. No callbacks into wget's code (it is built
 * -resident32: a library calling back would find a4 wrong). */
#include <exec/types.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>
#include <proto/amissl.h>
#include <proto/amisslmaster.h>
#include <libraries/amisslmaster.h>
#include <amissl/tags.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#include "wget.h"

struct Library *AmiSSLBase, *AmiSSLExtBase, *AmiSSLMasterBase;
static SSL_CTX *ctx;
static int checking;

/* an assign or volume of that name (no requester for a missing one) */
static int have_device(const char *name)
{
    struct DosList *dl = LockDosList(LDF_ASSIGNS | LDF_VOLUMES | LDF_READ);
    int yes = FindDosEntry(dl, (STRPTR)name, LDF_ASSIGNS | LDF_VOLUMES) != NULL;
    UnLockDosList(LDF_ASSIGNS | LDF_VOLUMES | LDF_READ);
    return yes;
}

static int have_dir(const char *path)
{
    BPTR l = Lock((STRPTR)path, SHARED_LOCK);
    if (l) UnLock(l);
    return l != 0;
}

const char *tls_init(struct Library *socketbase, int *errnoptr, int verify,
                     const char *cafile, const char *cadir)
{
    if (ctx) return NULL;
    if (!AmiSSLMasterBase) {
        if (!(AmiSSLMasterBase = OpenLibrary((STRPTR)"amisslmaster.library", AMISSLMASTER_MIN_VERSION)))
            return "https needs AmiSSL 5 (amisslmaster.library not found)";
        if (OpenAmiSSLTags(AMISSL_CURRENT_VERSION,
                           AmiSSL_UsesOpenSSLStructs, FALSE,
                           AmiSSL_GetAmiSSLBase, (ULONG)&AmiSSLBase,
                           AmiSSL_GetAmiSSLExtBase, (ULONG)&AmiSSLExtBase,
                           AmiSSL_SocketBase, (ULONG)socketbase,
                           AmiSSL_ErrNoPtr, (ULONG)errnoptr, TAG_DONE)) {
            CloseLibrary(AmiSSLMasterBase);
            AmiSSLMasterBase = NULL;
            return "https needs AmiSSL 5: it would not open (too old, or not enough memory)";
        }
    }
    if (!(ctx = SSL_CTX_new(TLS_client_method()))) return "AmiSSL could not set up TLS";
    /* the classic key exchanges only: OpenSSL 3.5+ offers X25519MLKEM768
     * first, which costs a 68030 seconds more per connection */
    SSL_CTX_set1_groups_list(ctx, "X25519:P-256:P-384");
    checking = verify;
    if (!verify) {
        SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
        return NULL;
    }
    if (cafile || cadir) {
        if (SSL_CTX_load_verify_locations(ctx, cafile, cadir) != 1) {
            ERR_clear_error();
            tls_end();
            return cafile ? "cannot read the certificates of --ca-certificate"
                          : "cannot use the drawer of --ca-directory";
        }
    } else {
        if (!have_device("AmiSSL")) {
            tls_end();
            return "no certificates to check with: AmiSSL: is not assigned "
                   "(its Certs drawer holds them); --no-check-certificate skips the check";
        }
        SSL_CTX_set_default_verify_paths(ctx);   /* AmiSSL:certs, AmiSSL:cert.pem */
        if (have_dir("AmiSSL:UserCerts")) SSL_CTX_load_verify_locations(ctx, NULL, "AmiSSL:UserCerts");
        ERR_clear_error();
    }
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
    return NULL;
}

void tls_end(void)
{
    if (ctx) { SSL_CTX_free(ctx); ctx = NULL; }
    if (AmiSSLMasterBase) {
        CloseAmiSSL();
        CloseLibrary(AmiSSLMasterBase);
        AmiSSLMasterBase = NULL;
        AmiSSLBase = AmiSSLExtBase = NULL;
    }
}

static int isip(const char *h)
{
    for (; *h; h++) if (!((*h >= '0' && *h <= '9') || *h == '.')) return 0;
    return 1;
}

void *tls_new(long fd, const char *host)
{
    SSL *s;
    if (!ctx || !(s = SSL_new(ctx))) return NULL;
    SSL_set_fd(s, fd);
    if (!isip(host)) SSL_set_tlsext_host_name(s, (char *)host);
    if (checking) {
        if (isip(host)) X509_VERIFY_PARAM_set1_ip_asc(SSL_get0_param(s), host);
        else SSL_set1_host(s, host);
    }
    SSL_set_connect_state(s);
    return s;
}

static void copy(char *to, const char *s, int n)
{
    int k = strlen(s);
    if (k > n - 1) k = n - 1;
    memcpy(to, s, k);
    to[k] = 0;
}

int tls_handshake(void *t, char *why, int n)
{
    SSL *s = t;
    int r = SSL_do_handshake(s), e;
    long v;
    unsigned long err;
    if (r == 1) return 1;
    e = SSL_get_error(s, r);
    if (e == SSL_ERROR_WANT_READ) return 0;
    if (e == SSL_ERROR_WANT_WRITE) return 2;
    v = SSL_get_verify_result(s);
    err = ERR_get_error();
    ERR_clear_error();
    if (checking && v != X509_V_OK) {
        const char *w = X509_verify_cert_error_string(v);
        copy(why, "certificate not trusted: ", n);
        copy(why + strlen(why), w ? w : "?", n - strlen(why));
        copy(why + strlen(why), " (--no-check-certificate skips the check)", n - strlen(why));
    } else if (err) {
        const char *w = ERR_reason_error_string(err);
        copy(why, "TLS handshake failed: ", n);
        copy(why + strlen(why), w ? w : "?", n - strlen(why));
    } else copy(why, "TLS handshake failed: the connection was closed", n);
    return -1;
}

static char lasterr[120];

/* what the last failed read or write said */
const char *tls_error(void) { return lasterr; }

static void keep(int e)
{
    unsigned long err = ERR_peek_error();
    const char *w = err ? ERR_reason_error_string(err) : NULL;
    if (w) copy(lasterr, w, sizeof lasterr);
    else if (e == SSL_ERROR_SYSCALL) copy(lasterr, "the connection broke", sizeof lasterr);
    else { strcpy(lasterr, "error "); lasterr[6] = '0' + e % 10; lasterr[7] = 0; }
}

long tls_read(void *t, char *buf, long n)
{
    int r = SSL_read((SSL *)t, buf, n), e;
    if (r > 0) return r;
    e = SSL_get_error((SSL *)t, r);
    if (e != SSL_ERROR_WANT_READ && e != SSL_ERROR_WANT_WRITE) keep(e);
    ERR_clear_error();
    if (e == SSL_ERROR_WANT_READ) return -2;
    if (e == SSL_ERROR_WANT_WRITE) return -3;
    if (e == SSL_ERROR_ZERO_RETURN) return 0;
    /* a server that closes without close_notify (many do, after the
     * last byte): an end, not an error; the length check tells */
    if (e == SSL_ERROR_SYSCALL) return 0;
    return -1;
}

long tls_write(void *t, const char *buf, long n)
{
    int r = SSL_write((SSL *)t, buf, n), e;
    if (r > 0) return r;
    e = SSL_get_error((SSL *)t, r);
    if (e != SSL_ERROR_WANT_READ && e != SSL_ERROR_WANT_WRITE) keep(e);
    ERR_clear_error();
    if (e == SSL_ERROR_WANT_READ) return -2;
    if (e == SSL_ERROR_WANT_WRITE) return -3;
    return -1;
}

void tls_free(void *t)
{
    if (t) SSL_free((SSL *)t);
}
