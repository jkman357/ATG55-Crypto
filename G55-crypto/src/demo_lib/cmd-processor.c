/** \file cmd_processor.c
* simple command processor for example console
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

#include <asf.h>
#include <string.h>
#include "cbuf.h"
#include "cryptoauthlib.h"
#include "cmd-processor.h"
#include "provision.h"
#include "node_auth.h"

/** \defgroup console Console functionality for node-auth-basic example
 *
 * \brief The console simply buffers keyboard input from the serial port and
 * store a command.  Once a newline is found, it calls the code to parse the command
 * and execute it.
 *
@{ */

/** \brief help method prints out the console menu commands to the console
	\return ATCA_STATUS
 */

int help(void)
{
	printf("\r\nUsage:\r\n");

    printf("client-provision  - Configure and load certificate data onto ATECC device.\r\n");
    printf("client-build      - Read certificate data off ATECC device and rebuild full signer and device certificates.\r\n");
    printf("host-chain-verify - Verify the certificate chain from the client.\r\n");
    printf("host-gen-chal     - Generate challenge for the client.\r\n");
    printf("client-gen-resp   - Generate response to challenge from host.\r\n");
    printf("host-verify-resp  - Verify the client response to the challenge.\r\n");
    printf("Utility functions:\r\n");
	printf("lockstat - zone lock status\r\n");
	printf("lockcfg  - lock config zone\r\n");
	printf("lockdata - lock data and OTP zones\r\n");
	printf("info     - get the chip revision\r\n");
	printf("sernum   - get the chip serial number\r\n");
		
	printf("\r\n");
	return ATCA_SUCCESS;
}

uint8_t cmdbytes[128];

/** \brief  lockstatus queries the lock status of configuration and data zones
 *  and prints the status of the zones to the console
 *  \return ATCA_STATUS
 */

static ATCA_STATUS lockstatus(void)
{
	ATCA_STATUS status;		
	uint8_t dataIsLocked=0xff, cfgIsLocked=0xff;
	
	if ( (status = atcab_is_locked( LOCK_ZONE_CONFIG, &cfgIsLocked )) != ATCA_SUCCESS )
		printf("can't read cfg lock\r\n");
	else if ( (status = atcab_is_locked( LOCK_ZONE_DATA, &dataIsLocked )) != ATCA_SUCCESS )
		printf("can't read data lock\r\n");

	if ( status == ATCA_SUCCESS ) {
        printf("Config Zone Lock: %s\r\n", cfgIsLocked == 0x01 ? "locked" : "unlocked");
        printf("Data Zone Lock  : %s\r\n", dataIsLocked == 0x01 ? "locked" :"unlocked" );
    }
	
	return status;
}

/** \brief lock_config_zone will lock the configuration zone of the CryptoAuth device
 * \returns the lock response from the chip
 */
static uint8_t lock_config_zone(void)
{
	uint8_t lock_response;
	atcab_lock_config_zone( &lock_response );
    
	return lock_response;		
}

/** \brief lock_data_zone will lock the data zone of the CryptoAuth device
 * \returns the lock response from the chip
 */
static uint8_t lock_data_zone(void)
{
	uint8_t lock_response;
	atcab_lock_data_zone( &lock_response );
    
	return lock_response;		
}

/** \brief getinfo returns the revision bytes from the chip.  These bytes identify
 * the exact revision of the silicon.
 */

static ATCA_STATUS getinfo( uint8_t *revision )
{
	return atcab_info( revision );
}

/** \brief getsernum returns the serial number bytes from the chip.  These bytes identify
 * the exact instance of a device - as expected, all serial numbers are unique across devices.
 */
static ATCA_STATUS getsernum( uint8_t *sernum )
{
	return atcab_read_serial_number( sernum );
}

/** \brief parseCmd takes a command string entered from the console and executes the command
 *  requested.
 *  \param[in] commands - a command string
 */

int parseCmd( char *commands )
{
	char *cmds = NULL;
	
	if ( (cmds = strstr( commands, "help")) ) {
		help();
	} else if ( (cmds = strstr( commands, "lockstat")) ) {
		lockstatus();
	} else if ( (cmds = strstr( commands, "lockcfg")) ) {
		if ( lock_config_zone() != ATCA_SUCCESS )
			printf("Could not lock config zone\r\n");
		lockstatus();
	} else if ( (cmds = strstr( commands, "lockdata")) ) {
		if ( lock_data_zone() != ATCA_SUCCESS )
			printf("Could not lock data zone\r\n");
		lockstatus();
	} else if ( (cmds = strstr( commands, "info")) ) {
		uint8_t revision[4];
		char displaystr[15];
		int displaylen = sizeof(displaystr);
		
		getinfo(revision);
		// dump revision
		atcab_bin2hex(revision, 4, displaystr, &displaylen );
		printf("\r\nrevision:\r\n%s\r\n", displaystr);
				
	} else if ( (cmds = strstr( commands, "sernum")) ) {
		uint8_t serialnum[ATCA_SERIAL_NUM_SIZE];
		char displaystr[30];
		int displaylen = sizeof(displaystr);
		
		getsernum(serialnum);
		// dump serial num
		atcab_bin2hex(serialnum, ATCA_SERIAL_NUM_SIZE, displaystr, &displaylen );
		printf("\r\nserial number:\r\n%s\r\n", displaystr);
    } else if ( (cmds = strstr( commands, "client-provision")) ) {
        int ret = client_provision();
        if (ret != ATCA_SUCCESS)
            printf("client_provision failed with error code %X\r\n", ret);
    } else if ( (cmds = strstr( commands, "client-build")) ) {
        int ret = client_rebuild_certs();
        if (ret != ATCA_SUCCESS)
            printf("client_rebuild_certs failed with error code %X\r\n", ret);
    } else if ( (cmds = strstr( commands, "host-chain-verify")) ) {
        int ret = host_verify_cert_chain();
        if (ret != ATCA_SUCCESS)
            printf("host_verify_cert_chain failed with error code %X\r\n", ret);
    } else if ( (cmds = strstr( commands, "host-gen-chal")) ) {
        int ret = host_generate_challenge();
        if (ret != ATCA_SUCCESS)
            printf("host_generate_challenge failed with error code %X\r\n", ret);
    } else if ( (cmds = strstr( commands, "client-gen-resp")) ) {
        int ret = client_generate_response();
        if (ret != ATCA_SUCCESS)
            printf("client_generate_response failed with error code %X\r\n", ret);
    }  else if ( (cmds = strstr( commands, "host-verify-resp")) ) {
        int ret = host_verify_response();
        if (ret != ATCA_SUCCESS)
            printf("verify_response failed with error code %X\r\n", ret);
    } else if ( strlen(commands) ) {
		printf("\r\nsyntax error in command: %s\r\n", commands);
	}
	
	return ATCA_SUCCESS;
}

/** \brief processCmd empties a circular buffer of stored command characters
 * int a command string, then makes the call to parse and execute the command
 */
int processCmd(void)
{
	static char cmd[256];
	uint16_t i = 0;
	while( !CBUF_IsEmpty(cmdQ) && i < sizeof(cmd))
		cmd[i++] = CBUF_Pop( cmdQ );
	cmd[i] = '\0';
	//printf("\r\n%s\r\n", command );
	parseCmd(cmd);
	printf("$ ");
	
	return ATCA_SUCCESS;
}


/** @} */

