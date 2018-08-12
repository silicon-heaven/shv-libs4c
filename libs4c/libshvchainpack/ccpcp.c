#include "ccpcp.h"

void ccpcp_pack_copy_bytes(ccpcp_pack_context *pack_context, const void *str, size_t len)
{
	size_t copied = 0;
	while (pack_context->err_no == CCPCP_RC_OK && copied < len) {
		uint8_t *p = ccpon_pack_reserve_space(pack_context, len);
		if(!p)
			break;
		size_t buff_size = pack_context->current - p;
		size_t rest = len - copied;
		if(rest > buff_size)
			rest = buff_size;
		memcpy(p, ((const char*)str) + copied, rest);
		copied += rest;
	}
}

uint8_t* ccpcp_unpack_assert_byte(ccpcp_unpack_context* unpack_context)
{
	size_t more = 1;
	uint8_t* p = unpack_context->current;
	uint8_t* nyp = p + more;
	if (nyp > unpack_context->end) {
		if (!unpack_context->handle_unpack_underflow) {
			unpack_context->err_no = CCPCP_RC_BUFFER_UNDERFLOW;
			//unpack_context->item.type = CCPCP_ITEM_INVALID;
			return NULL;
		}
		size_t sz = unpack_context->handle_unpack_underflow (unpack_context, more);
		if (sz < more) {
			unpack_context->err_no = CCPCP_RC_BUFFER_UNDERFLOW;
			//unpack_context->item.type = CCPCP_ITEM_INVALID;
			return NULL;
		}
		p = unpack_context->current;
		nyp = p + more;
	}
	unpack_context->current = nyp;
	return p;
}
