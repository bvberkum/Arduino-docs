#include <stdlib.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/pgmspace.h> //TODO setup PSTR? F()?
#include <string.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <uart/uart.h>

#include <mpelib/ser.h>
#include <mpelib/avr.h>
#include <mpelib/atmega.h>
#include <mpelib/sercmd.h>


/* *** Globals and sketch configuration *** */
#define DEBUG           1 /* Enable trace statements */
#define SERIAL          1 /* Enable serial */
							
#define _MEM            1   // Report free memory 
							

const char sketch[] = "AVRNode";
const int version = 0;

const char node[] = "nx";
char node_id[7];



static void helpCmd(void) {
	uart0_println("n: print Node ID");
	uart0_println("c: print config");
	uart0_println("v: print version");
	uart0_println("m: print free and used memory");
	uart0_println("N: set Node (3 byte char)");
	uart0_println("C: set Node ID (1 byte int)");
	uart0_println("W: load/save EEPROM");
	uart0_println("E: erase EEPROM!");
	uart0_println("?/h: this help");
}

static void memStat(void) {
	int free = freeRam();
	int used = usedRam();
	uart0_puts("m ");
	uart0_putint(free);
	uart0_putc(' ');
	uart0_putint(used);
	uart0_putc(' ');
	uart0_puts(RETURN_NEWLINE);
}

const Commands cmdTab[] PROGMEM = {
	{ '?', 0, helpCmd },
	{ 'h', 0, helpCmd },
	{ 'm', 0, memStat},
	{ 0 }
};

/* *** Main *** */

int main(void)
{
	mpelib_ser_start();
	sei();
	mpelib_ser_announce(sketch, version);
	sercmd_init(50);
	//virtSerial.begin(4800);
#if DEBUG || _MEM
	print_value("RAM", freeRam());
	double tmp = internalTemp();
	print_value("CTEMP", tmp);
	print_value("CTEMP", (int)tmp);
	uart0_putc(' ');
	showByte(tmp);
	uart_puts(RETURN_NEWLINE);
#endif
//	serialFlush();
	for(;;) {
		process_uart(cmdTab);
	}
}


