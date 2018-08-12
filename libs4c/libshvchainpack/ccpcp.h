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
} ccpcp_error_codes;

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
		uint16_t chunk_cnt;
		char escape_seq[2];
		uint8_t string_entered: 1;
		uint8_t last_chunk: 1;
	} parse_status;
} ccpcp_string;

void ccpcp_string_init(ccpcp_string *str_it);

typedef struct {
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

struct ccpcp_unpack_context;

typedef size_t (*ccpcp_unpack_underflow_handler)(struct ccpcp_unpack_context*, size_t);

typedef struct ccpcp_unpack_context {
	ccpcp_item item;
	uint8_t* start;
	uint8_t* current;
	uint8_t* end; /* logical end of buffer */
	int err_no; /* handlers can save error here */
	ccpcp_unpack_underflow_handler handle_unpack_underflow;
} ccpcp_unpack_context;

uint8_t* ccpcp_unpack_assert_byte(ccpcp_unpack_context* unpack_context);

#ifdef __cplusplus
}
#endif

#endif 
