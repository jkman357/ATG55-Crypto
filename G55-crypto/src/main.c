
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
	uint8_t serialnum[ATCA_SERIAL_NUM_SIZE];

	/* Initialize the SAM system */
	sysclk_init();

	/* Initialize the board */
	board_init();
	
	/* Initialize the console UART */
	configure_console();

	/* Output example information */
	puts(STRING_HEADER);
	
	atcab_init( &cfg_ateccx08a_i2c_default );
	
	while (true) {
		
		ch = 0;
		
		scanf("%c",&ch);
		
		if (ch) {
			printf("%c",ch); // echo to output
			if ( ch == 0x0d || ch == 0x0a ) {
				processCmd();
				} else {
				CBUF_Push( cmdQ, ch );  // queue character into circular buffer
			}
		}
	}
	
	
}
