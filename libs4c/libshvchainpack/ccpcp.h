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
	CCPCP_RC_OK = 0,
	CCPCP_RC_MALLOC_ERROR = -1,
	CCPCP_RC_BUFFER_OVERFLOW = -2,
	CCPCP_RC_BUFFER_UNDERFLOW = -3,
	CCPCP_RC_MALFORMED_INPUT = -4,
	CCPCP_RC_LOGICAL_ERROR = -5,
	CCPCP_RC_CONTAINER_STACK_OVERFLOW = -6,
} ccpcp_error_codes;

//=========================== PACK ============================

struct ccpcp_pack_context;

typedef size_t (*ccpcp_pack_overflow_handler)(struct ccpcp_pack_context*, size_t);

typedef struct ccpcp_pack_context {
	uint8_t* start;
	uint8_t* current;
	uint8_t* end;
	const char *indent;
	int nest_count;
	int err_no; /* handlers can save error here */
	ccpcp_pack_overflow_handler handle_pack_overflow;
} ccpcp_pack_context;

void ccpcp_pack_context_init(ccpcp_pack_context* pack_context, void *data, size_t length, ccpcp_pack_overflow_handler hpo);

void ccpcp_pack_copy_bytes (ccpcp_pack_context* pack_context, const void *str, size_t len);

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
	//CCPCP_ITEM_BLOB,
	CCPCP_ITEM_LIST,
	CCPCP_ITEM_ARRAY,
	CCPCP_ITEM_MAP,
	CCPCP_ITEM_IMAP,
	CCPCP_ITEM_DATE_TIME,
	CCPCP_ITEM_META,
	CCPCP_ITEM_CONTAINER_END,
} ccpcp_item_types;

typedef struct {
	const uint8_t* start;
	size_t length;
	struct {
		long size_to_load;
		uint16_t chunk_cnt;
		uint8_t escaped_byte;
		uint8_t string_entered: 1;
		uint8_t last_chunk: 1;
	} parse_status;
} ccpcp_string;

void ccpcp_string_init(ccpcp_string *str_it);

typedef struct {
	ccpcp_item_types type;
	uint32_t size;
} ccpcp_array;

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
		ccpcp_array Array;
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

typedef int (*ccpcp_container_stack_overflow_handler)(struct ccpcp_container_stack*);

typedef struct ccpcp_container_stack {
	ccpcp_container_state *container_states;
	size_t length;
	size_t capacity;
	ccpcp_container_stack_overflow_handler overflow_handler;
} ccpcp_container_stack;

void ccpc_container_stack_init(ccpcp_container_stack* self, ccpcp_container_state *states, size_t capacity, ccpcp_container_stack_overflow_handler hnd);

struct ccpcp_unpack_context;

typedef size_t (*ccpcp_unpack_underflow_handler)(struct ccpcp_unpack_context*, size_t);

typedef struct ccpcp_unpack_context {
	ccpcp_item item;
	const uint8_t* start;
	const uint8_t* current;
	const uint8_t* end; /* logical end of buffer */
	int err_no; /* handlers can save error here */
	ccpcp_unpack_underflow_handler handle_unpack_underflow;
	ccpcp_container_stack *container_stack;
} ccpcp_unpack_context;

void ccpcp_unpack_context_init(ccpcp_unpack_context* self, const uint8_t* data, size_t length
							   , ccpcp_unpack_underflow_handler huu
							   , ccpcp_container_stack *stack);

ccpcp_container_state* ccpc_unpack_context_push_container_state(ccpcp_unpack_context* self, ccpcp_item_types container_type);
ccpcp_container_state* ccpc_unpack_context_last_container_state(ccpcp_unpack_context* unpack_context);
void ccpc_unpack_context_pop_container_state(ccpcp_unpack_context* self);

uint8_t* ccpcp_unpack_assert_byte(ccpcp_unpack_context* unpack_context);

#define UNPACK_ERROR(error_code)                        \
{                                                       \
    unpack_context->item.type = CCPCP_ITEM_INVALID;        \
    unpack_context->err_no = error_code;           \
    return;                                             \
}

#define UNPACK_ASSERT_BYTE()              \
{                                                       \
    p = ccpcp_unpack_assert_byte(unpack_context);        \
    if(!p)           \
        return;                                             \
}


#ifdef __cplusplus
}
#endif

#endif 
