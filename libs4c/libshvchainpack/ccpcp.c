#ifdef BR_PLC
#include <bur/plctypes.h>
#ifdef __cplusplus
	extern "C"
	{
#endif
	#include "shv.h"
#ifdef __cplusplus
	};
#endif
#endif


#include "ccpcp.h"

#include <string.h>


#ifdef BR_PLC
/* TODO: Add your comment here */
void ccpcp(struct ccpcp* inst)
{
	/*TODO: Add your code here*/
}
#endif

const char *ccpcp_error_string(int err_no)
{
	switch (err_no) {
	case CCPCP_RC_OK: return "";
	case CCPCP_RC_MALLOC_ERROR: return "MALLOC_ERROR";
	case CCPCP_RC_BUFFER_OVERFLOW: return "BUFFER_OVERFLOW";
	case CCPCP_RC_BUFFER_UNDERFLOW: return "BUFFER_UNDERFLOW";
	case CCPCP_RC_MALFORMED_INPUT: return "MALFORMED_INPUT";
	case CCPCP_RC_LOGICAL_ERROR: return "LOGICAL_ERROR";
	case CCPCP_RC_CONTAINER_STACK_OVERFLOW: return "CONTAINER_STACK_OVERFLOW";
	case CCPCP_RC_CONTAINER_STACK_UNDERFLOW: return "CONTAINER_STACK_UNDERFLOW";
	default: return "UNKNOWN";
	}
}

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
const char *ccpcp_item_type_to_string(ccpcp_item_types t)
{
	switch(t) {
	case CCPCP_ITEM_INVALID: break;
	case CCPCP_ITEM_NULL: return "NULL";
	case CCPCP_ITEM_BOOLEAN: return "BOOLEAN";
	case CCPCP_ITEM_INT: return "INT";
	case CCPCP_ITEM_UINT: return "UINT";
	case CCPCP_ITEM_DOUBLE: return "DOUBLE";
	case CCPCP_ITEM_DECIMAL: return "DECIMAL";
	case CCPCP_ITEM_BLOB: return "BLOB";
	case CCPCP_ITEM_STRING: return "STRING";
	case CCPCP_ITEM_DATE_TIME: return "DATE_TIME";
	case CCPCP_ITEM_LIST: return "LIST";
	case CCPCP_ITEM_MAP: return "MAP";
	case CCPCP_ITEM_IMAP: return "IMAP";
	case CCPCP_ITEM_META: return "META";
	case CCPCP_ITEM_CONTAINER_END: return "CONTAINER_END";
	}
	return "INVALID";
}

void ccpcp_container_state_init(ccpcp_container_state *self, ccpcp_item_types cont_type)
{
	self->container_type = cont_type;
	self->current_item_type = CCPCP_ITEM_INVALID;
	self->item_count = 0;
	//self->custom_context = NULL;
}

void ccpcp_container_stack_init(ccpcp_container_stack *self, ccpcp_container_state *states, size_t capacity, ccpcp_container_stack_overflow_handler hnd)
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
	self->parser_line_no = 1;
	self->err_msg = "";
	self->handle_unpack_underflow = huu;
	self->custom_context = NULL;
	self->container_stack = stack;
	self->string_chunk_buff = self->default_string_chunk_buff;
	self->string_chunk_buff_len = sizeof(self->default_string_chunk_buff);
}

ccpcp_container_state *ccpcp_unpack_context_push_container_state(ccpcp_unpack_context *self, ccpcp_item_types container_type)
{
	if(!self->container_stack) {
		// C++ implementation does not require container states stack
		//self->err_no = CCPCP_RC_CONTAINER_STACK_OVERFLOW;
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

ccpcp_container_state* ccpcp_unpack_context_top_container_state(ccpcp_unpack_context* self)
{
	if(self->container_stack && self->container_stack->length > 0) {
		return self->container_stack->container_states + self->container_stack->length - 1;
	}
	return NULL;
}

ccpcp_container_state *ccpcp_unpack_context_parent_container_state(ccpcp_unpack_context *self)
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

ccpcp_container_state *ccpcp_unpack_context_closed_container_state(ccpcp_unpack_context *self)
{
	if(self->container_stack && self->item.type == CCPCP_ITEM_CONTAINER_END) {
		ccpcp_container_state *st = self->container_stack->container_states + self->container_stack->length;
		return st;
	}
	return NULL;
}

void ccpcp_unpack_context_pop_container_state(ccpcp_unpack_context* self)
{
	if(self->container_stack && self->container_stack->length > 0) {
		self->container_stack->length--;
	}
}

void ccpcp_string_init(ccpcp_string *self, ccpcp_unpack_context* unpack_context)
{
	self->chunk_size = 0;
	self->chunk_cnt = 0;
	self->last_chunk = 0;
	self->string_size = -1;
	self->size_to_load = -1;
	self->chunk_start = unpack_context->string_chunk_buff;
	self->chunk_buff_len = unpack_context->string_chunk_buff_len;
	self->blob_hex = 0;
}

const char* ccpcp_unpack_take_byte(ccpcp_unpack_context* unpack_context)
{
	const char* p = ccpcp_unpack_peek_byte(unpack_context);
	if(p)
		unpack_context->current++;
	return p;
}

const char *ccpcp_unpack_peek_byte(ccpcp_unpack_context *unpack_context)
{
	static const size_t more = 1;
	if (unpack_context->current >= unpack_context->end) {
		if (!unpack_context->handle_unpack_underflow) {
			unpack_context->err_no = CCPCP_RC_BUFFER_UNDERFLOW;
			return NULL;
		}
		size_t sz = unpack_context->handle_unpack_underflow (unpack_context);
		if (sz < more) {
			unpack_context->err_no = CCPCP_RC_BUFFER_UNDERFLOW;
			return NULL;
		}
		unpack_context->current = unpack_context->start;
	}
	const char* p = unpack_context->current;
	return p;
}

/*
bool ccpcp_item_is_string_unfinished(ccpcp_unpack_context *unpack_context)
{
	bool is_string_concat = false;
	ccpcp_container_state *parent_state = ccpcp_unpack_context_parent_container_state(unpack_context);
	if(parent_state != NULL) {
		if(unpack_context->item.type == CCPCP_ITEM_STRING) {
			ccpcp_string *it = &(unpack_context->item.as.String);
			if(it->chunk_cnt > 1) {
				// multichunk string
				// this can happen, when parsed string is greater than unpack_context buffer
				// or escape sequence is encountered
				// concatenate it with previous chunk
				is_string_concat = true;
			}
		}
	}
	return is_string_concat;
}

bool ccpcp_item_is_list_item(ccpcp_unpack_context *unpack_context)
{
	ccpcp_container_state *parent_state = ccpcp_unpack_context_parent_container_state(unpack_context);
	if(parent_state != NULL) {
		bool is_string_concat = 0;
		if(unpack_context->item.type == CCPCP_ITEM_STRING) {
			ccpcp_string *it = &(unpack_context->item.as.String);
			if(it->chunk_cnt > 1) {
				// multichunk string
				// this can happen, when parsed string is greater than unpack_context buffer
				// or escape sequence is encountered
				// concatenate it with previous chunk
				is_string_concat = 1;
			}
		}
		if(!is_string_concat && unpack_context->item.type != CCPCP_ITEM_CONTAINER_END) {
			switch(parent_state->container_type) {
			case CCPCP_ITEM_LIST:
			//case CCPCP_ITEM_ARRAY:
				if(!meta_just_closed)
					ccpon_pack_field_delim(out_ctx, parent_state->item_count == 1);
				break;
			case CCPCP_ITEM_MAP:
			case CCPCP_ITEM_IMAP:
			case CCPCP_ITEM_META: {
				bool is_key = (parent_state->item_count % 2);
				if(is_key) {
					if(!meta_just_closed)
						ccpon_pack_field_delim(out_ctx, parent_state->item_count == 1);
				}
				else {
					// delimite value
					ccpon_pack_key_val_delim(out_ctx);
				}
				break;
			}
			default:
				break;
			}
		}
	}
}

bool ccpcp_item_is_map_key(ccpcp_unpack_context *unpack_context)
{

}

bool ccpcp_item_is_map_val(ccpcp_unpack_context *unpack_context)
{

}
*/

double ccpcp_exponentional_to_double(int64_t const mantisa, const int exponent, const int base)
{
	double d = mantisa;
	int i;
	for (i = 0; i < exponent; ++i)
		d *= base;
	for (i = exponent; i < 0; ++i)
		d /= base;
	return d;
}

double ccpcp_decimal_to_double(const int64_t mantisa, const int exponent)
{
	return ccpcp_exponentional_to_double(mantisa, exponent, 10);
}

static int int_to_str(char *buff, size_t buff_len, int64_t val)
{
	int n = 0;
	bool neg = false;
	char *str = buff;
	int i;
	if(val < 0) {
		neg = true;
		val = -val;
		str = buff + 1;
		buff_len--;
	}
	if(val == 0) {
		if((size_t)n == buff_len)
			return -1;
		str[n++] = '0';
	}
	else while(val != 0) {
		int d = val % 10;
		val /= 10;
		if((size_t)n == buff_len)
			return -1;
		str[n++] = '0' + (char)d;
	}
	for (i = 0; i < n/2; ++i) {
		char c = str[i];
		str[i] = str[n - i - 1];
		str[n - i - 1] = c;
	}
	if(neg) {
		buff[0] = '-';
		n++;
	}
	return n;
}

int ccpcp_decimal_to_string(char *buff, size_t buff_len, int64_t mantisa, int exponent)
{
	bool neg = false;
	int i;
	if(mantisa < 0) {
		mantisa = -mantisa;
		neg = true;
	}

	// at least 21 characters for 64-bit types.
	char *str = buff;
	if(neg) {
		str++;
		buff_len--;
	}
	//const char *fmt = sizeof(long long) == sizeof (int64_t)? "%lld": "%ld";
	int n = int_to_str(str, buff_len, mantisa);
	if(n < 0) {
		return n;
	}

	int dec_places = -exponent;
	if(dec_places > 0 && dec_places < n) {
		int dot_ix = n - dec_places;
		for (i = dot_ix; i < n; ++i)
			str[n + dot_ix - i] = str[n + dot_ix - i-1];
		str[dot_ix] = '.';
		n++;
	}
	else if(dec_places > 0 && dec_places <= 3) {
		//ret = "0." + std::string(dec_places - ret.length(), '0') + ret;
		int extra_0_cnt = dec_places - n;
		for (i = 0; i < n; ++i)
			str[n - i - 1 + extra_0_cnt + 2] = str[n - i - 1];
		str[0] = '0';
		str[1] = '.';
		for (i = 0; i < extra_0_cnt; ++i)
			str[2 + i] = '0';
		n += extra_0_cnt + 2;
	}
	else if(dec_places < 0 && n + exponent <= 9) {
		for (i = 0; i < exponent; ++i)
			str[n++] = '0';
		str[n++] = '.';
	}
	else if(dec_places == 0) {
		str[n++] = '.';
	}
	else {
		str[n++] = 'e';
		int n2 = int_to_str(str+n, buff_len-n, exponent);
		if(n2 < 0) {
			return n2;
		}
		n += n2;
	}
	if(neg) {
		buff[0] = '-';
		n++;
	}
	return n;
}

