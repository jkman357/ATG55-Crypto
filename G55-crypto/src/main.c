
#include "asf.h"
#include "stdio_serial.h"
#include "conf_board.h"
#include "cryptoauthlib.h"
#include "led.h"
//#include "cbuf.h"
#include <cbuf.h>
#include <cmd-processor.h>
//#include "cmd-processor.h"

#define STRING_EOL    "\r"
#define STRING_HEADER "-- G55 & ECCx08 Example --\r\n" \
		"-- "BOARD_NAME" --\r\n" \
		"-- Compiled: "__DATE__" "__TIME__" --"STRING_EOL

#if SAMG55
/** TWI ID for simulated EEPROM application to use */
#define BOARD_ID_TWI_EEPROM         ID_TWI4

/** TWI Base for simulated TWI EEPROM application to use */
#define BOARD_BASE_TWI_EEPROM       TWI4

#endif

/** Global timestamp in milliseconds since start of application */
volatile uint32_t g_ul_ms_ticks = 0;

/**
 *  \brief Handler for System Tick interrupt.
 *
 *  Process System Tick Event
 *  increments the timestamp counter.
 */
void SysTick_Handler(void)
{
	g_ul_ms_ticks++;
}

/**
 *  \brief Configure the Console UART.
 */
static void configure_console(void)
{
	const usart_serial_options_t uart_serial_options = {
		.baudrate = CONF_UART_BAUDRATE,
#ifdef CONF_UART_CHAR_LENGTH
		.charlength = CONF_UART_CHAR_LENGTH,
#endif
		.paritytype = CONF_UART_PARITY,
#ifdef CONF_UART_STOP_BITS
		.stopbits = CONF_UART_STOP_BITS,
#endif
	};

	/* Configure console UART. */
	sysclk_enable_peripheral_clock(CONSOLE_UART_ID);
	stdio_serial_init(CONF_UART, &uart_serial_options);
}


/**
 * \brief Application entry point for TWI EEPROM example.
 *
 * \return Unused (ANSI-C compatibility).
 */
int main(void)
{
	uint8_t ch;
	
	uint8_t revision[4];
	char displaystr[256];
	int displaylen; 
	uint8_t serialnum[ATCA_SERIAL_NUM_SIZE];
	uint8_t random_number[32];
	ATCA_STATUS status = ATCA_GEN_FAIL;

	/* Initialize the SAM system */
	sysclk_init();

	/* Initialize the board */
	board_init();
	
	/* Initialize the console UART */
	configure_console();

	/* Output example information */
	//puts(STRING_HEADER);
	
	atcab_init( &cfg_ateccx08a_i2c_default );
	
	
	displaylen = sizeof(displaystr);
	atcab_info(revision);
	// dump revision
	atcab_bin2hex(revision, 4, displaystr, &displaylen );
	printf("\r\nrevision:\r\n%s\r\n", displaystr);
	
	atcab_read_serial_number(serialnum);
	displaylen = sizeof(displaystr);
	// dump serial num
	atcab_bin2hex(serialnum, ATCA_SERIAL_NUM_SIZE, displaystr, &displaylen );
	printf("\r\nserial number:\r\n%s\r\n", displaystr);
	
	displaylen = sizeof(displaystr);
	atcab_random(random_number);
	atcab_bin2hex(random_number, 32, displaystr, &displaylen );
	printf("\r\nrandom number:\r\n%s\r\n", displaystr);
	
	uint8_t public_key[64],get_public_key[64];
	bool isLocked;
	uint8_t frag[4] = { 0x44, 0x44, 0x44, 0x44 };
	memset(public_key, 0x44, 64 );  // mark the key with bogus data
	
	status = atcab_is_locked( LOCK_ZONE_CONFIG, &isLocked );
	if ( !isLocked )
		printf("\r\nConfiguration zone must be locked for this test to succeed.\r\n");
	
	atcab_genkey(0, public_key);
	displaylen = sizeof(displaystr);	
	atcab_bin2hex(public_key, 64, displaystr, &displaylen );
	printf("\r\n64 Bytes Public Key generation:\r\n%s\r\n", displaystr);
	

	atcab_is_locked( LOCK_ZONE_CONFIG, &isLocked );
	if ( !isLocked )
		printf("\r\nConfiguration zone must be locked for this test to succeed.\r\n");

	atcab_get_pubkey(0, get_public_key);
	displaylen = sizeof(displaystr);
	atcab_bin2hex(get_public_key, 64, displaystr, &displaylen );
	printf("\r\nGet 64 Bytes Public Key:\r\n%s\r\n", displaystr);

	uint8_t msg[64];
	uint8_t signature[ATCA_SIG_SIZE];
	isLocked = false;

	status = atcab_is_locked( LOCK_ZONE_CONFIG, &isLocked );
	if ( !isLocked )
		printf("\r\nConfiguration zone must be locked for this test to succeed.\r\n");

	memset( msg, 0xAA, 64 );
	memset( signature, 0, 64 );
	//for(int i = 0; i < 64; i++)
	//	msg[i] = i;

	atcab_sign(0, msg, signature);
	displaylen = sizeof(displaystr);
	atcab_bin2hex(msg, 64, displaystr, &displaylen );
	printf("\r\nMessage before Signature:\r\n%s\r\n", displaystr);
	
	displaylen = sizeof(displaystr);
	atcab_bin2hex(signature, 64, displaystr, &displaylen );
	printf("\r\nSignature:\r\n%s\r\n", displaystr);
		
	status = atcab_release();
}
