/*
 * atca_devtypes.h
 *
 * Created: 8/17/15 11:09:49 AM
 *  Author: landoncox
 */ 


#ifndef ATCA_DEVTYPES_H_
#define ATCA_DEVTYPES_H_

/** \defgroup device ATCADevice (atca_)
@{ */

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	ATSHA204A,
	ATECC108A,
	ATECC508A,
	ATAES132A
} ATCADeviceType;

#ifdef __cplusplus
}
#endif
/** @} */
#endif /* ATCA_DEVTYPES_H_ */