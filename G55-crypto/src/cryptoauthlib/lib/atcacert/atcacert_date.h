/** \brief Declarations for date handling with regard to certificates.
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

#ifndef ATCACERT_DATE_H
#define ATCACERT_DATE_H

#include <time.h>
#include "atcacert.h"

/** \defgroup atcacert_ Certificate manipulation methods (atcacert_)
 *
 * \brief
 * These methods provide convenient ways to perform certification I/O with
 * CryptoAuth chips and perform certificate manipulation in memory
 *
@{ */

/**
* Date formats.
*/
typedef enum atcacert_date_format_e {
    DATEFMT_ISO8601_SEP,      //!< ISO8601 full date YYYY-MM-DDThh:mm:ssZ
    DATEFMT_RFC5280_UTC,      //!< RFC 5280 (X.509) 4.1.2.5.1 UTCTime format YYMMDDhhmmssZ
    DATEFMT_POSIX_UINT32_BE,  //!< POSIX (aka UNIX) date format. Seconds since Jan 1, 1970. 32 bit unsigned integer, big endian.
    DATEFMT_RFC5280_GEN       //!< RFC 5280 (X.509) 4.1.2.5.2 GeneralizedTime format YYYYMMDDhhmmssZ
} atcacert_date_format_t;

#define DATEFMT_ISO8601_SEP_SIZE     (20)
#define DATEFMT_RFC5280_UTC_SIZE     (13)
#define DATEFMT_POSIX_UINT32_BE_SIZE (4)
#define DATEFMT_RFC5280_GEN_SIZE     (15)
#define DATEFMT_MAX_SIZE             DATEFMT_ISO8601_SEP_SIZE

static const size_t ATCACERT_DATE_FORMAT_SIZES[] = {
    DATEFMT_ISO8601_SEP_SIZE,
    DATEFMT_RFC5280_UTC_SIZE,
    DATEFMT_POSIX_UINT32_BE_SIZE,
    DATEFMT_RFC5280_GEN_SIZE
};

// Inform function naming when compiling in C++
#ifdef __cplusplus
extern "C" {
#endif

/**
* \brief Format a timestamp according to the format type.
*
* \param[in]    format               Format to use.
* \param[in]    timestamp            Timestamp to format.
* \param[out]   formatted_date       Formatted date will be return in this buffer.
* \param[inout] formatted_date_size  As input, the size of the formatted_date buffer.
*                                    As output, the size of the returned formatted_date.
*
* \return 0 on success
*/
int atcacert_date_enc( atcacert_date_format_t format,
                       const struct tm*       timestamp,
                       uint8_t*               formatted_date,
                       size_t*                formatted_date_size);

/**
* \brief Parse a formatted timestamp according to the specified format.
*
* \param[in]  format               Format to parse the formatted date as.
* \param[in]  formatted_date       Formatted date to be parsed.
* \param[in]  formatted_date_size  Size of the formatted date in bytes.
* \param[out] timestamp            Parsed timestamp is returned here.
*
* \return 0 on success
*/
int atcacert_date_dec( atcacert_date_format_t format,
                       const uint8_t*         formatted_date,
                       size_t                 formatted_date_size,
                       struct tm*             timestamp);

/**
* \brief Encode the issue and expire dates in the format used by the compressed certificate.
*
* \param[in]  issue_date    Issue date to encode. Note that minutes and seconds will be ignored.
* \param[in]  expire_years  Expire date is expressed as a number of years past the issue date.
*                           0 should be used if there is no expire date.
* \param[out] enc_dates     Encoded dates for use in the compressed certificate is returned here.
*                           3 bytes.
*
* \return 0 on success
*/
int atcacert_date_enc_compcert( const struct tm* issue_date,
                                uint8_t          expire_years,
                                uint8_t          enc_dates[3]);

/**
* \brief Decode the issue and expire dates from the format used by the compressed certificate.
*
* \param[in]  enc_dates    Encoded date from the compressed certificate. 3 bytes.
* \param[out] issue_date   Decoded issue date is returned here.
* \param[out] expire_date  Decoded expire date is returned here. If there is no expiration date,
*                          the expire date will be set to a maximum value 9999-12-31 23:59:59.
*
* \return 0 on success
*/
int atcacert_date_dec_compcert( const uint8_t enc_dates[3],
                                struct tm*    issue_date,
                                struct tm*    expire_date);

int atcacert_date_enc_iso8601_sep( const struct tm* timestamp,
                                   uint8_t          formatted_date[DATEFMT_ISO8601_SEP_SIZE]);

int atcacert_date_dec_iso8601_sep( const uint8_t formatted_date[DATEFMT_ISO8601_SEP_SIZE],
                                   struct tm*    timestamp);

int atcacert_date_enc_rfc5280_utc( const struct tm* timestamp,
                                   uint8_t          formatted_date[DATEFMT_RFC5280_UTC_SIZE]);

int atcacert_date_dec_rfc5280_utc( const uint8_t formatted_date[DATEFMT_RFC5280_UTC_SIZE],
                                   struct tm*    timestamp);

int atcacert_date_enc_rfc5280_gen( const struct tm* timestamp,
                                   uint8_t          formatted_date[DATEFMT_RFC5280_GEN_SIZE]);

int atcacert_date_dec_rfc5280_gen( const uint8_t formatted_date[DATEFMT_RFC5280_GEN_SIZE],
                                   struct tm*    timestamp);

int atcacert_date_enc_posix_uint32_be( const struct tm* timestamp,
                                       uint8_t          formatted_date[DATEFMT_POSIX_UINT32_BE_SIZE]);

int atcacert_date_dec_posix_uint32_be( const uint8_t formatted_date[DATEFMT_POSIX_UINT32_BE_SIZE],
                                       struct tm*    timestamp);

/** @} */
#ifdef __cplusplus
}
#endif

#endif