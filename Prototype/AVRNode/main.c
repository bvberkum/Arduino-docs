/**
 * Taking Node and Serial prototypes, loose Arduino.
 */
#include <stdlib.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include <uart/uart.h>
#define UART_BAUD_RATE 38400

int main(void) {
	uart_init( UART_BAUD_SELECT(UART_BAUD_RATE,F_CPU) );
	sei();
	uart_puts("startup...");
	for(;;) {
	}
}


