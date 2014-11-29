/*

 This'll initialize the nRF24L01, set it to listening and then
 dump register values to serial.

 If it is hooked up correctly, it'll show actual values--no zero's.

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
#define SRAM_SIZE       0x800


static String sketch = "RF24Test";
static String version = "0";
static String node = "rf24test";

static int tick = 0;
static int pos = 0;

/* IO pins */
static const byte ledPin = 13;
static const byte rf24_ce = 9;
static const byte rf24_csn = 8;

MpeSerial mpeser (57600);

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
	return SRAM_SIZE - freeRam();
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



/* Initialization routines */

void doConfig(void)
{
}

void initLibs()
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
	radio.openReadingPipe(1,pipes[1]);
	radio.startListening();
#endif //_NRF24
}


/* Main */

void setup(void)
{
	mpeser.begin();
	mpeser.startAnnounce(sketch, version);
	serialFlush();

	Serial.print("SRAM used: ");
	Serial.println(usedRam());

	initLibs();

	Serial.print("Init done, resetting libs..");

	doReset();

	Serial.println("OK\n\r[RF24/examples/GettingStarted/]");

#if _NRF24
	Serial.print("Radio details:");
	radio.printDetails();
	Serial.println();
#endif

	Serial.print("SRAM used: ");
	Serial.println(usedRam());
}

void loop(void)
{
	
}
