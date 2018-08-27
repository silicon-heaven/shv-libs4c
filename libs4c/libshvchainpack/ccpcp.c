#include "ccpcp.h"

uint8_t* ccpcp_pack_reserve_space(ccpcp_pack_context* pack_context, size_t more)
{
	uint8_t* p = pack_context->current;
	uint8_t* nyp = p + more;
	if (nyp > pack_context->end) {
		if (!pack_context->handle_pack_overflow) {
			pack_context->err_no = CCPCP_RC_BUFFER_OVERFLOW;
			return NULL;
		}
		size_t sz = pack_context->handle_pack_overflow (pack_context, more);
		if (sz < more) {
			pack_context->err_no = CCPCP_RC_BUFFER_OVERFLOW;
			return NULL;
		}
		p = pack_context->current;
		nyp = p + more;
	}
	pack_context->current = nyp;
	return p;
}

void ccpcp_pack_copy_bytes(ccpcp_pack_context *pack_context, const void *str, size_t len)
{
	size_t copied = 0;
	while (pack_context->err_no == CCPCP_RC_OK && copied < len) {
		uint8_t *p = ccpcp_pack_reserve_space(pack_context, len);
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

//================================ UNPACK ================================

void ccpcp_container_state_init(ccpcp_container_state *self, ccpcp_item_types cont_type)
{
	self->container_type = cont_type;
	self->container_size = 0;
	self->item_count = 0;
}

void ccpc_container_stack_init(ccpcp_container_stack *self, ccpcp_container_state *states, size_t capacity, ccpcp_container_stack_overflow_handler hnd)
{
	self->container_states = states;
	self->capacity = capacity;
	self->length = 0;
	self->overflow_handler = hnd;
}

void ccpcp_unpack_context_init (ccpcp_unpack_context* self, const uint8_t *data, size_t length, ccpcp_unpack_underflow_handler huu, ccpcp_container_stack *stack)
{
	self->start = self->current = data;
	self->end = self->start + length;
	self->err_no = CCPCP_RC_OK;
	self->handle_unpack_underflow = huu;
	self->container_stack = stack;
}

ccpcp_container_state *ccpc_unpack_context_push_container_state(ccpcp_unpack_context *self, ccpcp_item_types container_type)
{
	if(!self->container_stack) {
		self->err_no = CCPCP_RC_CONTAINER_STACK_OVERFLOW;
		return NULL;
	}
	if(self->container_stack->length == self->container_stack->capacity) {
		if(self->container_stack->overflow_handler) {
			self->err_no = CCPCP_RC_CONTAINER_STACK_OVERFLOW;
			return NULL;
		}
		int rc = self->container_stack->overflow_handler(self->container_stack);
		if(rc < 0) {
			self->err_no = CCPCP_RC_CONTAINER_STACK_OVERFLOW;
			return NULL;
		}
	}
	if(self->container_stack->length < self->container_stack->capacity-1) {
		ccpcp_container_state *state = self->container_stack->container_states + self->container_stack->length;
		ccpcp_container_state_init(state, container_type);
		self->container_stack->length++;
		return state;
	}
	self->err_no = CCPCP_RC_CONTAINER_STACK_OVERFLOW;
	return NULL;
}

ccpcp_container_state* ccpc_unpack_context_top_container_state(ccpcp_unpack_context* self)
{
	if(self->container_stack && self->container_stack->length > 0) {
		return self->container_stack->container_states + self->container_stack->length - 1;
	}
	return NULL;
}

ccpcp_container_state *ccpc_unpack_context_subtop_container_state(ccpcp_unpack_context *self)
{
	if(self->container_stack && self->container_stack->length > 1) {
		return self->container_stack->container_states + self->container_stack->length - 2;
	}
	return NULL;
}

void ccpc_unpack_context_pop_container_state(ccpcp_unpack_context* self)
{
	if(self->container_stack && self->container_stack->length > 0) {
		self->container_stack->length--;
	}
}

bool ccpcp_item_type_is_value(ccpcp_item_types t)
{
	switch(t) {
	case CCPCP_ITEM_INVALID:
		return false;
	case CCPCP_ITEM_LIST:
	case CCPCP_ITEM_ARRAY:
	case CCPCP_ITEM_MAP:
	case CCPCP_ITEM_IMAP:
	case CCPCP_ITEM_META:
	case CCPCP_ITEM_LIST_END:
	case CCPCP_ITEM_ARRAY_END:
	case CCPCP_ITEM_MAP_END:
	case CCPCP_ITEM_IMAP_END:
	case CCPCP_ITEM_META_END:
		return false;
	case CCPCP_ITEM_NULL:
	case CCPCP_ITEM_BOOLEAN:
	case CCPCP_ITEM_INT:
	case CCPCP_ITEM_UINT:
	case CCPCP_ITEM_DOUBLE:
	case CCPCP_ITEM_DECIMAL:
	case CCPCP_ITEM_DATE_TIME:
	case CCPCP_ITEM_STRING:
		return true;
	}
	return false;
}

bool ccpcp_item_type_is_container_end(ccpcp_item_types t)
{
	switch(t) {
	case CCPCP_ITEM_INVALID:
		return false;
	case CCPCP_ITEM_LIST:
	case CCPCP_ITEM_ARRAY:
	case CCPCP_ITEM_MAP:
	case CCPCP_ITEM_IMAP:
	case CCPCP_ITEM_META:
	case CCPCP_ITEM_NULL:
	case CCPCP_ITEM_BOOLEAN:
	case CCPCP_ITEM_INT:
	case CCPCP_ITEM_UINT:
	case CCPCP_ITEM_DOUBLE:
	case CCPCP_ITEM_DECIMAL:
	case CCPCP_ITEM_DATE_TIME:
	case CCPCP_ITEM_STRING:
		return false;
	case CCPCP_ITEM_LIST_END:
	case CCPCP_ITEM_ARRAY_END:
	case CCPCP_ITEM_MAP_END:
	case CCPCP_ITEM_IMAP_END:
	case CCPCP_ITEM_META_END:
		return true;
	}
	return false;
}

void ccpcp_string_init(ccpcp_string *str_it)
{
	str_it->start = NULL;
	str_it->length = 0;
	//str_it->parse_status.begin = 0;
	//str_it->parse_status.middle = 0;
	str_it->parse_status.chunk_cnt = 0;
	str_it->parse_status.last_chunk = 0;
	str_it->parse_status.size_to_load = -1;
	str_it->parse_status.string_entered = 0;
}

const uint8_t* ccpcp_unpack_assert_byte(ccpcp_unpack_context* unpack_context)
{
	size_t more = 1;
	const uint8_t* p = unpack_context->current;
	const uint8_t* nyp = p + more;
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
