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

#include <mpelib/ser.h>
#include <mpelib/sercmd.h>
#include <mpelib/avr.h>
#include <mpelib/atmega.h>


/* *** Globals and sketch configuration *** */
#define DEBUG           1 /* Enable trace statements */
#define SERIAL          1 /* Enable serial */
							
#define _MEM            1   // Report free memory 
							
							

const char sketch[] = "AVRNode";
const int version = 0;

const char node[] = "an";
char node_id[7];


/* *** EEPROM config *** {{{ */

#define CONFIG_VERSION "nx1"
#define CONFIG_START 0

typedef struct {
	char node[4];
	int node_id;
	int version;
	char config_id[4];
} Config ;

Config static_config = {
//	/* default values */
	"nx", 0, 0, CONFIG_VERSION
};


/* }}} */

/* InputParser handlers */

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

static void configCmd(void) {
	uart0_puts("c ");
	uart0_putc(static_config.node);
	uart0_putc(' ');
	uart0_putc(static_config.node_id);
	uart0_putc(' ');
	uart0_putc(static_config.version);
	uart0_putc(' ');
	uart0_putc(static_config.config_id);
	uart0_puts(RETURN_NEWLINE);
}

static void configSetNodeCmd(void) {
//	const char *node;
//	uart_cchar_get(node);
//	parser >> node;
//	static_config.node[0] = node[0];
//	static_config.node[1] = node[1];
//	static_config.node[2] = node[2];
//	initConfig();
	uart0_puts("N ");
	uart0_puts(static_config.node);
	uart0_println("");
}

static void configNodeIDCmd(void) {
	uart_v_get(&static_config.node_id, 2);
//	parser >> static_config.node_id;
//	initConfig();
//	uart0_puts("C ");
//	uart0_putc(node_id);
//	//serialFlush();
//	uart0_puts(RETURN_NEWLINE);
}

static void configNodeCmd(void) {
	uart0_puts("n ");
	uart0_puts(node_id);
	uart0_println("");
}

static void configVersionCmd(void) {
	uart0_puts("v ");
	showNibble(static_config.version);
	uart0_println("");
}

static void configEEPROM(void) {
//	int write;
//	parser >> write;
//	if (write) {
//		writeConfig(static_config);
//	} else {
//		loadConfig(static_config);
//		initConfig();
//	}
//	uart0_putc("W ");
//	uart0_println(write);
}

static void eraseEEPROM(void) {
//	uart0_putc("! Erasing EEPROM..");
//	for (int i = 0; i<EEPROM_SIZE; i++) {
//		char b = EEPROM.read(i);
//		if (b != 0x00) {
//			EEPROM.write(i, 0);
//			uart0_putc('.');
//		}
//	}
//	uart0_println(' ');
//	uart0_putc("E ");
//	uart0_println(EEPROM_SIZE);
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

const Command cmdTab[] PROGMEM = {
	{ '?', 0, helpCmd },
	{ 'h', 0, helpCmd },
	{ 'c', 0, configCmd },
	{ 'm', 0, memStat },
	{ 'n', 0, configNodeCmd },
	{ 'v', 0, configVersionCmd },
	{ 'N', 3, configSetNodeCmd },
	{ 'C', 1, configNodeIDCmd },
	{ 'W', 1, configEEPROM },
	{ 'E', 0, eraseEEPROM },
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
	uart0_putc(' ');
	uart0_putint(tmp);
	uart_puts(RETURN_NEWLINE);
#endif
//	serialFlush();
	for(;;) {
		process_uart(cmdTab);
	}
}

