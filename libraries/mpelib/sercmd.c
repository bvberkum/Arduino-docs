#include <stdio.h>
#include <avr/pgmspace.h>

#include <uart/uart.h>

#include "ser.h"
#include "sercmd.h"


uint8_t *buffer, limit, fill, top, next;
uint8_t instring, hexmode, hasvalue;
uint32_t value;


void sercmd_init(int size) {
	buffer = (uint8_t*) malloc(50);
	limit = 50;
	reset();
}

void reset(void) {
	fill = next = 0;
	instring = hexmode = hasvalue = 0;
	top = limit;
}

void process_uart(const Commands* cmds) {
	unsigned int ch = uart_getc();
	if ( ch & UART_NO_DATA ){
		return;
	}
	if ( ch & UART_FRAME_ERROR ) {
		uart_puts_P("UART Frame Error: ");
	}
	if ( ch & UART_OVERRUN_ERROR ) {
		uart_puts_P("UART Overrun Error: ");
	}
	if ( ch & UART_BUFFER_OVERFLOW ) {
		uart_puts_P("Buffer overflow error: ");
	}
	
	if (ch < ' ' || fill >= top) {
		reset();
		return;
	}

	if (instring) {
		if (ch == '"') {
			buffer[fill++] = 0;
			do
				buffer[--top] = buffer[--fill];
			while (fill > value);
			ch = top;
			instring = 0;
		}
		buffer[fill++] = ch;
		return;
	}
	if (hexmode && (('0' <= ch && ch <= '9') ||
				('A' <= ch && ch <= 'F') ||
				('a' <= ch && ch <= 'f'))) {
		if (!hasvalue)
			value = 0;
		if (ch > '9')
			ch += 9;
		value <<= 4;
		value |= (uint8_t) (ch & 0x0F);
		hasvalue = 1;
		return;
	}
	if ('0' <= ch && ch <= '9') {
		if (!hasvalue)
			value = 0;
		value = 10 * value + (ch - '0');
		hasvalue = 1;
		//print_value("value", value);
		return;
	}
	hexmode = 0;
	switch (ch) {
		case '$':   hexmode = 1;
					return;
		case '"':   instring = 1;
					value = fill;
					return;
		case ':':   //(uint16_t&) buffer[fill] = value;
					buffer[fill] = value;
					fill += 2;
					value >>= 16;
					// fall through
		case '.':   //(uint16_t&) buffer[fill] = value;
					buffer[fill] = value;
					fill += 2;
					hasvalue = 0;
					return;
		case '-':   value = - value;
					hasvalue = 0;
					return;
		case ' ':   if (!hasvalue)
						return;
					// fall through
		case ',':   buffer[fill++] = value;
					hasvalue = 0;
					return;
	}
	if (hasvalue) {
		uart0_puts("Unrecognized character: ");
		uart0_putc(ch);
		uart0_println("");
		reset();
		return;
	}

	for (Commands* p = cmds; ; ++p) {
		char code = pgm_read_byte(&p->code);
		if (code == 0)
			break;
		if (ch == code) {
			uint8_t bytes = pgm_read_byte(&p->bytes);
			if (fill < bytes) {
				uart0_puts("Not enough data: ");
				showNibble(fill);
				uart0_puts(", need ");
				showNibble(bytes);
				uart0_println(" bytes");
			} else {
				memset(buffer + fill, 0, top - fill);
				((void (*)(void)) pgm_read_word(&p->fun))();
			}
			reset();
			return;
		}
	}

	uart0_println("");

	//uart_putc( (unsigned char)ch );
}

void uart_v_get(void* ptr, uint8_t len) {
	memcpy(ptr, buffer + next, len);
	next += len;
}
void uart_cchar_get(const char* v) {
	uint8_t offset = buffer[next++];
	v = top <= offset && offset < limit ? (char*) buffer + offset : "";
}


