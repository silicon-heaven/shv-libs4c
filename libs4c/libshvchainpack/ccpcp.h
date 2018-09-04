#ifndef C_CPCP_H
#define C_CPCP_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	CCPCP_ChainPack = 1,
	CCPCP_Cpon,
} ccpcp_pack_format;

typedef enum
{
	CCPCP_RC_OK = 0,
	CCPCP_RC_MALLOC_ERROR = -1,
	CCPCP_RC_BUFFER_OVERFLOW = -2,
	CCPCP_RC_BUFFER_UNDERFLOW = -3,
	CCPCP_RC_MALFORMED_INPUT = -4,
	CCPCP_RC_LOGICAL_ERROR = -5,
	CCPCP_RC_CONTAINER_STACK_OVERFLOW = -6,
	CCPCP_RC_CONTAINER_STACK_UNDERFLOW = -7,
} ccpcp_error_codes;

//=========================== PACK ============================

struct ccpcp_pack_context;

typedef void (*ccpcp_pack_overflow_handler)(struct ccpcp_pack_context*, size_t size_hint);

typedef struct ccpcp_pack_context {
	char* start;
	char* current;
	char* end;
	const char *indent;
	int nest_count;
	int err_no; /* handlers can save error here */
	ccpcp_pack_overflow_handler handle_pack_overflow;
} ccpcp_pack_context;

void ccpcp_pack_context_init(ccpcp_pack_context* pack_context, void *data, size_t length, ccpcp_pack_overflow_handler hpo);

// try to make size_hint bytes space in pack_context
// returns number of bytes available in pack_context buffer, can be < size_hint, but always > 0
// returns 0 if fails
size_t ccpcp_pack_make_space(ccpcp_pack_context* pack_context, size_t size_hint);
char *ccpcp_pack_reserve_space(ccpcp_pack_context* pack_context, size_t more);
void ccpcp_pack_copy_byte (ccpcp_pack_context* pack_context, uint8_t b);
void ccpcp_pack_copy_bytes (ccpcp_pack_context* pack_context, const void *str, size_t len);
//void ccpcp_pack_copy_bytes_cpon_string_escaped (ccpcp_pack_context* pack_context, const void *str, size_t len);

//=========================== UNPACK ============================

typedef enum
{
	CCPCP_ITEM_INVALID = 0,
	CCPCP_ITEM_NULL,
	CCPCP_ITEM_BOOLEAN,
	CCPCP_ITEM_INT,
	CCPCP_ITEM_UINT,
	CCPCP_ITEM_DOUBLE,
	CCPCP_ITEM_DECIMAL,
	CCPCP_ITEM_STRING,
	//CCPCP_ITEM_CSTRING,
	//CCPCP_ITEM_BLOB,
	CCPCP_ITEM_DATE_TIME,

	CCPCP_ITEM_LIST,
	CCPCP_ITEM_LIST_END,
	//CCPCP_ITEM_ARRAY,
	//CCPCP_ITEM_ARRAY_END,
	CCPCP_ITEM_MAP,
	CCPCP_ITEM_MAP_END,
	CCPCP_ITEM_IMAP,
	CCPCP_ITEM_IMAP_END,
	CCPCP_ITEM_META,
	CCPCP_ITEM_META_END,
} ccpcp_item_types;

bool ccpcp_item_type_is_value(ccpcp_item_types t);
bool ccpcp_item_type_is_container_end(ccpcp_item_types t);

typedef struct {
	const char* start;
	long string_size;
	struct {
		size_t chunk_length;
		long size_to_load;
		uint16_t chunk_cnt;
		char escaped_byte;
		//uint8_t string_entered: 1;
		uint8_t last_chunk: 1;
	} parse_status;
} ccpcp_string;

void ccpcp_string_init(ccpcp_string *str_it);
/*
typedef struct {
	ccpcp_item_types type;
	int32_t size;
} ccpcp_array;
*/
typedef struct {
	int64_t msecs_since_epoch;
	int minutes_from_utc;
} ccpcp_date_time;

typedef struct {
	int64_t mantisa;
	int dec_places;
} ccpcp_decimal;

typedef struct {
	ccpcp_item_types type;
	union
	{
		ccpcp_string String;
		//ccpcp_string Blob;
		ccpcp_date_time DateTime;
		ccpcp_decimal Decimal;
		uint64_t UInt;
		int64_t Int;
		//ccpcp_array Array;
		double Double;
		bool Bool;
	} as;
} ccpcp_item;
/*
typedef enum {
	CPFLD_Field = 1,
	CPFLD_Key,
	CPFLD_Val
} ccpc_field_state;
*/
typedef struct {
	ccpcp_item_types container_type;
	//ccpc_field_state field_state;
	size_t item_count;
	size_t container_size;
} ccpcp_container_state;

void ccpcp_container_state_init(ccpcp_container_state *self, ccpcp_item_types cont_type);

struct ccpcp_container_stack;

typedef int (*ccpcp_container_stack_overflow_handler)(struct ccpcp_container_stack*);

typedef struct ccpcp_container_stack {
	ccpcp_container_state *container_states;
	size_t length;
	size_t capacity;
	ccpcp_container_stack_overflow_handler overflow_handler;
} ccpcp_container_stack;

void ccpc_container_stack_init(ccpcp_container_stack* self, ccpcp_container_state *states, size_t capacity, ccpcp_container_stack_overflow_handler hnd);

struct ccpcp_unpack_context;

typedef size_t (*ccpcp_unpack_underflow_handler)(struct ccpcp_unpack_context*);

typedef struct ccpcp_unpack_context {
	ccpcp_item item;
	const char* start;
	const char* current;
	const char* end; /* logical end of buffer */
	int err_no; /* handlers can save error here */
	ccpcp_unpack_underflow_handler handle_unpack_underflow;
	ccpcp_container_stack *container_stack;
} ccpcp_unpack_context;

void ccpcp_unpack_context_init(ccpcp_unpack_context* self, const void* data, size_t length
							   , ccpcp_unpack_underflow_handler huu
							   , ccpcp_container_stack *stack);

ccpcp_container_state* ccpc_unpack_context_push_container_state(ccpcp_unpack_context* self, ccpcp_item_types container_type);
ccpcp_container_state* ccpc_unpack_context_top_container_state(ccpcp_unpack_context* self);
ccpcp_container_state* ccpc_unpack_context_current_item_container_state(ccpcp_unpack_context* self);
void ccpc_unpack_context_pop_container_state(ccpcp_unpack_context* self);

const char *ccpcp_unpack_take_byte(ccpcp_unpack_context* unpack_context);

#define UNPACK_ERROR(error_code)                        \
{                                                       \
    unpack_context->item.type = CCPCP_ITEM_INVALID;        \
    unpack_context->err_no = error_code;           \
    return;                                             \
}

#define UNPACK_ASSERT_BYTE()              \
{                                                       \
    p = ccpcp_unpack_take_byte(unpack_context);        \
    if(!p)           \
        return;                                             \
}


#ifdef __cplusplus
}
#endif

#endif 
