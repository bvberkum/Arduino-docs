/**
 * Taking Node and Serial prototypes, loose Arduino.
 */
#include <stdlib.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/pgmspace.h> //TODO setup PSTR? F()?
#include <string.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <uart/uart.h>


/* *** Globals and sketch configuration *** */
#define DEBUG           1 /* Enable trace statements */
#define SERIAL          1 /* Enable serial */
							
#define _MEM            1   // Report free memory 
							
#if defined(__AVR_ATtiny84__) || defined(__AVR_ATtiny85__)
#define SRAM_SIZE       512
#define EEPROM_SIZE     512
#elif defined(__AVR_ATmega168__)
#define SRAM_SIZE       1024
#define EEPROM_SIZE     512
#elif defined(__AVR_ATmega328__) || defined(__AVR_ATmega328P__)
#define SRAM_SIZE       2048
#define EEPROM_SIZE     1024
#endif
							
#define CHAR_NEWLINE '\n'
#define CHAR_RETURN '\r'
#define RETURN_NEWLINE "\r\n"

const char sketch[] = "AVRNode";
const int version = 0;

unsigned char data_count = 0;
unsigned char data_in[8];
char command_in[8];

/* *** AVR routines *** {{{ */

int freeRam () {
	extern int __heap_start, *__brkval; 
	int v;
	return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

int usedRam () {
	return SRAM_SIZE - freeRam();
}

/* }}} *** */

/* *** ATmega routines *** {{{ */

double internalTemp(void)
{
	unsigned int wADC;
	double t;

	// The internal temperature has to be used
	// with the internal reference of 1.1V.
	// Channel 8 can not be selected with
	// the analogRead function yet.

	// Set the internal reference and mux.
	ADMUX = (_BV(REFS1) | _BV(REFS0) | _BV(MUX3));
	ADCSRA |= _BV(ADEN);  // enable the ADC

	//delay(20);            // wait for voltages to become stable.

	ADCSRA |= _BV(ADSC);  // Start the ADC

	// Detect end-of-conversion
	while (bit_is_set(ADCSRA,ADSC));

	// Reading register "ADCW" takes care of how to read ADCL and ADCH.
	wADC = ADCW;

	// The offset of 324.31 could be wrong. It is just an indication.
	t = (wADC - 311 ) / 1.22;

	// The returned temperature is in degrees Celcius.
	return (t);
}

/* }}} *** */

void print_value (char *id, int value) {
	char buffer[8];
	itoa(value, buffer, 10);
	uart_puts(id);
	uart_putc('=');
	uart_puts(buffer);
	uart_puts(RETURN_NEWLINE);
}

static void showNibble (uint8_t nibble) {
	char c = '0' + (nibble & 0x0F);
	if (c > '9')
		c += 7;
	uart_putc(c);
}

static void showByte (uint8_t value) {
	//if (config.hex_output) {
	showNibble(value >> 4);
	showNibble(value);
	//} else
	//    uart_puts((word) value);
}

/* Command parser */
//Stream& cmdIo = Serial;//virtSerial;
//extern InputParser::Commands cmdTab[] PROGMEM;
//byte* buffer = (byte*) malloc(50);
//InputParser parser (buffer, 50, cmdTab);//, virtSerial);
//

/* InputParser handlers {{{ */

void copy_command () {
	// Copy the contents of data_in into command_in
	memcpy(command_in, data_in, 8);
	// Now clear data_in, the UART can reuse it now
	memset(data_in, 0, 8);
}

void process_command() {
	//if(strcasestr(command_in,"GOTO") != NULL){
	//	if(strcasestr(command_in,"?") != NULL)
	//		print_value("goto", variable_goto);
	//	else
	//		variable_goto = parse_assignment(command_in);
	//}
	//else if(strcasestr(command_in,"A") != NULL){
	//	if(strcasestr(command_in,"?") != NULL)
	//		print_value("A", variable_A);
	//	else
	//		variable_A = parse_assignment(command_in);
	//}
} 

void uart_ok() {
  uart_puts("OK");
  uart_puts(RETURN_NEWLINE);
}

void process_uart() {
	unsigned int c = uart_getc();
	if ( c & UART_NO_DATA ){
		// no data available from UART 
	}
	else {
		if ( c & UART_FRAME_ERROR ) {
			uart_puts_P("UART Frame Error: ");
		}
		if ( c & UART_OVERRUN_ERROR ) {
			uart_puts_P("UART Overrun Error: ");
		}
		if ( c & UART_BUFFER_OVERFLOW ) {
			uart_puts_P("Buffer overflow error: ");
		}

		data_in[data_count] = c;

		// Return is signal for end of command input
		if (data_in[data_count] == CHAR_RETURN) {
			// Reset to 0, ready to go again
			data_count = 0;
			uart_puts(RETURN_NEWLINE);

			copy_command();
			process_command();
			uart_ok();
		} 
		else {
			data_count++;
		}

		uart_putc( (unsigned char)c );
	}
}

//InputParser::Commands cmdTab[] = {
//	{ '?', 0, helpCmd },
//	{ 'h', 0, helpCmd },
//	{ 'c', 0, configCmd},
//	{ 'm', 0, memStat},
//	{ 'n', 0, configNodeCmd },
//	{ 'v', 0, configVersionCmd },
//	{ 'N', 3, configSetNodeCmd },
//	{ 'C', 1, configNodeIDCmd },
//	{ 'W', 1, configEEPROM },
//	{ 'E', 0, eraseEEPROM },
//	{ 0 }
//};

/* }}} *** */

/* *** Main *** {{{ */

int main(void) {

	uart_init( UART_BAUD_SELECT(UART_BAUD_RATE,F_CPU) );
	sei();
	uart_puts("\n[");
	uart_puts(sketch);
	uart_puts(".");
	showNibble(version);
	uart_puts("]");

#if SERIAL
//	mpeser.begin();
//	mpeser.startAnnounce(sketch, String(version));
	//virtSerial.begin(4800);
#if DEBUG || _MEM
	print_value("RAM", freeRam());
	double tmp = internalTemp();
	print_value("CTEMP", tmp);
	print_value("CTEMP", (int)tmp);
	showByte(tmp);
	uart_puts(RETURN_NEWLINE);
#endif
//	serialFlush();
#endif
	for(;;) {
		process_uart();
	}
}

/* }}} *** */

