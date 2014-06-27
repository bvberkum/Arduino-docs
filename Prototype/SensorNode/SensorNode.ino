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

#endif //_DS
#if _NRF24
/* nRF24L01+: nordic 2.4Ghz digital radio  */

#endif //_NRF24
#if _HMC5883L
/* Digital magnetometer I2C module */

#endif //_HMC5883L


/** AVR routines */

int freeRam () {
	extern int __heap_start, *__brkval; 
	int v;
	return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

int usedRam () {
	return SRAM_SIZE - freeRam();
}


/** ATmega routines */

double internalTemp(void)
{
	unsigned int wADC;
	double t;

	// The internal temperature has to be used
	// with the internal reference of 1.1V.
	// Channel 8 can not be selected with
	// the analogRead function yet.

	// Set the internal reference and mux.
	ADMUX = (_BV(REFS1) | _BV(REFS0) | _BV(MUX3));
	ADCSRA |= _BV(ADEN);  // enable the ADC

	delay(20);            // wait for voltages to become stable.

	ADCSRA |= _BV(ADSC);  // Start the ADC

	// Detect end-of-conversion
	while (bit_is_set(ADCSRA,ADSC));

	// Reading register "ADCW" takes care of how to read ADCL and ADCH.
	wADC = ADCW;

	// The offset of 324.31 could be wrong. It is just an indication.
	t = (wADC - 311 ) / 1.22;

	// The returned temperature is in degrees Celcius.
	return (t);
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
#if _HMC5883L
/* Digital magnetometer I2C module */

}
#endif //_HMC5883L


/* Initialization routines */

void doConfig(void)
{
}

void initLibs()
{
#if _DHT
	dht.begin();
#endif

#if _HMC5883L
#endif //_HMC5883L
}


/* Run-time handlers */

void doReset(void)
{
	tick = 0;
}

bool doAnnounce()
{
}

void doMeasure()
{
}

// periodic report, i.e. send out a packet and optionally report on serial port
void doReport(void)
{
}


/* Main */

void setup(void)
{
#if SERIAL
	mpeser.begin();
	mpeser.startAnnounce(sketch, version);
	serialFlush();
#endif

	initLibs();

	doReset();
}

void loop(void)
{
	blink(ledPin, 1, 15);
	debug_ticks();
	serialFlush();
}

