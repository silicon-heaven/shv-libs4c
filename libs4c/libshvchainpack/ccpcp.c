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
		self->container_stack->length++;
		return self->container_stack->container_states + self->container_stack->length - 1;
	}
	self->err_no = CCPCP_RC_CONTAINER_STACK_OVERFLOW;
	return NULL;
}

ccpcp_container_state* ccpc_unpack_context_last_container_state(ccpcp_unpack_context* self)
{
	if(self->container_stack && self->container_stack->length > 0) {
		return self->container_stack->container_states + self->container_stack->length - 1;
	}
	self->err_no = CCPCP_RC_CONTAINER_STACK_OVERFLOW;
	return NULL;
}

void ccpc_unpack_context_pop_container_state(ccpcp_unpack_context* self)
{
	if(self->container_stack && self->container_stack->length > 0) {
		self->container_stack->length--;
	}
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

uint8_t* ccpcp_unpack_assert_byte(ccpcp_unpack_context* unpack_context)
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


