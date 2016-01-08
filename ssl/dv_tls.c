
#include "dv_ssl_loc.h"
#include "dv_tls.h"
#include "dv_bio.h"
#include "dv_types.h"
#include "dv_lib.h"
#include "dv_crypto.h"
#include "dv_errno.h"

int
dv_tls_new(dv_ssl_t *s)
{
    s->ssl_msg = dv_malloc(s->ssl_method->md_msg_max_len);
    if (s->ssl_msg == NULL) {
        return DV_ERROR;
    }

    return DV_OK;
}

void
dv_tls_free(dv_ssl_t *s)
{
    if (s->ssl_msg) {
        dv_free(s->ssl_msg);
    }
}

void
dv_tls_get_cipher_suites(dv_u16 *dest, const dv_u16 *suites, dv_u32 num)
{
    int             i = 0;

    for (i = 0; i < num; i++) {
        dest[i] = DV_HTONS(suites[i]);
    }
}

dv_implement_tls_meth_func(DV_TLS1_2_VERSION, DV_TLS_MSG_MAX_LEN, 
        dv_tls_v1_2_client_method, dv_undefined_function, dv_tls_bio_connect,
        dv_tls1_2_client_hello, dv_tls1_2_client_parse_msg, dv_bio_get_time_linux,
        dv_bio_read_sock, dv_bio_write_sock)
dv_implement_tls_meth_func(DV_TLS1_2_VERSION, DV_TLS_MSG_MAX_LEN,
        dv_tls_v1_2_server_method, dv_tls_bio_accept, dv_undefined_function,
        dv_tls1_2_server_hello, dv_tls1_2_client_parse_msg, dv_bio_get_time_linux, 
        dv_bio_read_sock, dv_bio_write_sock)
