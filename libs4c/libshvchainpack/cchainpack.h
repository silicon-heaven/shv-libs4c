#ifndef C_CHAINPACK_H
#define C_CHAINPACK_H

#include "ccpcp.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

void cchainpack_pack_null (ccpcp_pack_context* pack_context);
void cchainpack_pack_boolean (ccpcp_pack_context* pack_context, bool b);
void cchainpack_pack_int (ccpcp_pack_context* pack_context, int64_t i);
void cchainpack_pack_uint (ccpcp_pack_context* pack_context, uint64_t i);
void cchainpack_pack_double (ccpcp_pack_context* pack_context, double d);
void cchainpack_pack_decimal (ccpcp_pack_context* pack_context, int64_t i, int dec_places);
void cchainpack_pack_str (ccpcp_pack_context* pack_context, const char* s, unsigned l);
//void cchainpack_pack_blob (ccpcp_pack_context* pack_context, const void *v, unsigned l);
void cchainpack_pack_date_time (ccpcp_pack_context* pack_context, int64_t epoch_msecs, int min_from_utc);

void cchainpack_pack_array_begin (ccpcp_pack_context* pack_context, int size);
void cchainpack_pack_array_end (ccpcp_pack_context* pack_context);

void cchainpack_pack_list_begin (ccpcp_pack_context* pack_context);
void cchainpack_pack_list_end (ccpcp_pack_context* pack_context);

void cchainpack_pack_map_begin (ccpcp_pack_context* pack_context);
void cchainpack_pack_map_end (ccpcp_pack_context* pack_context);

void cchainpack_pack_imap_begin (ccpcp_pack_context* pack_context);
void cchainpack_pack_imap_end (ccpcp_pack_context* pack_context);

void cchainpack_pack_meta_begin (ccpcp_pack_context* pack_context);
void cchainpack_pack_meta_end (ccpcp_pack_context* pack_context);

void cchainpack_unpack_next (ccpcp_unpack_context* unpack_context);

#ifdef __cplusplus
}
#endif

#endif /* C_CHAINPACK_H */
