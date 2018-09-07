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

//extern const uint8_t CP_ARRAY_FLAG_MASK;
extern const uint8_t CP_STRING_META_KEY_PREFIX;

typedef enum {
	CP_INVALID = -1,

	CP_Null = 128,
	CP_UInt,
	CP_Int,
	CP_Double,
	CP_Bool,
	CP_Blob_depr, // deprecated
	CP_String,
	CP_DateTimeEpoch_depr, // deprecated
	CP_List,
	CP_Map,
	CP_IMap,
	CP_MetaMap,
	CP_Decimal,
	CP_DateTime,
	CP_CString,

	CP_FALSE = 253,
	CP_TRUE = 254,
	CP_TERM = 255,
} cchainpack_pack_packing_schema;

const char* cchainpack_packing_schema_name(int sch);

void cchainpack_pack_uint_data(ccpcp_pack_context* pack_context, uint64_t num);

void cchainpack_pack_null (ccpcp_pack_context* pack_context);
void cchainpack_pack_boolean (ccpcp_pack_context* pack_context, bool b);
void cchainpack_pack_int (ccpcp_pack_context* pack_context, int64_t i);
void cchainpack_pack_uint (ccpcp_pack_context* pack_context, uint64_t i);
void cchainpack_pack_double (ccpcp_pack_context* pack_context, double d);
void cchainpack_pack_decimal (ccpcp_pack_context* pack_context, int64_t i, int dec_places);
//void cchainpack_pack_blob (ccpcp_pack_context* pack_context, const void *v, unsigned l);
void cchainpack_pack_date_time (ccpcp_pack_context* pack_context, int64_t epoch_msecs, int min_from_utc);

void cchainpack_pack_string_key (ccpcp_pack_context* pack_context, const char* str, size_t str_len);
void cchainpack_pack_string_key_meta (ccpcp_pack_context* pack_context, const char* str, size_t str_len);
void cchainpack_pack_uint_key (ccpcp_pack_context* pack_context, uint64_t key);

void cchainpack_pack_string (ccpcp_pack_context* pack_context, const char* buff, size_t buff_len);
void cchainpack_pack_string_start (ccpcp_pack_context* pack_context, size_t string_len, const char* buff, size_t buff_len);
void cchainpack_pack_string_cont (ccpcp_pack_context* pack_context, const char* buff, size_t buff_len);

void cchainpack_pack_cstring (ccpcp_pack_context* pack_context, const char* buff, size_t buff_len);
void cchainpack_pack_cstring_terminated (ccpcp_pack_context* pack_context, const char* str);
void cchainpack_pack_cstring_start (ccpcp_pack_context* pack_context, const char* buff, size_t buff_len);
void cchainpack_pack_cstring_cont (ccpcp_pack_context* pack_context, const char* buff, size_t buff_len);
void cchainpack_pack_cstring_finish (ccpcp_pack_context* pack_context);

//void cchainpack_pack_array_begin (ccpcp_pack_context* pack_context, ccpcp_item_types type, int size);
void cchainpack_pack_container_end(ccpcp_pack_context* pack_context);

void cchainpack_pack_list_begin (ccpcp_pack_context* pack_context);
//void cchainpack_pack_list_end (ccpcp_pack_context* pack_context);

void cchainpack_pack_map_begin (ccpcp_pack_context* pack_context);
//void cchainpack_pack_map_end (ccpcp_pack_context* pack_context);

void cchainpack_pack_imap_begin (ccpcp_pack_context* pack_context);
//void cchainpack_pack_imap_end (ccpcp_pack_context* pack_context);

void cchainpack_pack_meta_begin (ccpcp_pack_context* pack_context);
//void cchainpack_pack_meta_end (ccpcp_pack_context* pack_context);

void cchainpack_unpack_next (ccpcp_unpack_context* unpack_context);

#ifdef __cplusplus
}
#endif

#endif /* C_CHAINPACK_H */
