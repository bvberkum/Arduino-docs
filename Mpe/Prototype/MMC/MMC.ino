/* 
   Atmega MMC/SD card routines 

   Need library. See Misc/MMC working but largish?

 */
#include <DotmpeLib.h>


/** Globals and sketch configuration  */
#define DEBUG           1 /* Enable trace statements */
#define SERIAL          1 /* Enable serial */
							
#define MAXLENLINE      79
							

static String sketch = "X-MMC";
static String version = "0";

static int tick = 0;
static int pos = 0;

static const byte ledPin = 13;

MpeSerial mpeser (57600);



/** AVR routines */

int freeRam () {
	extern int __heap_start, *__brkval; 
	int v;
	return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
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

/* Initialization routines */

static void doConfig(void)
{
}

void setupLibs()
{
}

static void reset(void)
{
	tick = 0;
}


/* Run-time handlers */

static bool doAnnounce()
{
}

static void doMeasure()
{
}

static void doReport(void)
{
}

static void runCommand()
{
}

void runScheduler(char task)
{
	switch (task) {
	}
}

/* Main */

void setup(void)
{
	mpeser.begin();
	mpeser.startAnnounce(sketch, version);

	setupLibs();
	serialFlush();

	reset();
}

void loop(void)
{
	//blink(ledPin, 1, 15);
	//debug_ticks();
	//serialFlush();
}
