#include <stdlib.h>

#include <uart/uart.h>

#include "ser.h"


void uart0_println(char *line) {
	uart_puts(line);
	uart_puts(RETURN_NEWLINE);
}

void uart0_putint(int value) {
	char buffer[8];
	itoa(value, buffer, 10);
	uart0_puts(buffer);
}

void print_value (char *id, int value) {
	uart_puts(id);
	uart_putc('=');
	uart0_putint(value);
	uart_puts(RETURN_NEWLINE);
}

void showNibble (uint8_t nibble) {
	char c = '0' + (nibble & 0x0F);
	if (c > '9')
		c += 7;
	uart_putc(c);
}

void showByte (uint8_t value) {
	//if (config.hex_output) {
	showNibble(value >> 4);
	showNibble(value);
	//} else
	//    uart_puts((word) value);
}

void uart_ok(void) {
	uart_puts("OK");
	uart_puts(RETURN_NEWLINE);
}

void mpelib_ser_start(void) {
	uart_init( UART_BAUD_SELECT(UART_BAUD_RATE, F_CPU) );
}
void mpelib_ser_announce(const char *sketch, int version) {
	uart_puts("\n[");
	uart_puts(sketch);
	uart_puts(".");
	showNibble(version);
	uart_puts("]");
}

