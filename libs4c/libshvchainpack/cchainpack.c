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
} packing_schema;

static const char* cchinpack_packing_schema_name(int cchinpack_packing_schema)
{
	switch (cchinpack_packing_schema) {
	case CP_INVALID: return "INVALID";
	case CP_FALSE: return "FALSE";
	case CP_TRUE: return "TRUE";

	case CP_Null: return "Null";
	case CP_UInt: return "UInt";
	case CP_Int: return "Int";
	case CP_Double: return "Double";
	case CP_Bool: return "Bool";
	case CP_Blob_depr: return "Blob_depr";
	case CP_String: return "String";
	case CP_CString: return "CString";
	case CP_List: return "List";
	case CP_Map: return "Map";
	case CP_IMap: return "IMap";
	case CP_DateTimeEpoch_depr: return "DateTimeEpoch_depr";
	case CP_DateTime: return "DateTime";
	case CP_MetaMap: return "MetaMap";
	//case MetaSMap: return "MetaSMap";
	case CP_Decimal: return "Decimal";

	case CP_TERM: return "TERM";
	}
	//SHVCHP_EXCEPTION("Unknown TypeInfo: " + Utils::toString((int)e));
	return "";
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

/// @pbitlen is used to enable same function usage for signed int unpacking
static void unpack_uint(ccpcp_unpack_context* unpack_context, uint64_t *pval, int *pbitlen)
{
	uint64_t num = 0;
	int bitlen = 0;
	do {
		const uint8_t *p;
		UNPACK_ASSERT_BYTE();
		uint8_t head = *p;

		int bytes_to_read_cnt;
		if     ((head & 128) == 0) {bytes_to_read_cnt = 0; num = head & 127; bitlen = 7;}
		else if((head &  64) == 0) {bytes_to_read_cnt = 1; num = head & 63; bitlen = 6 + 8;}
		else if((head &  32) == 0) {bytes_to_read_cnt = 2; num = head & 31; bitlen = 5 + 2*8;}
		else if((head &  16) == 0) {bytes_to_read_cnt = 3; num = head & 15; bitlen = 4 + 3*8;}
		else {
			bytes_to_read_cnt = (head & 0xf) + 4;
			bitlen = bytes_to_read_cnt * 8;
		}

		for (int i = 0; i < bytes_to_read_cnt; ++i) {
			UNPACK_ASSERT_BYTE();
			uint8_t r = *p;
			num = (num << 8) + r;
		};
	} while(false);
	if(pval)
		*pval = num;
	if(pbitlen)
		*pbitlen = bitlen;
}

static void unpack_int(ccpcp_unpack_context* unpack_context, int64_t *pval)
{
	int64_t snum = 0;
	int bitlen;
	uint64_t num;
	unpack_uint(unpack_context, &num, &bitlen);
	if(unpack_context->err_no == CCPCP_RC_OK) {
		uint64_t sign_bit_mask = 1 << (bitlen - 1);
		bool neg = num & sign_bit_mask;
		snum = (int64_t)num;
		if(neg) {
			snum &= ~sign_bit_mask;
			snum = -snum;
		}
	}
	if(pval)
		*pval = snum;
}

void unpack_string(ccpcp_unpack_context* unpack_context)
{
	if(unpack_context->item.type != CCPCP_ITEM_STRING)
		UNPACK_ERROR(CCPCP_RC_LOGICAL_ERROR);

	ccpcp_string *it = &unpack_context->item.as.String;

	const uint8_t *p;
	UNPACK_ASSERT_BYTE();

	bool is_cstr = it->parse_status.size_to_load < 0;
	if(is_cstr) {
		it->length = 0;
		it->start = p;
		for (unpack_context->current = p; unpack_context->current < unpack_context->end; unpack_context->current++) {
			p = unpack_context->current;
			if(*p == '\\') {
				if(it->length > 0) {
					// finish current chunk, esc sequence wil have own one byte long
					//unpack_context->current--;
					return;
				}
				UNPACK_ASSERT_BYTE();
				if(!p)
					return;
				switch (*p) {
				case '\\': it->parse_status.escaped_byte = '\\'; break;
				case '0' : it->parse_status.escaped_byte = 0; break;
				default: it->parse_status.escaped_byte = *p; break;
				}
				it->start = &(it->parse_status.escaped_byte);
				it->length = 1;
				break;
			}
			else {
				if (*p == 0) {
					// end of string
					unpack_context->current++;
					it->parse_status.last_chunk = 1;
					break;
				}
				else {
					it->length++;
				}
			}
		}
	}
	else {
		it->start = p;
		int64_t buffered_len = unpack_context->end - p;
		if(buffered_len > it->parse_status.size_to_load)
			buffered_len = it->parse_status.size_to_load;
		it->length = buffered_len;
		it->parse_status.size_to_load -= buffered_len;
		if(it->parse_status.size_to_load == 0)
			it->parse_status.last_chunk = 1;
		unpack_context->current = p + it->length;

	}
	it->parse_status.chunk_cnt++;
}

void cchainpack_unpack_next (ccpcp_unpack_context* unpack_context)
{
	if (unpack_context->err_no)
		return;

	const uint8_t *p;
	if(unpack_context->item.type == CCPCP_ITEM_STRING) {
		ccpcp_string *str_it = &unpack_context->item.as.String;
		if(!str_it->parse_status.last_chunk) {
			unpack_string(unpack_context);
			return;
		}
	}

	{
		ccpcp_container_state *state = ccpc_unpack_context_top_container_state(unpack_context);
		if(state) {
			state->item_count++;
		}
	}

	unpack_context->item.type = CCPCP_ITEM_INVALID;

	UNPACK_ASSERT_BYTE();

	uint8_t packing_schema = *p;
	if(packing_schema == STRING_META_KEY_PREFIX) {
		ccpcp_container_state *state = ccpc_unpack_context_top_container_state(unpack_context);
		if(state && state->container_type == CCPCP_ITEM_META && (state->item_count % 2) == 1) {
			packing_schema = CP_String;
			UNPACK_ASSERT_BYTE();
		}
	}

	if(packing_schema < 128) {
		if(packing_schema & 64) {
			// tiny Int
			unpack_context->item.type = CCPCP_ITEM_INT;
			unpack_context->item.as.Int = packing_schema & 63;
		}
		else {
			// tiny UInt
			unpack_context->item.type = CCPCP_ITEM_UINT;
			unpack_context->item.as.UInt = packing_schema & 63;
		}
	}
	else {
		bool is_array = 0;
		if(packing_schema < CP_FALSE) {
			is_array = packing_schema & ARRAY_FLAG_MASK;
			packing_schema &= ~ARRAY_FLAG_MASK;
		}

		switch(packing_schema) {
		case CP_TRUE: {
			unpack_context->item.type = CCPCP_ITEM_BOOLEAN;
			unpack_context->item.as.Bool = 1;
			break;
		}
		case CP_FALSE: {
			unpack_context->item.type = CCPCP_ITEM_BOOLEAN;
			unpack_context->item.as.Bool = 0;
			break;
		}
		case CP_Int: {
			int64_t n;
			unpack_int(unpack_context, &n);
			unpack_context->item.type = CCPCP_ITEM_INT;
			unpack_context->item.as.Int = n;
			break;
		}
		case CP_UInt: {
			uint64_t n;
			unpack_uint(unpack_context, &n, NULL);
			unpack_context->item.type = CCPCP_ITEM_UINT;
			unpack_context->item.as.UInt = n;
			break;
		}
		case CP_MetaMap: {
			unpack_context->item.type = CCPCP_ITEM_META;
			ccpc_unpack_context_push_container_state(unpack_context, unpack_context->item.type);
			break;
		}
		case CP_Map: {
			unpack_context->item.type = CCPCP_ITEM_MAP;
			ccpc_unpack_context_push_container_state(unpack_context, unpack_context->item.type);
			break;
		}
		case CP_IMap: {
			unpack_context->item.type = CCPCP_ITEM_MAP;
			ccpc_unpack_context_push_container_state(unpack_context, unpack_context->item.type);
			break;
		}
		case CP_List: {
			unpack_context->item.type = CCPCP_ITEM_LIST;
			ccpc_unpack_context_push_container_state(unpack_context, unpack_context->item.type);
			break;
		}
		case CP_TERM: {
			ccpcp_container_state *top_cont_state = ccpc_unpack_context_top_container_state(unpack_context);
			if(!top_cont_state)
				UNPACK_ERROR(CCPCP_RC_CONTAINER_STACK_UNDERFLOW)
			switch(top_cont_state->container_type) {
			case CCPCP_ITEM_LIST:
				unpack_context->item.type = CCPCP_ITEM_LIST_END;
				break;
			case CCPCP_ITEM_MAP:
				unpack_context->item.type = CCPCP_ITEM_MAP_END;
				break;
			case CCPCP_ITEM_IMAP:
				unpack_context->item.type = CCPCP_ITEM_IMAP_END;
				break;
			case CCPCP_ITEM_META:
				unpack_context->item.type = CCPCP_ITEM_META_END;
				break;
			default:
				UNPACK_ERROR(CCPCP_RC_MALFORMED_INPUT)
			}
			ccpc_unpack_context_pop_container_state(unpack_context);
			break;
		}
		case CP_String: {
			unpack_context->item.type = CCPCP_ITEM_STRING;
			ccpcp_string *it = &unpack_context->item.as.String;
			ccpcp_string_init(it);
			uint64_t str_len;
			unpack_uint(unpack_context, &str_len, NULL);
			if(unpack_context->err_no == CCPCP_RC_OK) {
				it->parse_status.size_to_load = str_len;
				unpack_string(unpack_context);
			}
			break;
		}
		case CP_CString: {
			unpack_context->item.type = CCPCP_ITEM_STRING;
			ccpcp_string *it = &unpack_context->item.as.String;
			ccpcp_string_init(it);
			it->parse_status.size_to_load = -1;
			unpack_string(unpack_context);
			break;
		}
		}
	}
}

