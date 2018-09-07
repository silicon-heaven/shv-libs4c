#include "cchainpack.h"
#include "ccpon.h"

void ccpcp_convert(ccpcp_unpack_context* in_ctx, ccpcp_pack_format in_format, ccpcp_pack_context* out_ctx, ccpcp_pack_format out_format)
{
	bool o_cpon_input = (in_format == CCPCP_Cpon);
	bool o_chainpack_output = (out_format == CCPCP_ChainPack);
	int prev_item = CCPCP_ITEM_INVALID;
	do {
		if(o_cpon_input)
			ccpon_unpack_next(in_ctx);
		else
			cchainpack_unpack_next(in_ctx);
		if(in_ctx->err_no != CCPCP_RC_OK)
			break;

		ccpcp_container_state *curr_item_cont_state = ccpc_unpack_context_current_item_container_state(in_ctx);
		if(o_chainpack_output) {
			if(curr_item_cont_state && curr_item_cont_state->current_item_is_key) {
				if(in_ctx->item.type == CCPCP_ITEM_STRING) {
					ccpcp_string *it = &(in_ctx->item.as.String);
					if(!(it->chunk_cnt == 1 && it->last_chunk)) {
						// string key cannot be multichunk
						in_ctx->err_no = CCPCP_RC_LOGICAL_ERROR;
						return;
					}
				}
				switch(curr_item_cont_state->container_type) {
				case CCPCP_ITEM_MAP: {
					if(in_ctx->item.type == CCPCP_ITEM_STRING) {
						ccpcp_string *it = &(in_ctx->item.as.String);
						cchainpack_pack_string_key(out_ctx, it->chunk_start, it->chunk_size);
					}
					else {
						// invalid key type
						in_ctx->err_no = CCPCP_RC_LOGICAL_ERROR;
					}
					break;
				}
				case CCPCP_ITEM_IMAP: {
					if(in_ctx->item.type == CCPCP_ITEM_INT) {
						cchainpack_pack_uint_key(out_ctx, (uint64_t)in_ctx->item.as.Int);
					}
					else if(in_ctx->item.type == CCPCP_ITEM_UINT) {
						cchainpack_pack_uint_key(out_ctx, in_ctx->item.as.UInt);
					}
					else {
						// invalid key type
						in_ctx->err_no = CCPCP_RC_LOGICAL_ERROR;
					}
					break;
				}
				case CCPCP_ITEM_META: {
					if(in_ctx->item.type == CCPCP_ITEM_STRING) {
						ccpcp_string *it = &(in_ctx->item.as.String);
						cchainpack_pack_string_key_meta(out_ctx, it->chunk_start, it->chunk_size);
					}
					else if(in_ctx->item.type == CCPCP_ITEM_INT) {
						cchainpack_pack_uint_key(out_ctx, (uint64_t)in_ctx->item.as.Int);
					}
					else if(in_ctx->item.type == CCPCP_ITEM_UINT) {
						cchainpack_pack_uint_key(out_ctx, in_ctx->item.as.UInt);
					}
					else {
						// invalid key type
						in_ctx->err_no = CCPCP_RC_LOGICAL_ERROR;
					}
					break;
				}
				default:
					in_ctx->err_no = CCPCP_RC_LOGICAL_ERROR;
					break;
				}
				continue;
			}
		}
		else {
			if(curr_item_cont_state != NULL) {
				bool is_string_concat = 0;
				if(in_ctx->item.type == CCPCP_ITEM_STRING) {
					ccpcp_string *it = &(in_ctx->item.as.String);
					if(it->chunk_cnt > 1) {
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
					//case CCPCP_ITEM_ARRAY:
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
							// pack key
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
		/*
		case CCPCP_ITEM_ARRAY: {
			if(o_chainpack_output)
				cchainpack_pack_array_begin(out_ctx, in_ctx->item.as.Array.size);
			else
				ccpon_pack_array_begin(out_ctx, in_ctx->item.as.Array.size);
			break;
		}
		*/
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
				cchainpack_pack_container_end(out_ctx);
			else
				ccpon_pack_list_end(out_ctx);
			break;
		}
		/*
		case CCPCP_ITEM_ARRAY_END: {
			if(o_chainpack_output)
				;
			else
				ccpon_pack_list_end(out_ctx);
			break;
		}
		*/
		case CCPCP_ITEM_MAP_END: {
			if(o_chainpack_output)
				cchainpack_pack_container_end(out_ctx);
			else
				ccpon_pack_map_end(out_ctx);
			break;
		}
		case CCPCP_ITEM_IMAP_END: {
			if(o_chainpack_output)
				cchainpack_pack_container_end(out_ctx);
			else
				ccpon_pack_imap_end(out_ctx);
			break;
		}
		case CCPCP_ITEM_META_END: {
			if(o_chainpack_output)
				cchainpack_pack_container_end(out_ctx);
			else
				ccpon_pack_meta_end(out_ctx);
			break;
		}
		case CCPCP_ITEM_STRING: {
			ccpcp_string *it = &in_ctx->item.as.String;
			if(o_chainpack_output) {
				if(it->chunk_cnt == 1 && it->last_chunk) {
					// one chunk string with known length is always packed as RAW
					cchainpack_pack_string(out_ctx, it->chunk_start, it->chunk_size);
				}
				else if(it->string_size >= 0) {
					if(it->chunk_cnt == 1)
						cchainpack_pack_string_start(out_ctx, it->string_size, it->chunk_start, it->string_size);
					else
						cchainpack_pack_string_cont(out_ctx, it->chunk_start, it->chunk_size);
				}
				else {
					// cstring
					if(it->chunk_cnt == 1)
						cchainpack_pack_cstring_start(out_ctx, it->chunk_start, it->chunk_size);
					else
						cchainpack_pack_cstring_cont(out_ctx, it->chunk_start, it->chunk_size);
					if(it->last_chunk)
						cchainpack_pack_cstring_finish(out_ctx);
				}
			}
			else {
				// Cpon
				if(it->chunk_cnt == 1)
					ccpon_pack_string_start(out_ctx, it->chunk_start, it->chunk_size);
				else
					ccpon_pack_string_cont(out_ctx, it->chunk_start, it->chunk_size);
				if(it->last_chunk)
					ccpon_pack_string_finish(out_ctx);
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
			if(!top_state) {
				if(in_ctx->item.type == CCPCP_ITEM_STRING && !in_ctx->item.as.String.last_chunk) {
					// do not stop parsing in the middle of the string
				}
				else {
					break;
				}
			}
		}
	} while(in_ctx->err_no == CCPCP_RC_OK && out_ctx->err_no == CCPCP_RC_OK);

	if(out_ctx->handle_pack_overflow)
		out_ctx->handle_pack_overflow(out_ctx, 0);
}
