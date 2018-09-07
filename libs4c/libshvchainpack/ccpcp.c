#include "ccpcp.h"

#include <string.h>

size_t ccpcp_pack_make_space(ccpcp_pack_context* pack_context, size_t size_hint)
{
	if(pack_context->err_no != CCPCP_RC_OK)
		return 0;
	size_t free_space = pack_context->end - pack_context->current;
	if(free_space < size_hint) {
		if (!pack_context->handle_pack_overflow) {
			pack_context->err_no = CCPCP_RC_BUFFER_OVERFLOW;
			return 0;
		}
		pack_context->handle_pack_overflow (pack_context, size_hint);
		free_space = pack_context->end - pack_context->current;
		if (free_space < 1) {
			pack_context->err_no = CCPCP_RC_BUFFER_OVERFLOW;
			return 0;
		}
	}
	return free_space;
}

char* ccpcp_pack_reserve_space(ccpcp_pack_context* pack_context, size_t more)
{
	if(pack_context->err_no != CCPCP_RC_OK)
		return NULL;
	size_t free_space = ccpcp_pack_make_space(pack_context, more);
	if (free_space < more) {
		pack_context->err_no = CCPCP_RC_BUFFER_OVERFLOW;
		return NULL;
	}
	char* p = pack_context->current;
	pack_context->current = p + more;
	return p;
}

void ccpcp_pack_copy_byte(ccpcp_pack_context *pack_context, uint8_t b)
{
	char *p = ccpcp_pack_reserve_space(pack_context, 1);
	if(!p)
		return;
	*p = b;
}

void ccpcp_pack_copy_bytes(ccpcp_pack_context *pack_context, const void *str, size_t len)
{
	size_t copied = 0;
	while (pack_context->err_no == CCPCP_RC_OK && copied < len) {
		size_t buff_size = ccpcp_pack_make_space(pack_context, len);
		if(buff_size == 0) {
			pack_context->err_no = CCPCP_RC_BUFFER_OVERFLOW;
			return;
		}
		size_t rest = len - copied;
		if(rest > buff_size)
			rest = buff_size;
		memcpy(pack_context->current, ((const char*)str) + copied, rest);
		copied += rest;
		pack_context->current += rest;
	}
}

//================================ UNPACK ================================

void ccpcp_container_state_init(ccpcp_container_state *self, ccpcp_item_types cont_type)
{
	self->container_type = cont_type;
	self->container_size = 0;
	self->item_count = 0;
	//self->current_item_is_key = 0;
}

void ccpc_container_stack_init(ccpcp_container_stack *self, ccpcp_container_state *states, size_t capacity, ccpcp_container_stack_overflow_handler hnd)
{
	self->container_states = states;
	self->capacity = capacity;
	self->length = 0;
	self->overflow_handler = hnd;
}

void ccpcp_unpack_context_init (ccpcp_unpack_context* self, const void *data, size_t length, ccpcp_unpack_underflow_handler huu, ccpcp_container_stack *stack)
{
	self->item.type = CCPCP_ITEM_INVALID;
	self->start = self->current = (const char*)data;
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

ccpcp_container_state *ccpc_unpack_context_current_item_container_state(ccpcp_unpack_context *self)
{
	if(self->container_stack && self->container_stack->length > 0) {
		ccpcp_container_state *top_st = self->container_stack->container_states + self->container_stack->length - 1;
		if(top_st && top_st->item_count == 0) {
			if(self->container_stack->length > 1)
				return self->container_stack->container_states + self->container_stack->length - 2;
			else
				return NULL;
		}
		return top_st;
	}
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
	str_it->chunk_start = NULL;
	str_it->chunk_size = 0;
	str_it->chunk_cnt = 0;
	str_it->last_chunk = 0;
	str_it->string_size = -1;
	str_it->size_to_load = -1;
	str_it->chunk_start = str_it->default_chunk_buff;
	str_it->chunk_buff_len = sizeof(str_it->default_chunk_buff);
}

const char* ccpcp_unpack_take_byte(ccpcp_unpack_context* unpack_context)
{
	size_t more = 1;
	const char* p = unpack_context->current;
	const char* nyp = p + more;
	if (nyp > unpack_context->end) {
		if (!unpack_context->handle_unpack_underflow) {
			unpack_context->err_no = CCPCP_RC_BUFFER_UNDERFLOW;
			//unpack_context->item.type = CCPCP_ITEM_INVALID;
			return NULL;
		}
		size_t sz = unpack_context->handle_unpack_underflow (unpack_context);
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
/*
bool ccpcp_item_is_map_key(ccpcp_unpack_context *unpack_context)
{
	ccpcp_container_state *st = ccpc_unpack_context_current_item_container_state(unpack_context);
	if(st) {
		if(st->container_type == CCPCP_ITEM_MAP
				|| st->container_type == CCPCP_ITEM_IMAP
				|| st->container_type == CCPCP_ITEM_META)
		{
			return st->item_count % 2;
		}
	}
	return false;
}
*/
