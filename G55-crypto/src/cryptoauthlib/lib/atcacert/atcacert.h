/**
* \file      atcacert_der.h
* \brief     Declarations common to all atcacert code.
* \author    Atmel Crypto Group, Colorado Springs, CO
* \copyright 2015 Atmel Corporation
*
* These are common definitions used by all the atcacert code.
*/

#ifndef ATCACERT_H
#define ATCACERT_H

#include <stddef.h>
#include <stdint.h>

/** \defgroup atcacert_ Certificate manipulation methods (atcacert_)
 *
 * \brief
 * These methods provide convenient ways to perform certification I/O with
 * CryptoAuth chips and perform certificate manipulation in memory
 *
@{ */
#ifndef FALSE
#define FALSE (0)
#endif
#ifndef TRUE
#define TRUE (1)
#endif

#define ATCACERT_E_SUCCESS          0 //!< Operation completed successfully.
#define ATCACERT_E_ERROR            1 //!< General error.
#define ATCACERT_E_BAD_PARAMS       2 //!< Invalid/bad parameter passed to function.
#define ATCACERT_E_BUFFER_TOO_SMALL 3 //!< Supplied buffer for output is too small to hold the result.
#define ATCACERT_E_DECODING_ERROR   4 //!< Data being decoded/parsed has an invalid format.
#define ATCACERT_E_INVALID_DATE     5 //!< 
#define ATCACERT_E_UNIMPLEMENTED    6 //!< Function is unimplemented for the current configuration.
#define ATCACERT_E_UNEXPECTED_ELEM_SIZE 7 //!< A certificate element size was what was expected.
#define ATCACERT_E_ELEM_MISSING         8 //!< The certificate element isn't defined for the certificate definition.
#define ATCACERT_E_ELEM_OUT_OF_BOUNDS   9 //!< Certificate element is out of bounds for the given certificate.
#define ATCACERT_E_BAD_CERT             10 //!< Certificate structure is bad in some way.
#define ATCACERT_E_WRONG_CERT_DEF       11
#define ATCACERT_E_VERIFY_FAILED        12 //!< Certificate or challenge/response verification failed

/** @} */
#endif