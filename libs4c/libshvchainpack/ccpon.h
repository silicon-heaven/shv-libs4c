#ifndef C_CPON_H
#define C_CPON_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

//#ifndef ccpon_int
//#define ccpon_int int
//#endif

/******************************* Return Codes *****************************/

typedef enum
{
	CCPON_RC_OK = 0,
	CCPON_RC_MALLOC_ERROR = -1,
	CCPON_RC_BUFFER_OVERFLOW = -2,
	CCPON_RC_BUFFER_UNDERFLOW = -3,
	CCPON_RC_MALFORMED_INPUT = -4,
	CCPON_RC_LOGICAL_ERROR = -5,
	//#define CCPON_RC_END_OF_INPUT -1
	//#define CCPON_RC_ERROR_IN_HANDLER -6
	//#define CCPON_RC_ILLEGAL_CALL -7
	//#define CCPON_RC_STOPPED -9
} ccpon_error_codes;


time_t ccpon_timegm(struct tm *tm);
void ccpon_gmtime(int64_t epoch_sec, struct tm *tm);

/******************************* P A C K **********************************/

struct ccpon_pack_context;

typedef size_t (*ccpon_pack_overflow_handler)(struct ccpon_pack_context*, size_t);

typedef struct ccpon_pack_context {
	uint8_t* start;
	uint8_t* current;
	uint8_t* end;
	const char *indent;
	int nest_count;
	int err_no; /* handlers can save error here */
	ccpon_pack_overflow_handler handle_pack_overflow;
} ccpon_pack_context;


void ccpon_pack_context_init(ccpon_pack_context* pack_context, void *data, size_t length, ccpon_pack_overflow_handler hpo);

void ccpon_pack_null (ccpon_pack_context* pack_context);
void ccpon_pack_boolean (ccpon_pack_context* pack_context, bool b);
void ccpon_pack_int (ccpon_pack_context* pack_context, int64_t i);
void ccpon_pack_uint (ccpon_pack_context* pack_context, uint64_t i);
void ccpon_pack_double (ccpon_pack_context* pack_context, double d);
void ccpon_pack_decimal (ccpon_pack_context* pack_context, int64_t i, int dec_places);
void ccpon_pack_str (ccpon_pack_context* pack_context, const char* s, unsigned l);
//void ccpon_pack_blob (ccpon_pack_context* pack_context, const void *v, unsigned l);
void ccpon_pack_date_time (ccpon_pack_context* pack_context, int64_t epoch_msecs, int min_from_utc);

void ccpon_pack_array_begin (ccpon_pack_context* pack_context);
void ccpon_pack_array_end (ccpon_pack_context* pack_context);

void ccpon_pack_list_begin (ccpon_pack_context* pack_context);
void ccpon_pack_list_end (ccpon_pack_context* pack_context);

void ccpon_pack_map_begin (ccpon_pack_context* pack_context);
void ccpon_pack_map_end (ccpon_pack_context* pack_context);

void ccpon_pack_imap_begin (ccpon_pack_context* pack_context);
void ccpon_pack_imap_end (ccpon_pack_context* pack_context);

void ccpon_pack_meta_begin (ccpon_pack_context* pack_context);
void ccpon_pack_meta_end (ccpon_pack_context* pack_context);

void ccpon_pack_copy_str (ccpon_pack_context* pack_context, const void *str, size_t len);

void ccpon_pack_field_delim (ccpon_pack_context* pack_context, bool is_first_field);
void ccpon_pack_key_delim (ccpon_pack_context* pack_context);

/***************************** U N P A C K ********************************/


typedef enum
{
	CCPON_ITEM_INVALID = 0,
	CCPON_ITEM_NULL,
	CCPON_ITEM_BOOLEAN,
	CCPON_ITEM_INT,
	CCPON_ITEM_UINT,
	CCPON_ITEM_DOUBLE,
	CCPON_ITEM_DECIMAL,
	CCPON_ITEM_STRING,
	//CCPON_ITEM_BLOB,
	CCPON_ITEM_LIST,
	CCPON_ITEM_ARRAY,
	CCPON_ITEM_MAP,
	CCPON_ITEM_IMAP,
	CCPON_ITEM_DATE_TIME,
	CCPON_ITEM_META,
	//CCPON_ITEM_KEY_DELIM,
	//CCPON_ITEM_FIELD_DELIM,
	CCPON_ITEM_CONTAINER_END,
	/*
	CCPON_ITEM_ARRAY_END,
	CCPON_ITEM_MAP_END,
	CCPON_ITEM_IMAP_END,
	CCPON_ITEM_META_END,
	*/
} ccpon_item_types;

//enum ccpon_string_format {CCPON_STRING_FORMAT_INVALID = 0, CCPON_STRING_FORMAT_UTF8_ESCAPED, CCPON_STRING_FORMAT_HEX};
//enum ccpon_string_unpack_escape_stage {CCPON_STRING_ESC_NONE = 0, CCPON_STRING_ESC_FOUND, CCPON_STRING_ESC_OCTAL, CCPON_STRING_ESC_HEX, CCPON_STRING_ESC_U16};

typedef struct {
	const uint8_t* start;
	size_t length;
	//enum ccpon_string_format format;
	struct {
		uint16_t chunk_cnt;
		char escape_seq[2];
		//uint8_t escaped_val;
		//uint8_t escaped_len;
		//uint8_t begin: 1;
		uint8_t string_entered: 1;
		uint8_t last_chunk: 1;
		//uint8_t in_escape: 1;
	} parse_status;
} ccpon_string;

void ccpon_string_init(ccpon_string *str_it);

typedef struct {
	uint32_t size;
} ccpon_array;

typedef struct {
	int64_t msecs_since_epoch;
	int minutes_from_utc;
} ccpon_date_time;

typedef struct {
	int64_t mantisa;
	int dec_places;
} ccpon_decimal;

typedef struct {
	ccpon_item_types type;
	union
	{
		ccpon_string String;
		ccpon_string Blob;
		ccpon_date_time DateTime;
		ccpon_decimal Decimal;
		uint64_t UInt;
		int64_t Int;
		ccpon_array Array;
		double Double;
		bool Bool;
	} as;
} ccpon_item;

struct ccpon_unpack_context;

typedef size_t (*ccpon_unpack_underflow_handler)(struct ccpon_unpack_context*, size_t);

typedef struct ccpon_unpack_context {
	ccpon_item item;
	uint8_t* start;
	uint8_t* current;
	uint8_t* end; /* logical end of buffer */
	//int return_code;
	int err_no; /* handlers can save error here */
	ccpon_unpack_underflow_handler handle_unpack_underflow;
} ccpon_unpack_context;



void ccpon_unpack_context_init(ccpon_unpack_context* unpack_context, uint8_t* data, size_t length, ccpon_unpack_underflow_handler huu);

void ccpon_unpack_next (ccpon_unpack_context* unpack_context);
void ccpon_skip_items (ccpon_unpack_context* unpack_context, long item_count);

#ifdef __cplusplus
}
#endif

#endif /* C_CPON_H */
