/*
 This'll initialize the nRF24L01, set it to listening and then
 dump register values to serial.
*/
#include <DotmpeLib.h>
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"


/** Globals and sketch configuration  */
							
#define MAXLENLINE      79
							

static String sketch = "RF24Test";
static String version = "0";
static String node = "rf24test";

static int tick = 0;
static int pos = 0;

static const byte ledPin = 13;

MpeSerial mpeser (57600);

// Set up nRF24L01 radio on SPI bus plus two extra pins
RF24 radio(9, 8); /* CE, CSN */

// nRF24L01 addresses: one for broadcast, one for listening
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };


/** AVR routines */

int freeRam () {
	extern int __heap_start, *__brkval; 
	int v;
	return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

int usedRam () {
	return 0x800 - freeRam();
}


/* Initialization routines */

void doConfig(void)
{
}

void setupLibs()
{
	radio.begin();

	printf_begin();
	printf("\n\rRF24/examples/GettingStarted/\n\r");
}

void reset(void)
{
	tick = 0;
	radio.openReadingPipe(1,pipes[1]);
	radio.startListening();
}


/* Run-time handlers */


/* Main */

void setup(void)
{
	mpeser.begin();
	mpeser.startAnnounce(sketch, version);
	//serialFlush();

	Serial.print("SRAM used: ");
	Serial.println(usedRam());

	setupLibs();

	reset();

	radio.printDetails();

	Serial.print("SRAM used: ");
	Serial.println(usedRam());
}

void loop(void)
{
	
}
