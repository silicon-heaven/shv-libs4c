/*      CWPack - cwpack.c   */
/*
 The MIT License (MIT)

 Copyright (c) 2017 Claes Wihlborg

 Permission is hereby granted, free of charge, to any person obtaining a copy of this
 software and associated documentation files (the "Software"), to deal in the Software
 without restriction, including without limitation the rights to use, copy, modify,
 merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 persons to whom the Software is furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <string.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

#include "ccpon.h"
//#include "ccpon_defines.h"

//#if defined _WIN32 || defined LIBC_NEWLIB || 1
// see http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap04.html#tag_04_15
// see https://stackoverflow.com/questions/16647819/timegm-cross-platform
// see https://www.boost.org/doc/libs/1_62_0/boost/chrono/io/time_point_io.hpp
static inline int is_leap(int y)
{
	return (y % 4) == 0 && ((y % 100) != 0 || (y % 400) == 0);
}

static inline int32_t days_from_0(int32_t year)
{
	year--;
	return 365 * year + (year / 400) - (year/100) + (year / 4);
}

static inline int32_t days_from_1970(int32_t year)
{
	static int32_t days_from_0_to_1970 = 0;
	if(days_from_0_to_1970 == 0)
		days_from_0_to_1970 = days_from_0(1970);
	return days_from_0(year) - days_from_0_to_1970;
}

static inline int days_from_1jan(int year, int month, int mday)
{
	static const int days[2][12] =
	{
		{ 0,31,59,90,120,151,181,212,243,273,304,334},
		{ 0,31,60,91,121,152,182,213,244,274,305,335}
	};

	return days[is_leap(year)][month] + mday - 1;
}

static time_t ccpon_timegm(struct tm *tm)
{
	time_t res = 0;
	int year = tm->tm_year + 1900;
	int month = tm->tm_mon; // 0 - 11
	int mday = tm->tm_mday; // 1 - 31
	res = days_from_1970(year);
	res += days_from_1jan(year, month, mday);
	res *= 24;
	res += tm->tm_hour;
	res *= 60;
	res += tm->tm_min;
	res *= 60;
	res += tm->tm_sec;
	return res;
}
//#endif

uint8_t* ccpon_pack_reserve_space(ccpon_pack_context* pack_context, unsigned long more)
{
	uint8_t* p = pack_context->current;
	uint8_t* nyp = p + more;
	if (nyp > pack_context->end) {
		if (!pack_context->handle_pack_overflow) {
			pack_context->return_code = CCPON_RC_BUFFER_OVERFLOW;
			return 0;
		}
		int rc = pack_context->handle_pack_overflow (pack_context, (unsigned long)(more));
		if (rc) {
			pack_context->return_code = rc;
			return 0;
		}
		p = pack_context->current;
		nyp = p + more;
	}
	pack_context->current = nyp;
	return p;
}

const char CCPON_STR_NULL[] = "null";
const char CCPON_STR_TRUE[] = "true";
const char CCPON_STR_FALSE[] = "false";
const char CCPON_STR_IMAP_BEGIN[] = "i{";
const char CCPON_STR_ARRAY_BEGIN[] = "a[";
const char CCPON_STR_ESC_BLOB_BEGIN[] = "b\"";
//const char CCPON_STR_HEX_BLOB_BEGIN[] = "x\"";
//const char CCPON_STR_DATETIME_BEGIN[] = "d\"";

#define CCPON_C_KEY_DELIM ':'
#define CCPON_C_FIELD_DELIM ','
#define CCPON_C_LIST_BEGIN '['
#define CCPON_C_LIST_END ']'
#define CCPON_C_ARRAY_END ']'
#define CCPON_C_MAP_BEGIN '{'
#define CCPON_C_MAP_END '}'
#define CCPON_C_META_BEGIN '<'
#define CCPON_C_META_END '>'
#define CCPON_C_DECIMAL_END 'n'
#define CCPON_C_UNSIGNED_END 'u'

/*************************   C   S Y S T E M   L I B R A R Y   ****************/

#ifdef FORCE_NO_LIBRARY

static void	*memcpy(void *dst, const void *src, size_t n)
{
    unsigned int i;
    uint8_t *d=(uint8_t*)dst, *s=(uint8_t*)src;
    for (i=0; i<n; i++)
    {
        *d++ = *s++;
    }
    return dst;
}

#endif

/*******************************   P A C K   **********************************/
int ccpon_pack_context_init (ccpon_pack_context* pack_context, void *data, unsigned long length, ccpon_pack_overflow_handler hpo)
{
	pack_context->start = pack_context->current = (uint8_t*)data;
	pack_context->end = pack_context->start + length;
	pack_context->err_no = 0;
	pack_context->handle_pack_overflow = hpo;
	pack_context->return_code = CCPON_RC_OK;
	return pack_context->return_code;
}

/*  Packing routines  --------------------------------------------------------------------------------  */


void ccpon_pack_uint(ccpon_pack_context* pack_context, uint64_t i)
{
	if (pack_context->return_code)
		return;

	// at least 21 characters for 64-bit types.
	static const unsigned LEN = 32;
	char str[LEN];
	int n = snprintf(str, LEN, "%luu", i);
	if(n < 0) {
		pack_context->err_no = CCPON_RC_LOGICAL_ERROR;
		return;
	}
	uint8_t *p = ccpon_pack_reserve_space(pack_context, (unsigned)n);
	if(p) {
		memcpy(p, str, (unsigned)n);
	}
}


void ccpon_pack_int(ccpon_pack_context* pack_context, int64_t i)
{
	if (pack_context->return_code)
		return;

	// at least 21 characters for 64-bit types.
	static const unsigned LEN = 32;
	char str[LEN];
	int n = snprintf(str, LEN, "%ld", i);
	if(n < 0) {
		pack_context->err_no = CCPON_RC_LOGICAL_ERROR;
		return;
	}
	uint8_t *p = ccpon_pack_reserve_space(pack_context, (unsigned)n);
	if(p) {
		memcpy(p, str, (unsigned)n);
	}
}

void ccpon_pack_double(ccpon_pack_context* pack_context, double d)
{
	if (pack_context->return_code)
		return;

	// at least 21 characters for 64-bit types.
	static const unsigned LEN = 32;
	char str[LEN];
	int n = snprintf(str, LEN, "%lg", d);
	if(n < 0) {
		pack_context->err_no = CCPON_RC_LOGICAL_ERROR;
		return;
	}
	int has_dot = 0;
	int has_e = 0;
	for (int i = 0; i < n; ++i) {
		if(str[i] == 'e') {
			has_e = 1;
			break;
		}
		if(str[i] == '.') {
			has_dot = 1;
			break;
		}
	}
	if(!has_dot && !has_e) {
		str[n++] = '.';
	}
	uint8_t *p = ccpon_pack_reserve_space(pack_context, (unsigned)n);
	if(p) {
		memcpy(p, str, (unsigned)n);
	}
}

void ccpon_pack_null(ccpon_pack_context* pack_context)
{
	if (pack_context->return_code)
		return;
	uint8_t *p = ccpon_pack_reserve_space(pack_context, sizeof(CCPON_STR_NULL));
	if(p) {
		memcpy(p, CCPON_STR_NULL, sizeof(CCPON_STR_NULL) - 1);
	}
}

static void ccpon_pack_true (ccpon_pack_context* pack_context)
{
	if (pack_context->return_code)
		return;
	uint8_t *p = ccpon_pack_reserve_space(pack_context, sizeof(CCPON_STR_TRUE));
	if(p) {
		memcpy(p, CCPON_STR_TRUE, sizeof(CCPON_STR_TRUE) - 1);
	}
}

static void ccpon_pack_false (ccpon_pack_context* pack_context)
{
	if (pack_context->return_code)
		return;
	uint8_t *p = ccpon_pack_reserve_space(pack_context, sizeof(CCPON_STR_FALSE));
	if(p) {
		memcpy(p, CCPON_STR_FALSE, sizeof(CCPON_STR_FALSE) - 1);
	}
}

void ccpon_pack_boolean(ccpon_pack_context* pack_context, bool b)
{
	if (pack_context->return_code)
		return;

	if(b)
		ccpon_pack_true(pack_context);
	else
		ccpon_pack_false(pack_context);
}


void ccpon_pack_array_begin(ccpon_pack_context* pack_context)
{
	if (pack_context->return_code)
		return;

	uint8_t *p = ccpon_pack_reserve_space(pack_context, sizeof(CCPON_STR_ARRAY_BEGIN));
	if(p) {
		memcpy(p, CCPON_STR_ARRAY_BEGIN, sizeof(CCPON_STR_ARRAY_BEGIN) - 1);
	}
}

void ccpon_pack_array_end(ccpon_pack_context *pack_context)
{
	if (pack_context->return_code)
		return;

	uint8_t *p = ccpon_pack_reserve_space(pack_context, 1);
	if(p) {
		*p = CCPON_C_ARRAY_END;
	}
}

void ccpon_pack_list_begin(ccpon_pack_context *pack_context)
{
	if (pack_context->return_code)
		return;

	uint8_t *p = ccpon_pack_reserve_space(pack_context, 1);
	if(p) {
		*p = CCPON_C_LIST_BEGIN;
	}
}

void ccpon_pack_list_end(ccpon_pack_context *pack_context)
{
	if (pack_context->return_code)
		return;

	uint8_t *p = ccpon_pack_reserve_space(pack_context, 1);
	if(p) {
		*p = CCPON_C_LIST_END;
	}
}

void ccpon_pack_map_begin(ccpon_pack_context *pack_context)
{
	if (pack_context->return_code)
		return;

	uint8_t *p = ccpon_pack_reserve_space(pack_context, 1);
	if(p) {
		*p = CCPON_C_MAP_BEGIN;
	}
}

void ccpon_pack_map_end(ccpon_pack_context *pack_context)
{
	if (pack_context->return_code)
		return;

	uint8_t *p = ccpon_pack_reserve_space(pack_context, 1);
	if(p) {
		*p = CCPON_C_MAP_END;
	}
}

void ccpon_pack_imap_begin(ccpon_pack_context* pack_context)
{
	if (pack_context->return_code)
		return;

	uint8_t *p = ccpon_pack_reserve_space(pack_context, sizeof(CCPON_STR_IMAP_BEGIN));
	if(p) {
		memcpy(p, CCPON_STR_IMAP_BEGIN, sizeof(CCPON_STR_IMAP_BEGIN) - 1);
	}
}

void ccpon_pack_imap_end(ccpon_pack_context *pack_context)
{
	if (pack_context->return_code)
		return;

	uint8_t *p = ccpon_pack_reserve_space(pack_context, 1);
	if(p) {
		*p = CCPON_C_MAP_END;
	}
}

void ccpon_pack_meta_begin(ccpon_pack_context *pack_context)
{
	if (pack_context->return_code)
		return;

	uint8_t *p = ccpon_pack_reserve_space(pack_context, 1);
	if(p) {
		*p = CCPON_C_META_BEGIN;
	}
}

void ccpon_pack_meta_end(ccpon_pack_context *pack_context)
{
	if (pack_context->return_code)
		return;

	uint8_t *p = ccpon_pack_reserve_space(pack_context, 1);
	if(p) {
		*p = CCPON_C_META_END;
	}
}

static uint8_t* ccpon_pack_blob_data_escaped(ccpon_pack_context* pack_context, const void* v, unsigned l)
{
	if (pack_context->return_code)
		return 0;
	uint8_t *p = ccpon_pack_reserve_space(pack_context, 4*l);
	if(p) {
		for (unsigned i = 0; i < l; i++) {
			const uint8_t ch = ((const uint8_t*)v)[i];
			switch (ch) {
			case '\\': *p++ = '\\'; *p++ = '\\'; break;
			case '"' : *p++ = '\\'; *p++ = '"'; break;
			case '\b': *p++ = '\\'; *p++ = 'b'; break;
			case '\f': *p++ = '\\'; *p++ = 'f'; break;
			case '\n': *p++ = '\\'; *p++ = 'n'; break;
			case '\r': *p++ = '\\'; *p++ = 'r'; break;
			case '\t': *p++ = '\\'; *p++ = 't'; break;
			default: {
				if (ch <= 0x1f) {
					int n = snprintf((char*)p, 4, "\\x%02x", ch);
					p += n;
				}
				else {
					*p++ = ch;
				}
				break;
			}
			}
		}
	}
	pack_context->current = p;
	return p;
}

void ccpon_pack_str(ccpon_pack_context* pack_context, const char* s, unsigned l)
{
	if (pack_context->return_code)
		return;
	uint8_t *p = ccpon_pack_reserve_space(pack_context, 1);
	if(p) {
		*p = '"';
		p = ccpon_pack_blob_data_escaped(pack_context, s, 4*l);
		if(p) {
			p = ccpon_pack_blob_data_escaped(pack_context, s, 1);
			if(p)
				*p = '"';
		}
	}
}

void ccpon_pack_blob(ccpon_pack_context* pack_context, const void* v, unsigned l)
{
	if (pack_context->return_code)
		return;
	uint8_t *p = ccpon_pack_reserve_space(pack_context, sizeof (CCPON_STR_ESC_BLOB_BEGIN) - 1);
	if(p) {
		memcpy(p, CCPON_STR_ESC_BLOB_BEGIN, sizeof (CCPON_STR_ESC_BLOB_BEGIN) - 1);
		p = ccpon_pack_blob_data_escaped(pack_context, v, 4*l);
		if(p) {
			p = ccpon_pack_blob_data_escaped(pack_context, v, 1);
			if(p)
				*p = '"';
		}
	}
}

/*******************************   U N P A C K   **********************************/

#define UNPACK_ERROR(error_code)                        \
{                                                       \
    unpack_context->item.type = CCPON_ITEM_INVALID;        \
    unpack_context->err_no = error_code;           \
    return;                                             \
}

uint8_t* ccpon_unpack_assert_byte(ccpon_unpack_context* unpack_context)
{
	int more = 1;
	uint8_t* p = unpack_context->current;
	uint8_t* nyp = p + more;
	if (nyp > unpack_context->end) {
		if (!unpack_context->handle_unpack_underflow) {
			unpack_context->err_no = CCPON_RC_BUFFER_UNDERFLOW;
			//unpack_context->item.type = CCPON_ITEM_INVALID;
			return NULL;
		}
		int rc = unpack_context->handle_unpack_underflow (unpack_context, more);
		if (rc != CCPON_RC_OK) {
			unpack_context->err_no = rc;
			//unpack_context->item.type = CCPON_ITEM_INVALID;
			return NULL;
		}
		p = unpack_context->current;
		nyp = p + more;
	}
	unpack_context->current = nyp;
	return p;
}

#define UNPACK_ASSERT_BYTE()              \
{                                                       \
    p = ccpon_unpack_assert_byte(unpack_context);        \
    if(!p)           \
        return;                                             \
}

uint8_t* ccpon_unpack_skip_blank(ccpon_unpack_context* unpack_context)
{
	while(1) {
		uint8_t* p = ccpon_unpack_assert_byte(unpack_context);
		if(!p)
			return p;
		if(*p > ' ')
			return p;
	}
}

static int unpack_int(ccpon_unpack_context* unpack_context, int64_t *p_val)
{
	uint8_t *p1 = unpack_context->current;
	int64_t val = 0;
	int neg = 0;
	for (int n=0; ; n++) {
		uint8_t *p = ccpon_unpack_assert_byte(unpack_context);
		if(!p)
			goto eonumb;
		uint8_t b = *p;
		switch (b) {
		case '+':
		case '-':
			if(n != 0) {
				unpack_context->current--;
				goto eonumb;
			}
			if(b == '-')
				neg = 1;
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			val *= 10;
			val += b - '0';
			break;
		default:
			unpack_context->current--;
			goto eonumb;
		}
	}
eonumb:
	if(neg)
		val = -val;
	if(p_val)
		*p_val = val;
	return (int)(unpack_context->current - p1);
}
/*
static int get_int(const uint8_t *s, long len, int64_t *val)
{
	int64_t ret = 0;
	int neg = 0;
	int n;
	for (n = 0; n < len; n++) {
		uint8_t b = s[n];
		switch (b) {
		case '+':
		case '-':
			if(n != 0)
				return CCPON_RC_MALFORMED_INPUT;
			if(b == '-')
				neg = 1;
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			ret *= 10;
			ret += b - '0';
			break;
		default:
			if(n == 0)
				return CCPON_RC_MALFORMED_INPUT;
			goto eonumb;
		}
	}
eonumb:
	if(neg)
		ret = -ret;
	if(val)
		*val = ret;
	return n;
}
*/
static void unpack_date_time(ccpon_unpack_context *unpack_context, struct tm *tm, int *msec, int *utc_offset)
{
	tm->tm_year = 0;
	tm->tm_mon = 0;
	tm->tm_mday = 0;
	tm->tm_hour = 0;
	tm->tm_min = 0;
	tm->tm_sec = 0;
	tm->tm_isdst = -1;

	*msec = 0;
	*utc_offset = 0;

	uint8_t *p;

	int64_t val;
	int n = unpack_int(unpack_context, &val);
	if(n < 0)
		return;
	tm->tm_year = (int)val - 1900;

	UNPACK_ASSERT_BYTE();

	n = unpack_int(unpack_context, &val);
	if(n < 0)
		return;
	tm->tm_mon = (int)val - 1;

	UNPACK_ASSERT_BYTE();

	n = unpack_int(unpack_context, &val);
	if(n < 0)
		return;
	tm->tm_mday = (int)val;

	UNPACK_ASSERT_BYTE();

	n = unpack_int(unpack_context, &val);
	if(n < 0)
		return;
	tm->tm_hour = (int)val;

	UNPACK_ASSERT_BYTE();

	n = unpack_int(unpack_context, &val);
	if(n < 0)
		return;
	tm->tm_min = (int)val;

	UNPACK_ASSERT_BYTE();

	n = unpack_int(unpack_context, &val);
	if(n < 0)
		return;
	tm->tm_sec = (int)val;

	p = ccpon_unpack_assert_byte(unpack_context);
	if(p) {
		if(*p == '.') {
			n = unpack_int(unpack_context, &val);
			if(n < 0)
				return;
			*msec = (int)val;
			p = ccpon_unpack_assert_byte(unpack_context);
		}
		if(p) {
			uint8_t b = *p;
			if(b == 'Z') {
				// UTC time
			}
			else if(b == '+' || b == '-') {
				// UTC time
				n = unpack_int(unpack_context, &val);
				if(n < 0)
					return;
				if(!(n == 2 || n == 4))
					UNPACK_ERROR(CCPON_RC_MALFORMED_INPUT);
				if(n == 2)
					*utc_offset = (int)(60 * val);
				else if(n == 4)
					*utc_offset = (int)(60 * (val / 100) + (val % 100));
				if(b == '-')
					*utc_offset = -*utc_offset;
				p += n;
			}
			else {
				// unget unused char
				unpack_context->current--;
			}
		}
	}
	unpack_context->err_no = CCPON_RC_OK;
	unpack_context->item.type = CCPON_ITEM_DATE_TIME;
	int64_t epoch_msec = ccpon_timegm(tm) * 1000;
	ccpon_date_time *it = &unpack_context->item.as.DateTime;
	epoch_msec += *msec;
	it->msecs_since_epoch = epoch_msec;
	it->minutes_from_utc = *utc_offset;
}

void ccpon_unpack_context_init (ccpon_unpack_context* unpack_context, const void *data, unsigned long length, ccpon_unpack_underflow_handler huu)
{
	unpack_context->start = unpack_context->current = (uint8_t*)data;
	unpack_context->end = unpack_context->start + length;
	unpack_context->err_no = CCPON_RC_OK;
	unpack_context->handle_unpack_underflow = huu;
	//return unpack_context->return_code;
}


/*  Unpacking routines  ----------------------------------------------------------  */

void ccpon_string_init(ccpon_string *str_it)
{
	str_it->start = 0;
	str_it->length = 0;
	str_it->parse_status.begin = 0;
	//str_it->parse_status.middle = 0;
	str_it->parse_status.end = 0;
	str_it->parse_status.escape_stage = CCPON_STRING_ESC_NONE;
}

static inline int is_octal(uint8_t b)
{
	return b >= '0' && b <= '7';
}

static inline int is_hex(uint8_t b)
{
	return (b >= '0' && b <= '9')
			|| (b >= 'a' && b <= 'f')
			|| (b >= 'A' && b <= 'F');
}

static void ccpon_unpack_string(ccpon_unpack_context* unpack_context)
{
	if(unpack_context->item.type != CCPON_ITEM_STRING)
		UNPACK_ERROR(CCPON_RC_LOGICAL_ERROR);
	/*
	UNPACK_ASSERT_SPACE2(1, 0);


	if(!it_str->parse_status.begin) {
		if (*p != '"')
			return CCPON_RC_MALFORMED_INPUT; // not a string
		it_str->parse_status.begin = 1;
	}


	if(!it_str->parse_status.begin && !it_str->parse_status.middle) {
		UNPACK_ASSERT_SPACE2(1, 0);
		// not a string
		if (*p != '"')
			return CCPON_RC_MALFORMED_INPUT;
		it_str->parse_status.begin = 1;
	}
	*/
	ccpon_string *it = &unpack_context->item.as.String;
	it->start = unpack_context->current;
	for (; unpack_context->current < unpack_context->end; unpack_context->current++) {
		uint8_t *p = unpack_context->current;
		if(it->parse_status.escape_stage == CCPON_STRING_ESC_NONE) {
			if (*p == '"') {
				if(it->parse_status.begin) {
					// end of string
					unpack_context->current++;
					it->parse_status.end = 1;
					it->length = (p - it->start);
					return;
				}
				else {
					// begin of string
					it->parse_status.begin = 1;
					it->start = p+1;
					//continue;
				}
			}
			else if (*p == '\\') {
				it->parse_status.escape_stage = CCPON_STRING_ESC_FOUND;
				//it->parse_status.escaped_val = 0;
				it->parse_status.escaped_len = 0;
			}
		}
		else if(it->parse_status.escape_stage == CCPON_STRING_ESC_FOUND) {
			if(is_octal(*p)) {
				it->parse_status.escape_stage = CCPON_STRING_ESC_NONE;
				it->parse_status.escaped_len++;
				//it->parse_status.escaped_val = (*p - '0');
			}
			else if(*p == 'x') {
				it->parse_status.escape_stage = CCPON_STRING_ESC_HEX;
			}
			else if(*p == 'u') {
				it->parse_status.escape_stage = CCPON_STRING_ESC_U16;
			}
			else {
				it->parse_status.escape_stage = CCPON_STRING_ESC_NONE;
			}
		}
		else if(it->parse_status.escape_stage == CCPON_STRING_ESC_OCTAL) {
			if(is_octal(*p)) {
				it->parse_status.escaped_len++;
				/// according to https://en.cppreference.com/w/cpp/language/escape we should take up to 3
				if(it->parse_status.escaped_len == 3)
					it->parse_status.escape_stage = CCPON_STRING_ESC_NONE;
			}
			else {
				it->parse_status.escape_stage = CCPON_STRING_ESC_NONE;
				// non OCTAL char, examine this char again
				unpack_context->current--;
			}
		}
		else if(it->parse_status.escape_stage == CCPON_STRING_ESC_HEX) {
			/// according to https://en.cppreference.com/w/cpp/language/escape we should take chars until they are HEX
			/// but we take up to 2, since longer sequence cannot fit uint8_t
			if(is_hex(*p)) {
				//it->parse_status.escaped_val = 16 * it->parse_status.escaped_val + ((*p <= '9')? *p-'0': (*p <= 'f')? *p-'f': *p-'F');
				it->parse_status.escaped_len++;
				if(it->parse_status.escaped_len == 2)
					it->parse_status.escape_stage = CCPON_STRING_ESC_NONE;
			}
			else {
				it->parse_status.escape_stage = CCPON_STRING_ESC_NONE;
				// non HEX char, examine this char again
				unpack_context->current--;
			}
		}
		else if(it->parse_status.escape_stage == CCPON_STRING_ESC_U16) {
			/// take any 4
			it->parse_status.escaped_len++;
			if(it->parse_status.escaped_len == 4)
				it->parse_status.escape_stage = CCPON_STRING_ESC_NONE;
		}
		else {
			UNPACK_ERROR(CCPON_RC_LOGICAL_ERROR);
		}
	}
}

void ccpon_unpack_next (ccpon_unpack_context* unpack_context)
{
	if (unpack_context->err_no)
		return;

	uint8_t *p;
	if(unpack_context->item.type == CCPON_ITEM_STRING) {
		ccpon_string *str_it = &unpack_context->item.as.String;
		if(!str_it->parse_status.end) {
			UNPACK_ASSERT_BYTE();
			ccpon_unpack_string(unpack_context);
			return;
		}
	}

	unpack_context->item.type = CCPON_ITEM_INVALID;

	p = ccpon_unpack_skip_blank(unpack_context);
	if(!p)
		return;

	switch(*p) {
	case CCPON_C_KEY_DELIM:
		//unpack_context->item.type = CCPON_ITEM_KEY_DELIM;
		// silently ignore
		ccpon_unpack_next(unpack_context);
		break;
	case CCPON_C_FIELD_DELIM:
		//unpack_context->item.type = CCPON_ITEM_FIELD_DELIM;
		ccpon_unpack_next(unpack_context);
		break;
	case CCPON_C_MAP_END:
	case CCPON_C_LIST_END:
		unpack_context->item.type = CCPON_ITEM_CONTAINER_END;
		break;
	case CCPON_C_MAP_BEGIN:
		unpack_context->item.type = CCPON_ITEM_MAP;
		break;
	case CCPON_C_LIST_BEGIN:
		unpack_context->item.type = CCPON_ITEM_LIST;
		break;
	case 'i': {
		UNPACK_ASSERT_BYTE();
		if(*p != '{')
			UNPACK_ERROR(CCPON_RC_MALFORMED_INPUT)
		unpack_context->item.type = CCPON_ITEM_IMAP;
		break;
	}
	case 'a': {
		UNPACK_ASSERT_BYTE();
		if(*p != '[')
			UNPACK_ERROR(CCPON_RC_MALFORMED_INPUT)
		unpack_context->item.type = CCPON_ITEM_ARRAY;
		break;
	}
	case 'd': {
		UNPACK_ASSERT_BYTE();
		if(!p || *p != '"')
			UNPACK_ERROR(CCPON_RC_MALFORMED_INPUT)
		struct tm tm;
		int msec;
		int utc_offset;
		unpack_date_time(unpack_context, &tm, &msec, &utc_offset);
		UNPACK_ASSERT_BYTE();
		if(!p || *p != '"')
			UNPACK_ERROR(CCPON_RC_MALFORMED_INPUT)
		break;
	}
	case 'x': {
		UNPACK_ASSERT_BYTE();
		unpack_context->item.type = CCPON_ITEM_STRING;
		ccpon_string *str_it = &unpack_context->item.as.String;
		ccpon_string_init(str_it);
		str_it->format = CCPON_STRING_FORMAT_HEX;
		ccpon_unpack_string(unpack_context);
		break;
	}
	/*
	case 'b': {
		UNPACK_ASSERT_BYTE();
		if(*p != '"')
			UNPACK_ERROR(CCPON_RC_MALFORMED_INPUT)
		if(ccpon_unpack_string(unpack_context) == 0)
		return;
		unpack_context->item.type = CCPON_ITEM_BLOB;
		unpack_context->item.as.String.start = p + 1;
		unpack_context->item.as.String.length = (int)(unpack_context->current - p - 2);
		unpack_context->item.as.String.format = CCPON_STRING_FORMAT_UTF8_ESCAPED;
		break;
	}
	*/
	case '"': {
		unpack_context->item.type = CCPON_ITEM_STRING;
		ccpon_string *str_it = &unpack_context->item.as.String;
		ccpon_string_init(str_it);
		str_it->format = CCPON_STRING_FORMAT_UTF8_ESCAPED;
		ccpon_unpack_string(unpack_context);
		break;
	}
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case '+':
	case '-': {
		// number
		int64_t mantisa = 0;
		int64_t exponent = 0;
		int64_t decimals = 0;
		int dec_cnt = 0;
		struct {
			uint8_t is_decimal: 1;
			uint8_t is_double: 1;
			uint8_t is_uint: 1;
			uint8_t is_neg: 1;
		} flags;
		flags.is_decimal = 0;
		flags.is_double = 0;
		flags.is_uint = 0;
		flags.is_neg = 0;

		flags.is_neg = *p == '-';
		if(!flags.is_neg)
			unpack_context->current--;
		int n = unpack_int(unpack_context, &mantisa);
		if(n < 0)
			UNPACK_ERROR(n)
		p = ccpon_unpack_assert_byte(unpack_context);
		while(p) {
			if(*p == CCPON_C_UNSIGNED_END) {
				flags.is_uint = 1;
				break;
			}
			if(*p == '.') {
				flags.is_double = 1;
				n = unpack_int(unpack_context, &decimals);
				if(n < 0)
					UNPACK_ERROR(n)
				dec_cnt = n;
				p = ccpon_unpack_assert_byte(unpack_context);
				if(!p)
					break;
				if(*p == CCPON_C_DECIMAL_END) {
					flags.is_decimal = 1;
					break;
				}
			}
			if(*p == 'e' || *p == 'E') {
				flags.is_double = 1;
				n = unpack_int(unpack_context, &exponent);
				if(n < 0)
					UNPACK_ERROR(n)
				break;
			}
			if(*p != '.') {
				// unget char
				unpack_context->current--;
			}
			break;
		}
		if(flags.is_decimal) {
			for (int i = 0; i < dec_cnt; ++i)
				mantisa *= 10;
			mantisa += decimals;
			unpack_context->item.type = CCPON_ITEM_DECIMAL;
			unpack_context->item.as.Decimal.mantisa = flags.is_neg? -mantisa: mantisa;
			unpack_context->item.as.Decimal.dec_places = dec_cnt;
		}
		else if(flags.is_double) {
			double d = decimals;
			for (int i = 0; i < dec_cnt; ++i)
				d /= 10;
			d += mantisa;
			if(exponent < 0) {
				for (int i=0; i>exponent; i--)
					d /= 10;
			}
			else {
				for (int i=0; i<exponent; i++)
					d *= 10;
			}
			unpack_context->item.type = CCPON_ITEM_DOUBLE;
			unpack_context->item.as.Double = flags.is_neg? -d: d;
		}
		else if(flags.is_uint) {
			unpack_context->item.type = CCPON_ITEM_UINT;
			unpack_context->item.as.UInt = mantisa;;

		}
		else {
			unpack_context->item.type = CCPON_ITEM_INT;
			unpack_context->item.as.Int = flags.is_neg? -mantisa: mantisa;;
		}
		unpack_context->err_no = CCPON_RC_OK;
		break;
	}
	}
}

