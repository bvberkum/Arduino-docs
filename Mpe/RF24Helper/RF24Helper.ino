/* nRF24L01+ testing
 *
 * Work in progress.
 *
 * - 
 */
#include <DotmpeLib.h>
#include <SPI.h>
//#include "nRF24L01.h"
#include <RF24.h>

#include "printf.h"


/** Globals and sketch configuration  */
#define DEBUG           1 /* Enable trace statements */
#define SERIAL          1 /* Enable serial */
							
#define _NRF24          1
							
#define MAXLENLINE      79
							

static String sketch = "RF24Test";
static String version = "0";

static int tick = 0;
static int pos = 0;

/* IO pins */
static const byte ledPin = 13; // XXX shared with nrf24 SCK
static const byte rf24_ce = 9;
static const byte rf24_csn = 8;

MpeSerial mpeser (57600);

/* Roles supported by this sketch */
typedef enum {
	role_reporter = 1,  /* start enum at 1, 0 is reserved for invalid */
	role_logger
} Role;

const char* role_friendly_name[] = { 
	"invalid", 
	"Reporter", 
	"Logger"
};

Role role;// = role_logger;

#if _NRF24
/* nRF24L01+: nordic 2.4Ghz digital radio  */

// Set up nRF24L01 radio on SPI bus plus two extra pins
RF24 radio(rf24_ce, rf24_csn); /* CE, CSN */

// nRF24L01 addresses: one for broadcast, one for listening
const uint64_t pipes[2] = { 
	0xF0F0F0F0E1LL, /* dest id: central link node */
	0xF0F0F0F0D2LL /* src id: local node */
};
#endif //_NRF24


/** AVR routines */

int freeRam () {
	extern int __heap_start, *__brkval; 
	int v;
	return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

int usedRam () {
	return 0x800 - freeRam();
}


/** Generic routines */

static void serialFlush () {
#if SERIAL
#if ARDUINO >= 100
	Serial.flush();
#endif
	delay(2); // make sure tx buf is empty before going back to sleep
#endif
}

void debug_ticks(void)
{
#if SERIAL && DEBUG
	tick++;
	if ((tick % 20) == 0) {
		Serial.print('.');
		pos++;
	}
	if (pos > MAXLENLINE) {
		pos = 0;
		Serial.println();
	}
	serialFlush();
#endif
}

#if _NRF24
/* Nordic nRF24L01+ routines */

void rf24_init()
{
	/* Setup and configure rf radio */
	radio.setRetries(15,15); /* delay, number */
	radio.setDataRate(RF24_250KBPS);
	/* Start radio */
	radio.openWritingPipe(pipes[0]);
	radio.openReadingPipe(1,pipes[1]);
	radio.startListening();
#if SERIAL && DEBUG
	radio.printDetails();
	serialFlush();
#endif
}

void radio_run()
{
	if (radio.available()) {
		unsigned long got_time;
		bool done = false;
		while (!done)
		{
			done = radio.read( &got_time, sizeof(unsigned long) );
			if (!done) {
#if SERIAL && DEBUG
				printf("Payload read failed %s...", done);
#endif
				return;
			}

#if SERIAL && DEBUG
			printf("Got payload %lu...", got_time);
#endif
			//delay(20);
			radio.stopListening();
			radio.write( &got_time, sizeof(unsigned long) );
#if SERIAL && DEBUG
			printf("Sent response.\n\r");
#endif
			radio.startListening();
		}
	}
}
#endif //_NRF24


/* Initialization routines */

void doConfig(void)
{
}

void setupLibs()
{
#if _NRF24
	radio.begin();
#endif //_NRF24

#if SERIAL && DEBUG
	printf_begin();
#endif
}


/* Run-time handlers */

void doReset(void)
{
	tick = 0;

#if _NRF24
	rf24_init();
#endif //_NRF24
}


/* Main */

void setup(void)
{
#if SERIAL
	mpeser.begin();
	mpeser.startAnnounce(sketch, version);
	serialFlush();

#if DEBUG
	Serial.print("SRAM used: ");
	Serial.println(usedRam());
#endif
#endif

	setupLibs();

	doReset();
}

void loop(void)
{
	debug_ticks();
#if _NRF24
	radio_run();
#endif //_NRF24
	delay(10);
}

