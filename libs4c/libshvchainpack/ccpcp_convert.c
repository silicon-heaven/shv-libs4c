#include "cchainpack.h"
#include "ccpon.h"

void ccpcp_convert(ccpcp_unpack_context* in_ctx, ccpcp_pack_format in_format, ccpcp_pack_context* out_ctx, ccpcp_pack_format out_format)
{
	bool o_cpon_input = in_format == CCPCP_Cpon;
	bool o_chainpack_output = out_format == CCPCP_ChainPack;
	int prev_item = CCPCP_ITEM_INVALID;
	do {
		if(o_cpon_input)
			ccpon_unpack_next(in_ctx);
		else
			cchainpack_unpack_next(in_ctx);
		if(in_ctx->err_no != CCPCP_RC_OK)
			break;

		if(!o_chainpack_output) {
			ccpcp_container_state *curr_item_cont_state = ccpc_unpack_context_current_item_container_state(in_ctx);
			if(curr_item_cont_state != NULL) {
				bool is_string_concat = 0;
				if(in_ctx->item.type == CCPCP_ITEM_STRING || in_ctx->item.type == CCPCP_ITEM_CSTRING) {
					ccpcp_string *it = &(in_ctx->item.as.String);
					if(it->parse_status.chunk_cnt > 1) {
						// multichunk string
						// this can happen, when parsed string is greater than unpack_context buffer
						// or escape sequence is encountered
						// concatenate it with previous chunk
						is_string_concat = 1;
					}
				}
				if(!is_string_concat && !ccpcp_item_type_is_container_end(in_ctx->item.type)) {
					switch(curr_item_cont_state->container_type) {
					case CCPCP_ITEM_LIST:
					case CCPCP_ITEM_ARRAY:
						if(prev_item != CCPCP_ITEM_META_END)
							ccpon_pack_field_delim(out_ctx, curr_item_cont_state->item_count == 1);
						break;
					case CCPCP_ITEM_MAP:
					case CCPCP_ITEM_IMAP:
					case CCPCP_ITEM_META: {
						bool is_val = (curr_item_cont_state->item_count % 2) == 0;
						if(is_val) {
							ccpon_pack_key_delim(out_ctx);
						}
						else {
							if(prev_item != CCPCP_ITEM_META_END)
								ccpon_pack_field_delim(out_ctx, curr_item_cont_state->item_count == 1);
						}
						break;
					}
					default:
						break;
					}
				}
			}
		}
		switch(in_ctx->item.type) {
		case CCPCP_ITEM_INVALID: {
			// end of input
			break;
		}
		case CCPCP_ITEM_LIST: {
			if(o_chainpack_output)
				cchainpack_pack_list_begin(out_ctx);
			else
				ccpon_pack_list_begin(out_ctx);
			break;
		}
		case CCPCP_ITEM_ARRAY: {
			if(o_chainpack_output)
				cchainpack_pack_array_begin(out_ctx, in_ctx->item.as.Array.size);
			else
				ccpon_pack_array_begin(out_ctx, in_ctx->item.as.Array.size);
			break;
		}
		case CCPCP_ITEM_MAP: {
			if(o_chainpack_output)
				cchainpack_pack_map_begin(out_ctx);
			else
				ccpon_pack_map_begin(out_ctx);
			break;
		}
		case CCPCP_ITEM_IMAP: {
			if(o_chainpack_output)
				cchainpack_pack_imap_begin(out_ctx);
			else
				ccpon_pack_imap_begin(out_ctx);
			break;
		}
		case CCPCP_ITEM_META: {
			if(o_chainpack_output)
				cchainpack_pack_meta_begin(out_ctx);
			else
				ccpon_pack_meta_begin(out_ctx);
			break;
		}
		case CCPCP_ITEM_LIST_END: {
			if(o_chainpack_output)
				cchainpack_pack_list_end(out_ctx);
			else
				ccpon_pack_list_end(out_ctx);
			break;
		}
		case CCPCP_ITEM_ARRAY_END: {
			if(o_chainpack_output)
				;
			else
				ccpon_pack_list_end(out_ctx);
			break;
		}
		case CCPCP_ITEM_MAP_END: {
			if(o_chainpack_output)
				cchainpack_pack_map_end(out_ctx);
			else
				ccpon_pack_map_end(out_ctx);
			break;
		}
		case CCPCP_ITEM_IMAP_END: {
			if(o_chainpack_output)
				cchainpack_pack_imap_end(out_ctx);
			else
				ccpon_pack_imap_end(out_ctx);
			break;
		}
		case CCPCP_ITEM_META_END: {
			if(o_chainpack_output)
				cchainpack_pack_meta_end(out_ctx);
			else
				ccpon_pack_meta_end(out_ctx);
			break;
		}
		case CCPCP_ITEM_STRING: {
			ccpcp_string *it = &in_ctx->item.as.String;
			if(o_chainpack_output) {
				if(it->parse_status.chunk_cnt == 1) {
					// first string chunk
					cchainpack_pack_uint(out_ctx, it->length);
					ccpcp_pack_copy_bytes(out_ctx, it->start, it->length);
				}
				else {
					// next string chunk
					ccpcp_pack_copy_bytes(out_ctx, it->start, it->length);
				}
			}
			else {
				if(it->parse_status.chunk_cnt == 1) {
					// first string chunk
					ccpcp_pack_copy_bytes(out_ctx, "\"", 1);
					ccpcp_pack_copy_bytes(out_ctx, it->start, it->length);
				}
				else {
					// next string chunk
					ccpcp_pack_copy_bytes(out_ctx, it->start, it->length);
				}
				if(it->parse_status.last_chunk)
					ccpcp_pack_copy_bytes(out_ctx, "\"", 1);
			}
			break;
		}
		case CCPCP_ITEM_BOOLEAN: {
			if(o_chainpack_output)
				cchainpack_pack_boolean(out_ctx, in_ctx->item.as.Bool);
			else
				ccpon_pack_boolean(out_ctx, in_ctx->item.as.Bool);
			break;
		}
		case CCPCP_ITEM_INT: {
			if(o_chainpack_output)
				cchainpack_pack_int(out_ctx, in_ctx->item.as.Int);
			else
				ccpon_pack_int(out_ctx, in_ctx->item.as.Int);
			break;
		}
		case CCPCP_ITEM_UINT: {
			if(o_chainpack_output)
				cchainpack_pack_uint(out_ctx, in_ctx->item.as.UInt);
			else
				ccpon_pack_uint(out_ctx, in_ctx->item.as.UInt);
			break;
		}
		case CCPCP_ITEM_DECIMAL: {
			if(o_chainpack_output)
				cchainpack_pack_decimal(out_ctx, in_ctx->item.as.Decimal.mantisa, in_ctx->item.as.Decimal.dec_places);
			else
				ccpon_pack_decimal(out_ctx, in_ctx->item.as.Decimal.mantisa, in_ctx->item.as.Decimal.dec_places);
			break;
		}
		case CCPCP_ITEM_DOUBLE: {
			ccpon_pack_double(out_ctx, in_ctx->item.as.Double);
			break;
		}
		case CCPCP_ITEM_DATE_TIME: {
			ccpcp_date_time *it = &in_ctx->item.as.DateTime;
			if(o_chainpack_output)
				cchainpack_pack_date_time(out_ctx, it->msecs_since_epoch, it->minutes_from_utc);
			else
				ccpon_pack_date_time(out_ctx, it->msecs_since_epoch, it->minutes_from_utc);
			break;
		}
		default:
			if(o_chainpack_output)
				cchainpack_pack_null(out_ctx);
			else
				ccpon_pack_null(out_ctx);
			break;
		}
		prev_item = in_ctx->item.type;
		{
			ccpcp_container_state *top_state = ccpc_unpack_context_top_container_state(in_ctx);
			// take just one object from stream
			if(!top_state)
				break;
		}
	} while(in_ctx->err_no == CCPCP_RC_OK && out_ctx->err_no == CCPCP_RC_OK);

	if(out_ctx->handle_pack_overflow)
		out_ctx->handle_pack_overflow(out_ctx);
}
