/** \file hal_samd21_i2c_asf.c
 * ATCA Hardware abstraction layer for SAMD21 I2C over ASF drivers.  This code is structured
 * in two parts.  Part 1 is the connection of the ATCA HAL API to the physical I2C implementation. 
 * Part 2 is the ASF I2C primitives to set up the interface.
 *  
 * Prerequisite: add SERCOM I2C Master Polled support to application in Atmel Studio
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
#include <stdio.h>
#include "atca_hal.h"
#include "hal_sam4s_i2c_asf.h"

#define TWI_Channel0	TWI4
#define TWI_Channel1	TWI1
#define FLEX_Channel0	FLEXCOM4
#define FLEX_Channel1	FLEXCOM0

/** \defgroup hal_ Crypto hardware abstraction layer (hal_)
 *
 * \brief
 * These methods define the hardware abstraction layer for communicating with a CryptoAuth device
 * using I2C driver of ASF.
 *
@{ */

/** \brief logical to physical bus mapping structure */
ATCAI2CMaster_t *i2c_hal_data[MAX_I2C_BUSES]; // map logical, 0-based bus number to index
int i2c_bus_ref_ct = 0;  // total in-use count across buses
//twi_master_options_t opt_twi_master;
twi_options_t	opt_twi_master;

#ifdef DEBUG_HAL
static void print_array(uint8_t *data, uint32_t data_size)
{
//	printf("%.4x\r\n", data_size);

	uint32_t n;
	
	for (n=0; n<data_size; n++)
	{
		printf("%.2x ", data[n]);
		if (((n+1)%16) == 0)
		{
			printf("\r\n");
			if ((n+1) != data_size)
			{
				printf("         ");
			}
		}
	}
	if (data_size%16 != 0)
	{
		printf("\r\n");
	}
}
#endif

/** \brief
	- this HAL implementation assumes you've included the ASF SERCOM I2C libraries in your project, otherwise, 
	the HAL layer will not compile because the ASF I2C drivers are a dependency *
 */

/** \brief hal_i2c_init manages requests to initialize a physical interface.  it manages use counts so when an interface
 * has released the physical layer, it will disable the interface for some other use.
 * You can have multiple ATCAIFace instances using the same bus, and you can have multiple ATCAIFace instances on 
 * multiple i2c buses, so hal_i2c_init manages these things and ATCAIFace is abstracted from the physical details.
 */

/** \brief initialize an I2C interface using given config
 * \param[in] hal - opaque ptr to HAL data
 * \param[in] cfg - interface configuration
 */
ATCA_STATUS hal_i2c_init(void *hal, ATCAIfaceCfg *cfg)
{
	int bus = cfg->atcai2c.bus;	// 0-based logical bus number
	ATCAHAL_t *phal = (ATCAHAL_t *)hal;
	
	if (i2c_bus_ref_ct == 0)	// power up state, no i2c buses will have been used
	{
		for(int i = 0; i < MAX_I2C_BUSES; i++)
		{
			i2c_hal_data[i] = NULL;
		}
	}
	
	i2c_bus_ref_ct++;	// total across buses
	
	if (bus >= 0 && bus < MAX_I2C_BUSES)
	{
		// if this is the first time this bus and interface has been created, do the physical work of enabling it
		if (i2c_hal_data[bus] == NULL)
		{
			i2c_hal_data[bus] = malloc(sizeof(ATCAI2CMaster_t));
			i2c_hal_data[bus]->ref_ct = 1;	// buses are shared, this is the first instance
			
			switch(bus)
			{
				/*
				case 0: pmc_enable_periph_clk(ID_TWI0); break;				
				case 1: pmc_enable_periph_clk(ID_TWI1); break;
				*/
				case 0:
					//flexcom_enable(FLEXCOM0);
					//flexcom_set_opmode(FLEXCOM0, FLEXCOM_TWI);
					flexcom_enable(FLEX_Channel0);
					flexcom_set_opmode(FLEX_Channel0, FLEXCOM_TWI);
					break;
					
				case 1:
					flexcom_enable(FLEX_Channel1);
					flexcom_set_opmode(FLEX_Channel1, FLEXCOM_TWI);
					break;
			}
			
			opt_twi_master.master_clk = sysclk_get_cpu_hz();
			opt_twi_master.speed = cfg->atcai2c.baud;
			opt_twi_master.smbus = 0;
			
			switch(bus)
			{
				case 0: twi_master_init(TWI_Channel0, &opt_twi_master);break;
				case 1: twi_master_init(TWI_Channel1, &opt_twi_master);break;
			}
			
			// store this for use during the release phase
			i2c_hal_data[bus]->bus_index = bus;
		}
		else
		{
			// otherwise, another interface already initialized the bus, so this interface will share it and any different
			// cfg parameters will be ignored...first one to initialize this sets the configuration
			i2c_hal_data[bus]->ref_ct++;
		}
		
		phal->hal_data = i2c_hal_data[bus];
		
		return ATCA_SUCCESS;
	}
	
	return ATCA_COMM_FAIL;
}

/** \brief HAL implementation of I2C post init
	* \param[in] ATCAIface instance
	* \return ATCA_STATUS
 */
ATCA_STATUS hal_i2c_post_init(ATCAIface iface)
{
	return ATCA_SUCCESS;
}

/** \brief HAL implementation of I2C send over ASF
	* \param[in] ATCAIface instance
	* \param[in] txdata pointer to space to bytes to send
	* \param[in] txlength number of bytes to send
	* \return ATCA_STATUS
 */
ATCA_STATUS hal_i2c_send(ATCAIface iface, uint8_t *txdata, int txlength)
{
#ifdef DEBUG_HAL
	printf("hal_i2c_send()\r\n");
	
	printf("\r\nCommand Packet (size:0x%.4x)\r\n", txlength);
	printf("Count  : %.2x\r\n", txdata[1]);
	printf("Opcode : %.2x\r\n", txdata[2]);
	printf("Param1 : %.2x\r\n", txdata[3]);
	printf("Param2 : "); print_array(&txdata[4], 2);
	if (txdata[1] > 7)
	{
		printf("Data   : "); print_array(&txdata[6], txdata[1]-7);
	}
	printf("CRC    : "); print_array(&txdata[txdata[1]-1], 2);
	printf("\r\n");
#endif
	
	ATCAIfaceCfg *cfg = atgetifacecfg(iface);
	int bus = cfg->atcai2c.bus;
	
	txdata[0] = 0x03;   // insert the Word Address Value, Command token
	txlength++;         // account for word address value byte.
	
	//twi_package_t packet = {
	/*
	twi_packet_t packet = {
		.chip        = cfg->atcai2c.slave_address >> 1,
		.addr[0]     = 0x03,
		.addr_length = 1,
		//.addr_length = 0,
		.buffer      = (void *)&txdata[1],	//(void *)txdata,
		.length      = (uint32_t)txdata[1]
	};*/
	twi_packet_t packet = {
		.chip			= cfg->atcai2c.slave_address >> 1,
		.addr[0]     = NULL,
		.addr_length	= 0,
		.buffer			= (void*)txdata,
		.length			= (uint32_t)txlength //(uint32_t)txdata[1]
	};

	// for this implementation of I2C with CryptoAuth chips, txdata is assumed to have ATCAPacket format
	
	// other device types that don't require i/o tokens on the front end of a command need a different hal_i2c_send and wire it up instead of this one
	// this covers devices such as ATSHA204A and ATECCx08A that require a word address value pre-pended to the packet
	
	switch(bus)
	{
		case 0:
			//if (twi_master_write(TWI0, &packet) != TWI_SUCCESS)
			if (twi_master_write(TWI_Channel0, &packet) != TWI_SUCCESS)
			{
				return ATCA_COMM_FAIL;
			}
			break;
		case 1:
			if (twi_master_write(TWI_Channel1, &packet) != TWI_SUCCESS)
			{
				return ATCA_COMM_FAIL;
			}
			break;
	}
	
	return ATCA_SUCCESS;
}

/** \brief HAL implementation of I2C receive function for ASF I2C
 * \param[in] ATCAIface instance
 * \param[in] rxdata pointer to space to receive the data
 * \param[in] ptr to expected number of receive bytes to request
 * \return ATCA_STATUS
 */
ATCA_STATUS hal_i2c_receive( ATCAIface iface, uint8_t *rxdata, uint16_t *rxlength)
{
#ifdef DEBUG_HAL
	printf("hal_i2c_receive()\r\n");
#endif
	
	ATCAIfaceCfg *cfg = atgetifacecfg(iface);
	int retries = cfg->rx_retries;
	int bus = cfg->atcai2c.bus;
	uint32_t status = !TWI_SUCCESS;
	
	//twi_package_t packet = {
	twi_packet_t packet = {	
		.chip        = cfg->atcai2c.slave_address >> 1,
		.addr[0]     = NULL,
		.addr_length = 0,
		.buffer      = (void *)rxdata,
		.length      = (uint32_t)*rxlength
	};
	
	switch(bus)
	{
		case 0:
			while(retries-- > 0 && status != TWI_SUCCESS)
			{
				//status = twi_master_read(TWI0, &packet);
				status = twi_master_read(TWI_Channel0, &packet);
			}
			break;
		case 1:
			while(retries-- > 0 && status != TWI_SUCCESS)
			{
				status = twi_master_read(TWI_Channel1, &packet);
			}
			break;
	}
	if (status != TWI_SUCCESS)
	{
		return ATCA_COMM_FAIL;
	}
	
#ifdef DEBUG_HAL
	printf("\r\nResponse Packet (size:0x%.4x)\r\n", rxlength);
	printf("Count  : %.2x\r\n", rxdata[0]);
	if (rxdata[0] > 3)
	{
		printf("Data   : "); print_array(&rxdata[1], rxdata[0]-3);
		printf("CRC    : "); print_array(&rxdata[rxdata[0]-2], 2);
	}
	printf("\r\n");
#endif
	
	return ATCA_SUCCESS;
}

/** \brief method to change the bus speed of I2C
 * \param[in] interface on which to change bus speed
 * \param[in] baud rate (typically 100000 or 400000)
 */
void change_i2c_speed( ATCAIface iface, uint32_t speed )
{
	ATCAIfaceCfg *cfg = atgetifacecfg(iface);
	int bus = cfg->atcai2c.bus;
	
	switch(bus)
	{
		case 0:
			//pmc_enable_periph_clk(ID_TWI0);
			//flexcom_enable(FLEXCOM0);
			//flexcom_set_opmode(FLEXCOM0, FLEXCOM_TWI);
			flexcom_enable(FLEX_Channel0);
			flexcom_set_opmode(FLEX_Channel0, FLEXCOM_TWI);
			
			opt_twi_master.master_clk = sysclk_get_cpu_hz();
			opt_twi_master.speed      = speed;
			opt_twi_master.smbus      = 0;
			//twi_master_init(TWI0, &opt_twi_master);
			twi_master_init(TWI_Channel0, &opt_twi_master);
			
			break;
		case 1:
			//pmc_enable_periph_clk(ID_TWI1);
			flexcom_enable(FLEX_Channel1);
			flexcom_set_opmode(FLEX_Channel1, FLEXCOM_TWI);
			
			opt_twi_master.master_clk = sysclk_get_cpu_hz();
			opt_twi_master.speed      = speed;
			opt_twi_master.smbus      = 0;
			twi_master_init(TWI_Channel1, &opt_twi_master);
			break;
	}
}

/** \brief wake up CryptoAuth device using I2C bus
 * \param[in] interface to logical device to wakeup
 */
ATCA_STATUS hal_i2c_wake(ATCAIface iface)
{
	ATCAIfaceCfg *cfg = atgetifacecfg(iface);
	int bus = cfg->atcai2c.bus;
	int retries = cfg->rx_retries;
	uint32_t bdrt = cfg->atcai2c.baud;
	uint8_t data[4], expected[4] = { 0x04,0x11,0x33,0x43 };
	int status = !TWI_SUCCESS;
	
	// if not already at 100KHz, change it
	if (bdrt != 100000)
	{
		change_i2c_speed(iface, 100000);
	}
	
	// Send 0x00 as wake pulse
	switch(bus)
	{
		//case 0: twi_write_byte(TWI0, 0x00); break;
		case 0: twi_write_byte(TWI_Channel0,0);break;
		case 1: twi_write_byte(TWI_Channel1,0); break;
	}
	
	atca_delay_us(cfg->wake_delay);	// wait tWHI + tWLO which is configured based on device type and configuration structure
	
	//twi_package_t packet = {
	twi_packet_t packet = {
		.chip        = cfg->atcai2c.slave_address >> 1,
		.addr[0]     = NULL,
		.addr_length = 0,
		.buffer      = (void *)data,
		.length      = 4
	};
	
	// if necessary, revert baud rate to what came in.
	if (bdrt != 100000)
	{
		change_i2c_speed(iface, bdrt);
	}
	
	switch(bus)
	{
		case 0:
			//if (twi_master_read(TWI0, &packet) != TWI_SUCCESS)
			while (retries-- > 0 && status != TWI_SUCCESS)
				status = twi_master_read(TWI_Channel0, &packet);
			/*if (twi_master_read(TWI_Channel0, &packet) != TWI_SUCCESS)
			{
				return ATCA_COMM_FAIL;
			}*/
			break;
		case 1:
			/*
			if (twi_master_read(TWI_Channel1, &packet) != TWI_SUCCESS)
			{
				return ATCA_COMM_FAIL;
			}*/
			break;
	}
	
	if (memcmp(data, expected, 4) == 0)
	{
		return ATCA_SUCCESS;
	}
	
	return ATCA_COMM_FAIL;
}

/** \brief idle CryptoAuth device using I2C bus
 * \param[in] interface to logical device to idle
 */
ATCA_STATUS hal_i2c_idle(ATCAIface iface)
{
	ATCAIfaceCfg *cfg = atgetifacecfg(iface);
	int bus = cfg->atcai2c.bus;
	uint8_t data[4];
	
	data[0] = 0x02;	// idle word address value
	
	//twi_package_t packet = {
	twi_packet_t packet = {
		.chip        = cfg->atcai2c.slave_address >> 1,
		.addr[0]     = NULL,
		.addr_length = 0,
		.buffer      = (void *)data,
		.length      = 1
	};
	
	switch(bus)
	{
		case 0:
			//if (twi_master_write(TWI0, &packet) != TWI_SUCCESS)
			if (twi_master_write(TWI_Channel0, &packet) != TWI_SUCCESS)
			{
				return ATCA_COMM_FAIL;
			}
			break;
		case 1:
			if (twi_master_write(TWI_Channel1, &packet) != TWI_SUCCESS)
			{
				return ATCA_COMM_FAIL;
			}
			break;
	}
	
	return ATCA_SUCCESS;
}

/** \brief sleep CryptoAuth device using I2C bus
 * \param[in] interface to logical device to sleep
 */
ATCA_STATUS hal_i2c_sleep(ATCAIface iface)
{
	ATCAIfaceCfg *cfg = atgetifacecfg(iface);
	int bus = cfg->atcai2c.bus;
	uint8_t data[4];
	
	data[0] = 0x01;	// sleep word address value
	
	//twi_package_t packet = {
	twi_packet_t packet = {
		.chip        = cfg->atcai2c.slave_address >> 1,
		.addr[0]     = NULL,
		.addr_length = 0,
		.buffer      = (void *)data,
		.length      = 1
	};
	
	switch(bus)
	{
		case 0:
			//if (twi_master_write(TWI0, &packet) != TWI_SUCCESS)
			if (twi_master_write(TWI_Channel0, &packet) != TWI_SUCCESS)
			{
				return ATCA_COMM_FAIL;
			}
			break;
		case 1:
			if (twi_master_write(TWI_Channel1, &packet) != TWI_SUCCESS)
			{
				return ATCA_COMM_FAIL;
			}
			break;
	}
	
	return ATCA_SUCCESS;
}

/** \brief manages reference count on given bus and releases resource if no more refences exist
 * \param[in] hal_data - opaque pointer to hal data structure - known only to the HAL implementation
 */
ATCA_STATUS hal_i2c_release( void *hal_data )
{
	ATCAI2CMaster_t *hal = (ATCAI2CMaster_t *)hal_data;
	
	i2c_bus_ref_ct--;  // track total i2c bus interface instances for consistency checking and debugging
	
	// if the use count for this bus has gone to 0 references, disable it.  protect against an unbracketed release
	if (hal && --(hal->ref_ct) <= 0 && i2c_hal_data[hal->bus_index] != NULL)
	{
		switch(hal->bus_index)
		{
			//case 0: twi_reset(TWI0); break;
			case 0: twi_reset(TWI_Channel0);break;
			case 1: twi_reset(TWI_Channel1);break;
		}
		free(i2c_hal_data[hal->bus_index]);
		i2c_hal_data[hal->bus_index] = NULL;
	}
	
	return ATCA_SUCCESS;
}

/** @} */
