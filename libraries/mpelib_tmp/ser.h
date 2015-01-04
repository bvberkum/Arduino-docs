#ifndef mpelib_ser_h
#define mpelib_ser_h

#ifndef UART_BAUD_RATE
#define UART_BAUD_RATE 2400
#endif

#define CHAR_NEWLINE '\n'
#define CHAR_RETURN '\r'
#define RETURN_NEWLINE "\r\n"

#include <stdlib.h>

#include <uart.h>


//static void uart0_println(char *line) {
//	uart0_puts(line);
//	uart0_puts(RETURN_NEWLINE);
//}
//
//static void uart0_putint(int value) {
//	char buffer[8];
//	itoa(value, buffer, 10);
//	uart0_puts(buffer);
//}

static void print_value (char *id, int value) {
	Serial.print(id);
	Serial.print('=');
	Serial.println(value);
}

static void showNibble(char nibble) {
	char c = '0' + (nibble & 0x0F);
	if (c > '9')
		c += 7;
	Serial.print(c);
}

static void showByte(char value) {
	//if (config.hex_output) {
	showNibble(value >> 4);
	showNibble(value);
	//} else
	//    uart_puts((word) value);
}

static void uart_ok(void) {
	Serial.println("OK");
}

static void mpelib_ser_start(void) {
	uart_init( UART_BAUD_SELECT(UART_BAUD_RATE, F_CPU) );
}
static void mpelib_ser_announce(const char *sketch, int version) {
	Serial.print("\n[");
	Serial.print(sketch);
	Serial.print(".");
	showNibble(version);
	Serial.print("]");
}

#endif

