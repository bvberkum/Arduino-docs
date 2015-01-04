#ifndef mpelib_sercmd_h
#define mpelib_sercmd_h

struct cmd_struct {
	char code;
	uint8_t bytes;
	void (*fun)(void);
};
typedef struct cmd_struct Command;

/*
typedef struct cmd_struct {
	char code;
	uint8_t bytes;
	void (*fun)(void);
} Command ;

extern Command sercmd_tab[];

void sercmd_init(int size);
void sercmd_reset(void);
void process_uart(Command* cmds);
void uart_v_get(void* ptr, uint8_t len);
void uart_cchar_get(const char* v);
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <avr/pgmspace.h>

#include "ser.h"
//#include "sercmd.h"


uint8_t *sercmd_buffer, sercmd_limit, sercmd_fill, sercmd_top, sercmd_next;
uint8_t sercmd_instring, sercmd_hexmode, sercmd_hasvalue;
uint32_t sercmd_value;

void sercmd_reset(void) {
	sercmd_fill = sercmd_next = 0;
	sercmd_instring = sercmd_hexmode = sercmd_hasvalue = 0;
	sercmd_top = sercmd_limit;
}

void sercmd_init(int size) {
	sercmd_buffer = (uint8_t*) malloc(50);
	sercmd_limit = 50;
	sercmd_reset();
}

void process_uart(Command* cmds) {
	unsigned int ch = uart_getc();
	if ( ch & UART_NO_DATA ){
		return;
	}
	if ( ch & UART_FRAME_ERROR ) {
		Serial.print("UART Frame Error: ");
	}
	if ( ch & UART_OVERRUN_ERROR ) {
		Serial.print("UART Overrun Error: ");
	}
	if ( ch & UART_BUFFER_OVERFLOW ) {
		Serial.print("Buffer overflow error: ");
	}
	
	if (ch < ' ' || sercmd_fill >= sercmd_top) {
		sercmd_reset();
		return;
	}

	if (sercmd_instring) {
		if (ch == '"') {
			sercmd_buffer[sercmd_fill++] = 0;
			do
				sercmd_buffer[--sercmd_top] = sercmd_buffer[--sercmd_fill];
			while (sercmd_fill > sercmd_value);
			ch = sercmd_top;
			sercmd_instring = 0;
		}
		sercmd_buffer[sercmd_fill++] = ch;
		return;
	}
	if (sercmd_hexmode && (('0' <= ch && ch <= '9') ||
				('A' <= ch && ch <= 'F') ||
				('a' <= ch && ch <= 'f'))) {
		if (!sercmd_hasvalue)
			sercmd_value = 0;
		if (ch > '9')
			ch += 9;
		sercmd_value <<= 4;
		sercmd_value |= (uint8_t) (ch & 0x0F);
		sercmd_hasvalue = 1;
		return;
	}
	if ('0' <= ch && ch <= '9') {
		if (!sercmd_hasvalue)
			sercmd_value = 0;
		sercmd_value = 10 * sercmd_value + (ch - '0');
		sercmd_hasvalue = 1;
		//print_value("sercmd_value", sercmd_value);
		return;
	}
	sercmd_hexmode = 0;
	switch (ch) {
		case '$':   sercmd_hexmode = 1;
					return;
		case '"':   sercmd_instring = 1;
					sercmd_value = sercmd_fill;
					return;
		case ':':   //(uint16_t&) sercmd_buffer[sercmd_fill] = sercmd_value;
					sercmd_buffer[sercmd_fill] = sercmd_value;
					sercmd_fill += 2;
					sercmd_value >>= 16;
					// fall through
		case '.':   //(uint16_t&) sercmd_buffer[sercmd_fill] = sercmd_value;
					sercmd_buffer[sercmd_fill] = sercmd_value;
					sercmd_fill += 2;
					sercmd_hasvalue = 0;
					return;
		case '-':   sercmd_value = - sercmd_value;
					sercmd_hasvalue = 0;
					return;
		case ' ':   if (!sercmd_hasvalue)
						return;
					// fall through
		case ',':   sercmd_buffer[sercmd_fill++] = sercmd_value;
					sercmd_hasvalue = 0;
					return;
	}
	if (sercmd_hasvalue) {
		Serial.print("Unrecognized character: ");
		Serial.println(ch);
		sercmd_reset();
		return;
	}

	Command* p;
	for (p = cmds; ; ++p) {
		char code = pgm_read_byte(&p->code);
		if (code == 0)
			break;
		if (ch == code) {
			uint8_t bytes = pgm_read_byte(&p->bytes);
			if (sercmd_fill < bytes) {
				Serial.print("Not enough data: ");
				showNibble(sercmd_fill);
				Serial.print(", need ");
				showNibble(bytes);
				Serial.println(" bytes");
			} else {
				memset(sercmd_buffer + sercmd_fill, 0, sercmd_top - sercmd_fill);
				((void (*)(void)) pgm_read_word(&p->fun))();
			}
			sercmd_reset();
			return;
		}
	}

	Serial.println("");

	//uart_putc( (unsigned char)ch );
}

void uart_v_get(void* ptr, uint8_t len) {
	memcpy(ptr, sercmd_buffer + sercmd_next, len);
	sercmd_next += len;
}

void uart_cchar_get(const char* v) {
	uint8_t offset = sercmd_buffer[sercmd_next++];
	v = sercmd_top <= offset && offset < sercmd_limit ? (char*) sercmd_buffer + offset : "";
}


#endif

