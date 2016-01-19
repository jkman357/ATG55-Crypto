/** \brief  date handling with regard to certificates.
*
* Copyright (c) 2015 Atmel Corporation. All rights reserved.
*
* \asf_license_start
*
* \page License
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*
* 3. The name of Atmel may not be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
* 4. This software may only be redistributed and used in connection with an
*    Atmel microcontroller product.
*
* THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
* EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
* OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
* ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
* \asf_license_stop
 */ 


#include "atcacert_date.h"
#include <string.h>

int atcacert_date_enc( atcacert_date_format_t format,
                       const struct tm*       timestamp,
                       uint8_t*               formatted_date,
                       size_t*                formatted_date_size)
{
    if (timestamp == NULL || formatted_date_size == NULL || format >= sizeof(ATCACERT_DATE_FORMAT_SIZES) / sizeof(ATCACERT_DATE_FORMAT_SIZES[0]))
        return ATCACERT_E_BAD_PARAMS;

    if (formatted_date != NULL && *formatted_date_size < ATCACERT_DATE_FORMAT_SIZES[format])
    {
        *formatted_date_size = ATCACERT_DATE_FORMAT_SIZES[format];
        return ATCACERT_E_BUFFER_TOO_SMALL;
    }
    *formatted_date_size = ATCACERT_DATE_FORMAT_SIZES[format];
    if (formatted_date == NULL)
        return ATCACERT_E_SUCCESS; // Caller just wanted 

    switch (format)
    {
        case DATEFMT_ISO8601_SEP:     return atcacert_date_enc_iso8601_sep(timestamp, formatted_date);
        case DATEFMT_RFC5280_UTC:     return atcacert_date_enc_rfc5280_utc(timestamp, formatted_date);
        case DATEFMT_POSIX_UINT32_BE: return atcacert_date_enc_posix_uint32_be(timestamp, formatted_date);
        case DATEFMT_RFC5280_GEN:     return atcacert_date_enc_rfc5280_gen(timestamp, formatted_date);
        default: return ATCACERT_E_BAD_PARAMS;
    }

    return ATCACERT_E_BAD_PARAMS;
}

int atcacert_date_dec( atcacert_date_format_t format,
                       const uint8_t*         formatted_date,
                       size_t                 formatted_date_size,
                       struct tm*             timestamp)
{
    if (formatted_date == NULL || timestamp == NULL || format >= sizeof(ATCACERT_DATE_FORMAT_SIZES) / sizeof(ATCACERT_DATE_FORMAT_SIZES[0]))
        return ATCACERT_E_BAD_PARAMS;

    if (formatted_date_size < ATCACERT_DATE_FORMAT_SIZES[format])
        return ATCACERT_E_DECODING_ERROR; // Not enough data to parse this date format

    switch (format)
    {
    case DATEFMT_ISO8601_SEP:     return atcacert_date_dec_iso8601_sep(formatted_date, timestamp);
    case DATEFMT_RFC5280_UTC:     return atcacert_date_dec_rfc5280_utc(formatted_date, timestamp);
    case DATEFMT_POSIX_UINT32_BE: return atcacert_date_dec_posix_uint32_be(formatted_date, timestamp);
    case DATEFMT_RFC5280_GEN:     return atcacert_date_dec_rfc5280_gen(formatted_date, timestamp);
    default: return ATCACERT_E_BAD_PARAMS;
    }

    return ATCACERT_E_SUCCESS;
}

/**
 * \brief Convert an unsigned integer to a zero padded string with no terminating null.
 */
static uint8_t* uint_to_str(uint32_t num, int width, uint8_t* str)
{
    uint8_t* ret = str + width;
    int i;

    // Pre-fill the string width with zeros
    for (i = 0; i < width; i++)
        *(str++) = '0';
    // Convert the number from right to left
    for (; num; num /= 10)
        *(--str) = '0' + (num % 10);

    return ret;
}

/**
* \brief Convert a number string as a zero padded unsigned integer back into a number
*/
static const uint8_t* str_to_uint(const uint8_t* str, int width, uint32_t* num)
{
    const uint8_t* error_ret = str;
    const uint8_t* good_ret = str + width;
    uint32_t prev_num = 0;
    uint32_t digit_value = 1;
    int digit;

    str += width - 1;
    *num = 0;
    for (digit = 0; digit < width; digit++)
    {
        if (*str < '0' || *str > '9')
            return error_ret; // Character is not a digit
        if (digit >= 10)
        {
            if (*str != '0')
                return error_ret; // Number is larger than the output can handle
            continue;
        }
        if (digit == 9 && *str > '4')
            return error_ret; // Number is larger than the output can handle

        *num += digit_value * (*str - '0');
        if (*num < prev_num)
            return error_ret; // Number rolled over, it is larger than the output can handle
        
        digit_value *= 10;
        prev_num = *num;
        str--;
    }

    return good_ret;
}

/**
* \brief Convert a number string as a zero padded unsigned integer back into a number constrained
*        to an integer's size.
*/
static const uint8_t* str_to_int(const uint8_t* str, int width, int* num)
{
    uint32_t unum = 0;
    const uint8_t* ret = str_to_uint(str, width, &unum);
    if (ret != str && unum > 2147483647UL)
        ret = str; // Number exceeds int32's range
    *num = (int)unum;
    return ret;
}

int atcacert_date_enc_iso8601_sep( const struct tm* timestamp,
                                   uint8_t          formatted_date[DATEFMT_ISO8601_SEP_SIZE])
{
    uint8_t* cur_pos = formatted_date;
    int year = 0;

    if (timestamp == NULL || formatted_date == NULL)
        return ATCACERT_E_BAD_PARAMS;

    year = timestamp->tm_year + 1900;

    if (year < 0 || year > 9999)
        return ATCACERT_E_INVALID_DATE;
    cur_pos = uint_to_str(year, 4, cur_pos);

    *(cur_pos++) = '-';

    if (timestamp->tm_mon < 0 || timestamp->tm_mon > 11)
        return ATCACERT_E_INVALID_DATE;
    cur_pos = uint_to_str(timestamp->tm_mon + 1, 2, cur_pos);

    *(cur_pos++) = '-';

    if (timestamp->tm_mday < 1 || timestamp->tm_mday > 31)
        return ATCACERT_E_INVALID_DATE;
    cur_pos = uint_to_str(timestamp->tm_mday, 2, cur_pos);

    *(cur_pos++) = 'T';

    if (timestamp->tm_hour < 0 || timestamp->tm_hour > 23)
        return ATCACERT_E_INVALID_DATE;
    cur_pos = uint_to_str(timestamp->tm_hour, 2, cur_pos);

    *(cur_pos++) = ':';

    if (timestamp->tm_min < 0 || timestamp->tm_min > 59)
        return ATCACERT_E_INVALID_DATE;
    cur_pos = uint_to_str(timestamp->tm_min, 2, cur_pos);

    *(cur_pos++) = ':';

    if (timestamp->tm_sec < 0 || timestamp->tm_sec > 59)
        return ATCACERT_E_INVALID_DATE;
    cur_pos = uint_to_str(timestamp->tm_sec, 2, cur_pos);

    *(cur_pos++) = 'Z';

    return ATCACERT_E_SUCCESS;
}

int atcacert_date_dec_iso8601_sep( const uint8_t formatted_date[DATEFMT_ISO8601_SEP_SIZE],
                                   struct tm*    timestamp)
{
    const uint8_t* cur_pos = formatted_date;
    const uint8_t* new_pos = NULL;

    if (formatted_date == NULL || timestamp == NULL)
        return ATCACERT_E_BAD_PARAMS;

    memset(timestamp, 0, sizeof(*timestamp));

    new_pos = str_to_int(cur_pos, 4, &timestamp->tm_year);
    if (new_pos == cur_pos)
        return ATCACERT_E_DECODING_ERROR; // There was a problem converting the string to a number
    cur_pos = new_pos;
    timestamp->tm_year -= 1900;

    if (*(cur_pos++) != '-')
        return ATCACERT_E_DECODING_ERROR; // Unexpected separator

    new_pos = str_to_int(cur_pos, 2, &timestamp->tm_mon);
    if (new_pos == cur_pos)
        return ATCACERT_E_DECODING_ERROR; // There was a problem converting the string to a number
    cur_pos = new_pos;
    timestamp->tm_mon -= 1;

    if (*(cur_pos++) != '-')
        return ATCACERT_E_DECODING_ERROR; // Unexpected separator

    new_pos = str_to_int(cur_pos, 2, &timestamp->tm_mday);
    if (new_pos == cur_pos)
        return ATCACERT_E_DECODING_ERROR; // There was a problem converting the string to a number
    cur_pos = new_pos;

    if (*(cur_pos++) != 'T')
        return ATCACERT_E_DECODING_ERROR; // Unexpected separator

    new_pos = str_to_int(cur_pos, 2, &timestamp->tm_hour);
    if (new_pos == cur_pos)
        return ATCACERT_E_DECODING_ERROR; // There was a problem converting the string to a number
    cur_pos = new_pos;

    if (*(cur_pos++) != ':')
        return ATCACERT_E_DECODING_ERROR; // Unexpected separator

    new_pos = str_to_int(cur_pos, 2, &timestamp->tm_min);
    if (new_pos == cur_pos)
        return ATCACERT_E_DECODING_ERROR; // There was a problem converting the string to a number
    cur_pos = new_pos;

    if (*(cur_pos++) != ':')
        return ATCACERT_E_DECODING_ERROR; // Unexpected separator

    new_pos = str_to_int(cur_pos, 2, &timestamp->tm_sec);
    if (new_pos == cur_pos)
        return ATCACERT_E_DECODING_ERROR; // There was a problem converting the string to a number
    cur_pos = new_pos;

    if (*(cur_pos++) != 'Z')
        return ATCACERT_E_DECODING_ERROR; // Unexpected UTC marker

    return ATCACERT_E_SUCCESS;
}

int atcacert_date_enc_rfc5280_utc( const struct tm* timestamp,
                                   uint8_t          formatted_date[DATEFMT_RFC5280_UTC_SIZE])
{
    uint8_t* cur_pos = formatted_date;
    int year = 0;

    if (timestamp == NULL || formatted_date == NULL)
        return ATCACERT_E_BAD_PARAMS;

    year = timestamp->tm_year + 1900;

    if (year >= 1950 && year <= 1999)
        year = year - 1900;
    else if (year >= 2000 && year <= 2049)
        year = year - 2000;
    else
        return ATCACERT_E_INVALID_DATE;  // Year out of range for RFC2459 UTC format
    cur_pos = uint_to_str(year, 2, cur_pos);

    if (timestamp->tm_mon < 0 || timestamp->tm_mon > 11)
        return ATCACERT_E_INVALID_DATE;
    cur_pos = uint_to_str(timestamp->tm_mon + 1, 2, cur_pos);

    if (timestamp->tm_mday < 1 || timestamp->tm_mday > 31)
        return ATCACERT_E_INVALID_DATE;
    cur_pos = uint_to_str(timestamp->tm_mday, 2, cur_pos);

    if (timestamp->tm_hour < 0 || timestamp->tm_hour > 23)
        return ATCACERT_E_INVALID_DATE;
    cur_pos = uint_to_str(timestamp->tm_hour, 2, cur_pos);

    if (timestamp->tm_min < 0 || timestamp->tm_min > 59)
        return ATCACERT_E_INVALID_DATE;
    cur_pos = uint_to_str(timestamp->tm_min, 2, cur_pos);

    if (timestamp->tm_sec < 0 || timestamp->tm_sec > 59)
        return ATCACERT_E_INVALID_DATE;
    cur_pos = uint_to_str(timestamp->tm_sec, 2, cur_pos);

    *(cur_pos++) = 'Z';

    return ATCACERT_E_SUCCESS;
}

int atcacert_date_dec_rfc5280_utc( const uint8_t formatted_date[DATEFMT_RFC5280_UTC_SIZE],
                                   struct tm*    timestamp)
{
    const uint8_t* cur_pos = formatted_date;
    const uint8_t* new_pos = NULL;

    if (formatted_date == NULL || timestamp == NULL)
        return ATCACERT_E_BAD_PARAMS;

    memset(timestamp, 0, sizeof(*timestamp));

    new_pos = str_to_int(cur_pos, 2, &timestamp->tm_year);
    if (new_pos == cur_pos)
        return ATCACERT_E_DECODING_ERROR; // There was a problem converting the string to a number
    cur_pos = new_pos;
    if (timestamp->tm_year < 50)
        timestamp->tm_year += 2000;
    else
        timestamp->tm_year += 1900;
    timestamp->tm_year -= 1900;

    new_pos = str_to_int(cur_pos, 2, &timestamp->tm_mon);
    if (new_pos == cur_pos)
        return ATCACERT_E_DECODING_ERROR; // There was a problem converting the string to a number
    cur_pos = new_pos;
    timestamp->tm_mon -= 1;

    new_pos = str_to_int(cur_pos, 2, &timestamp->tm_mday);
    if (new_pos == cur_pos)
        return ATCACERT_E_DECODING_ERROR; // There was a problem converting the string to a number
    cur_pos = new_pos;

    new_pos = str_to_int(cur_pos, 2, &timestamp->tm_hour);
    if (new_pos == cur_pos)
        return ATCACERT_E_DECODING_ERROR; // There was a problem converting the string to a number
    cur_pos = new_pos;

    new_pos = str_to_int(cur_pos, 2, &timestamp->tm_min);
    if (new_pos == cur_pos)
        return ATCACERT_E_DECODING_ERROR; // There was a problem converting the string to a number
    cur_pos = new_pos;

    new_pos = str_to_int(cur_pos, 2, &timestamp->tm_sec);
    if (new_pos == cur_pos)
        return ATCACERT_E_DECODING_ERROR; // There was a problem converting the string to a number
    cur_pos = new_pos;

    if (*(cur_pos++) != 'Z')
        return ATCACERT_E_DECODING_ERROR; // Unexpected UTC marker

    return ATCACERT_E_SUCCESS;
}

int atcacert_date_enc_rfc5280_gen( const struct tm* timestamp,
                                   uint8_t          formatted_date[DATEFMT_RFC5280_GEN_SIZE])
{
    uint8_t* cur_pos = formatted_date;
    int year = 0;

    if (timestamp == NULL || formatted_date == NULL)
        return ATCACERT_E_BAD_PARAMS;

    year = timestamp->tm_year + 1900;

    if (year < 0 || year > 9999)
        return ATCACERT_E_INVALID_DATE;
    cur_pos = uint_to_str(year, 4, cur_pos);

    if (timestamp->tm_mon < 0 || timestamp->tm_mon > 11)
        return ATCACERT_E_INVALID_DATE;
    cur_pos = uint_to_str(timestamp->tm_mon + 1, 2, cur_pos);

    if (timestamp->tm_mday < 1 || timestamp->tm_mday > 31)
        return ATCACERT_E_INVALID_DATE;
    cur_pos = uint_to_str(timestamp->tm_mday, 2, cur_pos);

    if (timestamp->tm_hour < 0 || timestamp->tm_hour > 23)
        return ATCACERT_E_INVALID_DATE;
    cur_pos = uint_to_str(timestamp->tm_hour, 2, cur_pos);

    if (timestamp->tm_min < 0 || timestamp->tm_min > 59)
        return ATCACERT_E_INVALID_DATE;
    cur_pos = uint_to_str(timestamp->tm_min, 2, cur_pos);

    if (timestamp->tm_sec < 0 || timestamp->tm_sec > 59)
        return ATCACERT_E_INVALID_DATE;
    cur_pos = uint_to_str(timestamp->tm_sec, 2, cur_pos);

    *(cur_pos++) = 'Z';

    return ATCACERT_E_SUCCESS;
}

int atcacert_date_dec_rfc5280_gen( const uint8_t formatted_date[DATEFMT_RFC5280_GEN_SIZE],
                                   struct tm*    timestamp)
{
    const uint8_t* cur_pos = formatted_date;
    const uint8_t* new_pos = NULL;

    if (formatted_date == NULL || timestamp == NULL)
        return ATCACERT_E_BAD_PARAMS;

    memset(timestamp, 0, sizeof(*timestamp));

    new_pos = str_to_int(cur_pos, 4, &timestamp->tm_year);
    if (new_pos == cur_pos)
        return ATCACERT_E_DECODING_ERROR; // There was a problem converting the string to a number
    cur_pos = new_pos;
    timestamp->tm_year -= 1900;

    new_pos = str_to_int(cur_pos, 2, &timestamp->tm_mon);
    if (new_pos == cur_pos)
        return ATCACERT_E_DECODING_ERROR; // There was a problem converting the string to a number
    cur_pos = new_pos;
    timestamp->tm_mon -= 1;

    new_pos = str_to_int(cur_pos, 2, &timestamp->tm_mday);
    if (new_pos == cur_pos)
        return ATCACERT_E_DECODING_ERROR; // There was a problem converting the string to a number
    cur_pos = new_pos;

    new_pos = str_to_int(cur_pos, 2, &timestamp->tm_hour);
    if (new_pos == cur_pos)
        return ATCACERT_E_DECODING_ERROR; // There was a problem converting the string to a number
    cur_pos = new_pos;

    new_pos = str_to_int(cur_pos, 2, &timestamp->tm_min);
    if (new_pos == cur_pos)
        return ATCACERT_E_DECODING_ERROR; // There was a problem converting the string to a number
    cur_pos = new_pos;

    new_pos = str_to_int(cur_pos, 2, &timestamp->tm_sec);
    if (new_pos == cur_pos)
        return ATCACERT_E_DECODING_ERROR; // There was a problem converting the string to a number
    cur_pos = new_pos;

    if (*(cur_pos++) != 'Z')
        return ATCACERT_E_DECODING_ERROR; // Unexpected UTC marker

    return ATCACERT_E_SUCCESS;
}

int atcacert_date_enc_posix_uint32_be( const struct tm* timestamp,
                                       uint8_t          formatted_date[DATEFMT_POSIX_UINT32_BE_SIZE])
{
    struct tm timestamp_nc;
    time_t posix_time = 0;
    int year = 0;

    if (timestamp == NULL || formatted_date == NULL)
        return ATCACERT_E_BAD_PARAMS;
        
    year = timestamp->tm_year + 1900;
        
    if (year > 2106 || year < 1970)
        return ATCACERT_E_INVALID_DATE; //Timestamp out of range for POSIX time.
    if (timestamp->tm_mon < 0 || timestamp->tm_mon > 11)
        return ATCACERT_E_INVALID_DATE;
    if (timestamp->tm_mday < 1 || timestamp->tm_mday > 31)
        return ATCACERT_E_INVALID_DATE;
    if (timestamp->tm_hour < 0 || timestamp->tm_hour > 23)
        return ATCACERT_E_INVALID_DATE;
    if (timestamp->tm_min < 0 || timestamp->tm_min > 59)
        return ATCACERT_E_INVALID_DATE;
    if (timestamp->tm_sec < 0 || timestamp->tm_sec > 59)
        return ATCACERT_E_INVALID_DATE;
    // Check for date past max date for POSIX time
    if (year == 2106)
    {
        if (timestamp->tm_mon > 1)
            return ATCACERT_E_INVALID_DATE;
        if (timestamp->tm_mon == 1)
        {
            if (timestamp->tm_mday > 7)
                return ATCACERT_E_INVALID_DATE;
            if (timestamp->tm_mday == 7)
            {
                if (timestamp->tm_hour > 6)
                    return ATCACERT_E_INVALID_DATE;
                if (timestamp->tm_hour == 6)  
                {
                    if (timestamp->tm_min > 28)
                        return ATCACERT_E_INVALID_DATE;
                    if (timestamp->tm_min == 28)
                    {
                        if (timestamp->tm_sec > 14)
                            return ATCACERT_E_INVALID_DATE;
                    }
                }
            }
        }
    }
    
#ifdef WIN32
    timestamp_nc = *timestamp;
    posix_time = _mkgmtime(&timestamp_nc);
    if (posix_time == -1)
        return ATCACERT_E_INVALID_DATE;
#else
    timestamp_nc = *timestamp;
    posix_time = mktime(&timestamp_nc);
    if (posix_time == -1)
        return ATCACERT_E_INVALID_DATE;
#endif
    
    formatted_date[0] = (uint8_t)((posix_time >> 24) & 0xFF);
    formatted_date[1] = (uint8_t)((posix_time >> 16) & 0xFF);
    formatted_date[2] = (uint8_t)((posix_time >> 8) & 0xFF);
    formatted_date[3] = (uint8_t)((posix_time >> 0) & 0xFF);
    
    return ATCACERT_E_SUCCESS;
}

int atcacert_date_dec_posix_uint32_be( const uint8_t formatted_date[DATEFMT_POSIX_UINT32_BE_SIZE],
                                       struct tm*    timestamp)
{
#ifdef WIN32
    time_t posix_time = 0;
    errno_t ret = 0;

    if (formatted_date == NULL || timestamp == NULL)
        return ATCACERT_E_BAD_PARAMS;

    posix_time = ((time_t)formatted_date[0] << 24) | ((time_t)formatted_date[1] << 16) | ((time_t)formatted_date[2] << 8) | ((time_t)formatted_date[3]);
    ret = gmtime_s(timestamp, &posix_time);
    if (ret != 0)
        return ATCACERT_E_DECODING_ERROR; // Failed to convert to timestamp structure
#else
    time_t posix_time = 0;
    struct tm* ret = NULL;
    if (formatted_date == NULL || timestamp == NULL)
        return ATCACERT_E_BAD_PARAMS;

    posix_time = ((time_t)formatted_date[0] << 24) | ((time_t)formatted_date[1] << 16) | ((time_t)formatted_date[2] << 8) | ((time_t)formatted_date[3]);
    
    ret = gmtime_r(&posix_time, timestamp);
    if (ret == NULL)
        return ATCACERT_E_DECODING_ERROR; // Failed to convert to timestamp structure
#endif

    return ATCACERT_E_SUCCESS;
}

int atcacert_date_enc_compcert( const struct tm* issue_date,
                                uint8_t          expire_years,
                                uint8_t          enc_dates[3])
{
    /*
    * Issue and expire dates are compressed/encoded as below
    * +---------------+---------------+---------------+
    * | Byte 1        | Byte 2        | Byte 3        |
    * +---------------+---------------+---------------+
    * | | | | | | | | | | | | | | | | | | | | | | | | |
    * | 5 bits  | 4 bits| 5 bits  | 5 bits  | 5 bits  |
    * | Year    | Month | Day     | Hour    | Expire  |
    * |         |       |         |         | Years   |
    * +---------+-------+---------+---------+---------+
    *
    * Minutes and seconds are always zero.
    */
    if (issue_date == NULL || enc_dates == NULL)
        return ATCACERT_E_BAD_PARAMS;

    if ((issue_date->tm_year + 1900) < 2000 || (issue_date->tm_year + 1900) > 2031)
        return ATCACERT_E_INVALID_DATE;
    if (issue_date->tm_mon < 0 || issue_date->tm_mon > 11)
        return ATCACERT_E_INVALID_DATE;
    if (issue_date->tm_mday < 1 || issue_date->tm_mday > 31)
        return ATCACERT_E_INVALID_DATE;
    if (issue_date->tm_hour < 0 || issue_date->tm_hour > 23)
        return ATCACERT_E_INVALID_DATE;
    if (expire_years > 31)
        return ATCACERT_E_INVALID_DATE;

    memset(enc_dates, 0, 3);

    enc_dates[0] = (enc_dates[0] & 0x07) | (((issue_date->tm_year + 1900 - 2000) & 0x1F) << 3);
    enc_dates[0] = (enc_dates[0] & 0xF8) | (((issue_date->tm_mon + 1) & 0x0F) >> 1);
    enc_dates[1] = (enc_dates[1] & 0x7F) | (((issue_date->tm_mon + 1) & 0x0F) << 7);
    enc_dates[1] = (enc_dates[1] & 0x83) | ((issue_date->tm_mday & 0x1F) << 2);
    enc_dates[1] = (enc_dates[1] & 0xFC) | ((issue_date->tm_hour & 0x1F) >> 3);
    enc_dates[2] = (enc_dates[2] & 0x1F) | ((issue_date->tm_hour & 0x1F) << 5);
    enc_dates[2] = (enc_dates[2] & 0xE0) | (expire_years & 0x1F);

    return ATCACERT_E_SUCCESS;
}

int atcacert_date_dec_compcert( const uint8_t enc_dates[3],
                                struct tm*    issue_date,
                                struct tm*    expire_date)
{
    uint8_t expire_years = 0;
    /*
    * Issue and expire dates are compressed/encoded as below
    * +---------------+---------------+---------------+
    * | Byte 1        | Byte 2        | Byte 3        |
    * +---------------+---------------+---------------+
    * | | | | | | | | | | | | | | | | | | | | | | | | |
    * | 5 bits  | 4 bits| 5 bits  | 5 bits  | 5 bits  |
    * | Year    | Month | Day     | Hour    | Expire  |
    * |         |       |         |         | Years   |
    * +---------+-------+---------+---------+---------+
    *
    * Minutes and seconds are always zero.
    */

    if (enc_dates == NULL || issue_date == NULL || expire_date == NULL)
        return ATCACERT_E_BAD_PARAMS;

    memset(issue_date, 0, sizeof(*issue_date));
    memset(expire_date, 0, sizeof(*expire_date));

    issue_date->tm_year = (enc_dates[0] >> 3) + 2000 - 1900;
    issue_date->tm_mon  = (((enc_dates[0] & 0x07) << 1) | ((enc_dates[1] & 0x80) >> 7)) - 1;
    issue_date->tm_mday = ((enc_dates[1] & 0x7C) >> 2);
    issue_date->tm_hour = ((enc_dates[1] & 0x03) << 3) | ((enc_dates[2] & 0xE0) >> 5);

    expire_years = (enc_dates[2] & 0x1F);

    if (expire_years != 0)
    {
        expire_date->tm_year = issue_date->tm_year + expire_years;
        expire_date->tm_mon  = issue_date->tm_mon;
        expire_date->tm_mday = issue_date->tm_mday;
        expire_date->tm_hour = issue_date->tm_hour;
    }
    else
    {
        // Expire years of 0, means no expiration. Set to max date.
        expire_date->tm_year = 9999 - 1900;
        expire_date->tm_mon  = 12 - 1;
        expire_date->tm_mday = 31;
        expire_date->tm_hour = 23;
        expire_date->tm_min  = 59;
        expire_date->tm_sec  = 59;
    }

    return ATCACERT_E_SUCCESS;
}