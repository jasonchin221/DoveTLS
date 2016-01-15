
#include "dv_crypto.h"
#include "dv_ssl.h"
#include "dv_tls_loc.h"
#include "dv_tls.h"
#include "dv_lib.h"
#include "dv_errno.h"
#include "dv_debug.h"
#include "dv_tls1_2_proto.h"
#include "dv_assert.h"

static int dv_tls1_2_parse_handshake(dv_ssl_t *s, void *buf, dv_u32 len);
static int dv_tls1_2_parse_client_hello(dv_ssl_t *s, void *buf, dv_u32 len);
static int dv_tls1_2_parse_server_hello(dv_ssl_t *s, void *buf, dv_u32 len);

static const dv_u16 dv_tls1_2_cipher_suites[] = {
    DV_TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384, 
    DV_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,
    DV_TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384,
    DV_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256,
    DV_TLS_RSA_WITH_AES_256_GCM_SHA384,
    DV_TLS_RSA_WITH_AES_256_CBC_SHA256,
    DV_TLS_RSA_WITH_AES_128_GCM_SHA256,
    DV_TLS_RSA_WITH_AES_128_CBC_SHA256,
};

static const dv_msg_parse_t dv_tls1_2_msg_parser[] = {
    {
        DV_TLS_CONTENT_TYPE_HANDSHAKE,
        dv_tls1_2_parse_handshake,
    },
};

static const dv_msg_parse_t dv_tls1_2_handshake_parser[] = {
    {
        DV_TLS_HANDSHAKE_TYPE_CLIENT_HELLO,
        dv_tls1_2_parse_client_hello,
    },
    {
        DV_TLS_HANDSHAKE_TYPE_SERVER_HELLO,
        dv_tls1_2_parse_server_hello,
    },
};


#define dv_tls1_2_client_hello_head_set(buf, rh, hh, ch, suites) \
    do { \
        rh = buf; \
        hh = (dv_tls_handshake_header_t *)(rh + 1); \
        ch = (dv_tlsv1_2_client_hello_t *)(hh + 1); \
        suites = (dv_u16 *)(ch + 1); \
    } while (0)

#define dv_certificate_set_header(ca, clen, tlen) \
    do {\
        ca -= sizeof(clen); \
        DV_SET_LENGTH(clen, tlen); \
        memcpy(ca, clen, sizeof(clen)); \
        tlen += sizeof(clen); \
    } while(0)


static dv_u16
dv_tls1_2_set_handshake_header(dv_tls_record_header_t *rh, 
        dv_tls_handshake_header_t *hh, dv_u16 version,
        dv_u8 type, dv_u32 hlen)
{
    dv_u16      tlen = 0;

    hh->hh_msg_type = type;
    DV_SET_LENGTH(hh->hh_length, hlen);
    
    rh->rh_content_type = DV_TLS_CONTENT_TYPE_HANDSHAKE;
    rh->rh_version.pv_version = DV_HTONS(version);
    tlen = sizeof(*hh) + hlen;
    rh->rh_length = DV_HTONS(tlen);

    return tlen + sizeof(*rh);
}

int
dv_tls1_2_client_hello(dv_ssl_t *s)
{
    dv_tls_record_header_t      *rh = NULL;
    dv_tls_handshake_header_t   *hh = NULL;
    dv_tlsv1_2_client_hello_t   *ch = NULL;
    dv_u8                       *cpre = NULL;
    dv_u16                      *cipher_suites_len = NULL;
    dv_u16                      *ext_len = NULL;
    dv_u32                      hlen = 0;
    dv_u32                      len = s->ssl_method->md_msg_max_len;
    dv_u16                      tlen = 0;

    dv_tls1_2_client_hello_head_set(s->ssl_msg, rh, hh, ch, cipher_suites_len);
    cpre = (dv_u8 *)(cipher_suites_len + 
            DV_ARRAY_SIZE(dv_tls1_2_cipher_suites) + 1);
    ext_len = (dv_u16 *)(cpre + 2);
    *ext_len = 0;
    hlen += sizeof(*ext_len);
    *cpre++ = 1;
    *cpre = 0;
    hlen += 2*sizeof(*cpre);

    *cipher_suites_len = sizeof(dv_tls1_2_cipher_suites);

    dv_tls_get_cipher_suites(cipher_suites_len + 1,
            dv_tls1_2_cipher_suites,
            DV_ARRAY_SIZE(dv_tls1_2_cipher_suites));
    hlen += sizeof(*cipher_suites_len ) + *cipher_suites_len; 
    *cipher_suites_len = DV_HTONS(*cipher_suites_len);

    ch->ch_version.pv_version = DV_HTONS(s->ssl_method->md_version);
    s->ssl_method->md_bio_get_time(&ch->ch_random.rd_gmt_unix_time);
    hlen += sizeof(dv_tlsv1_2_client_hello_t);


    tlen = dv_tls1_2_set_handshake_header(rh, hh, DV_TLS1_0_VERSION,
        DV_TLS_HANDSHAKE_TYPE_CLIENT_HELLO, hlen);

    dv_assert(tlen <= len);

    return tlen;
}

static int
_dv_tls1_2_server_hello(dv_ssl_t *s, void *buf, int len)
{
    dv_tls_record_header_t      *rh = NULL;
    dv_tls_handshake_header_t   *hh = NULL;
    dv_tlsv1_2_server_hello_t   *sh = NULL;
    dv_u32                      hlen = 0;
    dv_u16                      mlen = 0;

    if (len < sizeof(*rh) + sizeof(*sh) + sizeof(hh)) {
        return DV_ERROR;
    }

    rh = buf;
    hh = (dv_tls_handshake_header_t *)(rh + 1);
    sh = (dv_tlsv1_2_server_hello_t *)(hh + 1);

    s->ssl_method->md_bio_get_time(&sh->sh_random.rd_gmt_unix_time);
    sh->sh_version.pv_version = DV_HTONS(s->ssl_method->md_version);
    sh->sh_cipher_suite = DV_HTONS(s->ssl_cipher_suite);
    sh->sh_session_id = 0;
    sh->sh_compress_method = 0;
    sh->sh_ext_len = 0;
    hlen += sizeof(*sh);

    mlen = dv_tls1_2_set_handshake_header(rh, hh, s->ssl_method->md_version,
        DV_TLS_HANDSHAKE_TYPE_SERVER_HELLO, hlen);

    dv_assert(mlen <= len);
    
    return mlen;
}

static int
dv_tls1_2_certificate(dv_ssl_t *s, void *buf, int len)
{
    dv_tls_record_header_t      *rh = NULL;
    dv_tls_handshake_header_t   *hh = NULL;
    dv_u8                       *ca = NULL;
    dv_u8                       clen[3] = {0};
    dv_u32                      tlen = 0;
    dv_u16                      mlen = 0;

    if (s->ssl_ca == NULL) {
        return DV_ERROR;
    }

    tlen = s->ssl_ca_len;
    rh = buf;
    hh = (dv_tls_handshake_header_t *)(rh + 1);
    ca = (dv_u8 *)(hh + 1);

    ca += 2*sizeof(clen);

    dv_assert((tlen + ca - (dv_u8 *)buf) <= len);

    memcpy(ca, s->ssl_ca, tlen);

    dv_certificate_set_header(ca, clen, tlen);
    dv_certificate_set_header(ca, clen, tlen);

    mlen = dv_tls1_2_set_handshake_header(rh, hh, s->ssl_method->md_version,
        DV_TLS_HANDSHAKE_TYPE_CERTIFICATE, tlen);

    dv_assert(mlen <= len);
    
    return mlen;
}


int
dv_tls1_2_server_hello(dv_ssl_t *s)
{
    void                *buf = NULL;
    int                 mlen = 0;
    int                 tlen = 0;

    buf = s->ssl_msg;
    tlen = s->ssl_method->md_msg_max_len;
    mlen = _dv_tls1_2_server_hello(s, buf, tlen);
    if (mlen <= 0) {
        return DV_ERROR;
    }
    buf += mlen;
    tlen -= mlen;

    mlen = dv_tls1_2_certificate(s, buf, tlen);
    if (mlen < 0) {
        return DV_ERROR;
    }
    buf += mlen;
    tlen -= mlen;

    if (s->ssl_ca_mode & DV_SSL_VERIFY_PEER) {
    }

    mlen = dv_tls1_2_set_handshake_header(buf, 
            buf + sizeof(dv_tls_record_header_t), 
            s->ssl_method->md_version,
            DV_TLS_HANDSHAKE_TYPE_SERVER_HELLO_DONE, 0);

    dv_assert(mlen <= tlen);
    
    return s->ssl_method->md_msg_max_len - tlen + mlen;
}

static int
dv_tls1_2_parse_client_hello(dv_ssl_t *s, void *buf, dv_u32 len)
{
    dv_tlsv1_2_client_hello_t   *ch = NULL;
    dv_u16                      *cipher_suites_len = NULL;
    dv_u16                      cipher_suite = 0;
    int                         i = 0;

    if (s->ssl_state != DV_SSL_STATE_INIT) {
        DV_DEBUG("Err: Can't receive Client Hello in state(%d)\n",
                s->ssl_state);
        return DV_ERROR;
    }

    if (s->ssl_server != DV_TRUE) {
        DV_DEBUG("Err: Client receive Client Hello\n");
        return DV_ERROR;
    }

    ch = buf;
    if (DV_NTOHS(ch->ch_version.pv_version) != DV_TLS1_2_VERSION) {
        DV_DEBUG("TLS Client Hello version(%X) invalid!\n", 
                DV_NTOHS(ch->ch_version.pv_version));
        return DV_ERROR;
    }

    cipher_suites_len = (dv_u16 *)(ch + 1);
    for (i = 0; i < DV_NTOHS(*cipher_suites_len)/sizeof(dv_u16); i++) {
        cipher_suite = DV_NTOHS(*(cipher_suites_len + 1 + i));
        if (dv_tls_match_cipher_suites(cipher_suite, dv_tls1_2_cipher_suites,
            DV_ARRAY_SIZE(dv_tls1_2_cipher_suites)) == DV_TRUE) {
            s->ssl_cipher_suite = cipher_suite;
            break;
        }
    }

    return DV_OK;
}

static int
dv_tls1_2_parse_server_hello(dv_ssl_t *s, void *buf, dv_u32 len)
{
    return DV_OK;
}

static int
dv_tls1_2_parse_handshake(dv_ssl_t *s, void *buf, dv_u32 len)
{
    return dv_tls_parse_handshake(s, dv_tls1_2_handshake_parser,
            DV_ARRAY_SIZE(dv_tls1_2_handshake_parser), buf, len);
}

int 
dv_tls1_2_parse_message(dv_ssl_t *s)
{
    return dv_tls_parse_record(s, dv_tls1_2_msg_parser,
            DV_ARRAY_SIZE(dv_tls1_2_msg_parser));
}

