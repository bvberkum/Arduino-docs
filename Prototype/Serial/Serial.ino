/*
	boilerplate for ATmega node with Serial interface
*/
#include <DotmpeLib.h>


/** Globals and sketch configuration  */
#define DEBUG           1 /* Enable trace statements */
#define SERIAL          1 /* Enable serial */
							
							
#define MAXLENLINE      79
							

static String sketch = "X-Serial";
static String version = "0";

static int tick = 0;
static int pos = 0;

/* IO pins */
static const byte ledPin = 13;

MpeSerial mpeser (57600);


/** AVR routines */


/** ATmega routines */


/** Generic routines */

static void serialFlush () {
#if SERIAL
#if ARDUINO >= 100
	Serial.flush();
#endif
	delay(2); // make sure tx buf is empty before going back to sleep
#endif
}

void blink(int led, int count, int length, int length_off=0) {
	for (int i=0;i<count;i++) {
		digitalWrite (led, HIGH);
		delay(length);
		digitalWrite (led, LOW);
		delay(length);
		(length_off > 0) ? delay(length_off) : delay(length);
	}
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

/* Initialization routines */

void doConfig(void)
{
}

void setupLibs()
{
}


/* Run-time handlers */

void doReset(void)
{
	tick = 0;
}

bool doAnnounce()
{
}


/* Main */

void setup(void)
{
	mpeser.begin();
	mpeser.startAnnounce(sketch, version);
	serialFlush();

	setupLibs();

	doReset();
}

void loop(void)
{
	//blink(ledPin, 1, 15);
	//debug_ticks();
	//serialFlush();
}
