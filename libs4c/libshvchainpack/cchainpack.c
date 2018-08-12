#include "cchainpack.h"

#include <string.h>
#include <stdio.h>
#include <math.h>

// UTC msec since 2.2. 2018 folowed by signed UTC offset in 1/4 hour
// Fri Feb 02 2018 00:00:00 == 1517529600 EPOCH
static const int64_t SHV_EPOCH_MSEC = 1517529600000;
static const uint8_t ARRAY_FLAG_MASK = 64;
static const uint8_t STRING_META_KEY_PREFIX = 0xFE;
typedef enum {
	INVALID = -1,

	Null = 128,
	UInt,
	Int,
	Double,
	Bool,
	Blob_depr, // deprecated
	String,
	DateTimeEpoch_depr, // deprecated
	List,
	Map,
	IMap,
	MetaMap,
	Decimal,
	DateTime,
	CString,

	FALSE = 253,
	TRUE = 254,
	TERM = 255,
} cchinpack_packing_schema;

static const char* cchinpack_packing_schema_name(cchinpack_packing_schema e)
{

}

void cchainpack_pack_uint(ccpcp_pack_context* pack_context, uint64_t i)
{
	if (pack_context->err_no)
		return;
}

void cchainpack_pack_int(ccpcp_pack_context* pack_context, int64_t i)
{
	if (pack_context->err_no)
		return;

}

void cchainpack_pack_decimal(ccpcp_pack_context *pack_context, int64_t i, int dec_places)
{
	if (pack_context->err_no)
		return;

}

void cchainpack_pack_double(ccpcp_pack_context* pack_context, double d)
{
	if (pack_context->err_no)
		return;

}

void cchainpack_pack_date_time(ccpcp_pack_context *pack_context, int64_t epoch_msecs, int min_from_utc)
{
	if (pack_context->err_no)
		return;
}

void cchainpack_pack_null(ccpcp_pack_context* pack_context)
{
	if (pack_context->err_no)
		return;
}

static void cchainpack_pack_true (ccpcp_pack_context* pack_context)
{
	if (pack_context->err_no)
		return;
}

static void cchainpack_pack_false (ccpcp_pack_context* pack_context)
{
	if (pack_context->err_no)
		return;
}

void cchainpack_pack_boolean(ccpcp_pack_context* pack_context, bool b)
{
	if (pack_context->err_no)
		return;

}

void cchainpack_pack_array_begin(ccpcp_pack_context* pack_context, int size)
{
	if (pack_context->err_no)
		return;

}

void cchainpack_pack_array_end(ccpcp_pack_context *pack_context)
{
}

void cchainpack_pack_list_begin(ccpcp_pack_context *pack_context)
{
	if (pack_context->err_no)
		return;


}

void cchainpack_pack_list_end(ccpcp_pack_context *pack_context)
{
	if (pack_context->err_no)
		return;

}

void cchainpack_pack_map_begin(ccpcp_pack_context *pack_context)
{
	if (pack_context->err_no)
		return;

}

void cchainpack_pack_map_end(ccpcp_pack_context *pack_context)
{
	if (pack_context->err_no)
		return;

}

void cchainpack_pack_imap_begin(ccpcp_pack_context* pack_context)
{
	if (pack_context->err_no)
		return;

}

void cchainpack_pack_imap_end(ccpcp_pack_context *pack_context)
{
}

void cchainpack_pack_meta_begin(ccpcp_pack_context *pack_context)
{
	if (pack_context->err_no)
		return;

}

void cchainpack_pack_meta_end(ccpcp_pack_context *pack_context)
{
	if (pack_context->err_no)
		return;

}
/*
static uint8_t* ccpon_pack_blob_data_escaped(ccpcp_pack_context* pack_context, const void* v, unsigned l)
{
	if (pack_context->err_no)
		return 0;
	uint8_t *p = ccpon_pack_reserve_space(pack_context, 4*l);
	if(p) {
		for (unsigned i = 0; i < l; i++) {
			const uint8_t ch = ((const uint8_t*)v)[i];
			switch (ch) {
			case '\\': *p++ = '\\'; *p++ = '\\'; break;
			case '"' : *p++ = '\\'; *p++ = '"'; break;
			case '\b': *p++ = '\\'; *p++ = 'b'; break;
			case '\f': *p++ = '\\'; *p++ = 'f'; break;
			case '\n': *p++ = '\\'; *p++ = 'n'; break;
			case '\r': *p++ = '\\'; *p++ = 'r'; break;
			case '\t': *p++ = '\\'; *p++ = 't'; break;
			default: {
				if (ch < ' ') {
					int n = snprintf((char*)p, 4, "\\x%02x", ch);
					p += n;
				}
				else {
					*p++ = ch;
				}
				break;
			}
			}
		}
	}
	pack_context->current = p;
	return p;
}

void cchainpack_pack_str(ccpcp_pack_context* pack_context, const char* s, unsigned l)
{
	if (pack_context->err_no)
		return;
	uint8_t *p = ccpon_pack_reserve_space(pack_context, 1);
	if(p) {
		*p = '"';
		p = ccpon_pack_blob_data_escaped(pack_context, s, 4*l);
		if(p) {
			p = ccpon_pack_blob_data_escaped(pack_context, s, 1);
			if(p)
				*p = '"';
		}
	}
}

void cchainpack_pack_blob(ccpcp_pack_context* pack_context, const void* v, unsigned l)
{
	if (pack_context->err_no)
		return;
	uint8_t *p = ccpon_pack_reserve_space(pack_context, sizeof (CCPCP_STR_ESC_BLOB_BEGIN) - 1);
	if(p) {
		memcpy(p, CCPCP_STR_ESC_BLOB_BEGIN, sizeof (CCPCP_STR_ESC_BLOB_BEGIN) - 1);
		p = ccpon_pack_blob_data_escaped(pack_context, v, 4*l);
		if(p) {
			p = ccpon_pack_blob_data_escaped(pack_context, v, 1);
			if(p)
				*p = '"';
		}
	}
}
*/
//============================   U N P A C K   =================================

void cchainpack_unpack_next (ccpcp_unpack_context* unpack_context)
{
	if (unpack_context->err_no)
		return;

	uint8_t *p;

	unpack_context->item.type = CCPCP_ITEM_INVALID;

}

