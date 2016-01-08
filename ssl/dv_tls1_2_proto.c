
#include "dv_crypto.h"
#include "dv_ssl.h"
#include "dv_tls_loc.h"
#include "dv_tls.h"
#include "dv_lib.h"
#include "dv_errno.h"
#include "dv_tls1_2_proto.h"

static int dv_tls1_2_client_parse_handshake(dv_ssl_t *s, void *buf, dv_u32 len);

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

static const dv_msg_parse_t dv_tls1_2_parse_msg[] = {
    {
        DV_TLS_CONTENT_TYPE_HANDSHAKE,
        dv_tls1_2_client_parse_handshake,
    },
};

#define dv_tls1_2_client_hello_head_set(buf, rh, hh, ch, suites) \
    do { \
        rh = buf; \
        hh = (dv_tls_handshake_header_t *)(rh + 1); \
        ch = (dv_tlsv1_2_client_hello_t *)(hh + 1); \
        suites = (dv_u16 *)(ch + 1); \
    } while (0)

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

    ch->ch_version.pv_version = DV_HTONS(DV_TLS1_2_VERSION);
    s->ssl_method->md_bio_get_time(&ch->ch_random.rd_gmt_unix_time);
    hlen += sizeof(dv_tlsv1_2_client_hello_t);

    hh->hh_msg_type = DV_TLS_HANDSHAKE_TYPE_CLIENT_HELLO;
    DV_SET_LENGTH(hh->hh_length, hlen);
    
    rh->rh_content_type = DV_TLS_CONTENT_TYPE_HANDSHAKE;
    rh->rh_version.pv_version = DV_HTONS(DV_TLS1_0_VERSION);
    tlen = sizeof(*hh) + hlen;
    rh->rh_length = DV_HTONS(tlen);

    if (sizeof(*rh) + tlen > len) {
        return DV_ERROR;
    }

    return sizeof(*rh) + tlen;
}

/*
 * function: parse message from server
 */
int
dv_tls1_2_client_parse_msg(dv_ssl_t *s)
{
    return DV_OK;
}

int
dv_tls1_2_server_hello(dv_ssl_t *s)
{
    dv_tls_record_header_t  *rh = NULL;

    rh = s->ssl_msg;
    rh->rh_content_type = DV_TLS_CONTENT_TYPE_HANDSHAKE;
    rh->rh_version.pv_version = DV_TLS1_2_VERSION;
    rh->rh_length = 0;
    
    return sizeof(*rh) + rh->rh_length;
}

static int
dv_tls1_2_client_parse_handshake(dv_ssl_t *s, void *buf, dv_u32 len)
{
#if 0
    dv_tls_handshake_header_t   *hh = NULL;
    dv_tlsv1_2_client_hello_t   *ch = NULL;
    dv_u16                      *cipher_suites_len = NULL;
    int                         ret = DV_OK;
#endif

    return DV_OK;
}

int 
dv_tls1_2_parse_message(dv_ssl_t *s)
{
    return dv_tls_parse_record(s, dv_tls1_2_parse_msg,
            DV_ARRAY_SIZE(dv_tls1_2_parse_msg));
}

