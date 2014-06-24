/*
	boilerplate for ATmega node with Serial interface
*/
#include <DotmpeLib.h>


/** Globals and sketch configuration  */
#define DEBUG           1 /* Enable trace statements */
#define SERIAL          1 /* Enable serial */
							
#define _DHT            0
#define DHT_HIGH        1   // enable for DHT22/AM2302, low for DHT11
#define _DS             0
#define _LCD84x48       0
#define _NRF24          0
							
#define MAXLENLINE      79
							

static String sketch = "X-SensorNode";
static String version = "0";

static int tick = 0;
static int pos = 0;

/* IO pins */
static const byte ledPin = 13;

MpeSerial mpeser (57600);


#if _DHT
/* DHT temp/rh sensor 
 - AdafruitDHT
*/

#endif //_DHT
#if _LCD84x48
#endif //_LCD84x48
#if _DS
/* Dallas OneWire bus with registration for DS18B20 temperature sensors */

#endif // _DS
#if _NRF24
/* nRF24L01+: nordic 2.4Ghz digital radio  */

#endif //_NRF24


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


#if _DS
/* Dallas DS18B20 thermometer routines */

#endif //_DS
#if _NRF24
/* Nordic nRF24L01+ routines */

#endif //_NRF24


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
	debug_ticks();
	//serialFlush();
}

