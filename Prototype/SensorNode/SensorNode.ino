/*
	boilerplate for ATmega node with Serial interface
	uses JeeLib scheduler like JeeLab's roomNode
*/
#include <DotmpeLib.h>
#include <JeeLib.h>


/** Globals and sketch configuration  */
#define DEBUG           1 /* Enable trace statements */
#define SERIAL          1 /* Enable serial */
							
#define _DHT            0
#define DHT_HIGH        1   // enable for DHT22/AM2302, low for DHT11
#define _DS             0
#define _LCD84x48       0
#define _NRF24          0
							
#define SMOOTH          5   // smoothing factor used for running averages
							
#define MAXLENLINE      79
							


String sketch = "X-SensorNode";
String version = "0";

int tick = 0;
int pos = 0;


/* IO pins */
static const byte ledPin = 13;

MpeSerial mpeser (57600);


/* Scheduled tasks */
enum { 
	HANDSHAKE,
	MEASURE,
	REPORT,
	STDBY };
// Scheduler.pollWaiting returns -1 or -2
static const char WAITING = 0xFF; // -1: waiting to run
static const char IDLE = 0xFE; // -2: no tasks running

static word schedbuf[STDBY];
Scheduler scheduler (schedbuf, STDBY);
// has to be defined because we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

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


/* Report variables */

static byte reportCount;    // count up until next report, i.e. packet send

struct {
} payload;

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

void blink(int led, int count, int length, int length_off=-1) {
	for (int i=0;i<count;i++) {
		digitalWrite (led, HIGH);
		delay(length);
		digitalWrite (led, LOW);
		(length_off > -1) ? delay(length_off) : delay(length);
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


// utility code to perform simple smoothing as a running average
static int smoothedAverage(int prev, int next, byte firstTime =0) {
	if (firstTime)
		return next;
	return ((SMOOTH - 1) * prev + next + SMOOTH / 2) / SMOOTH;
}

void debugline(String msg) {
#if DEBUG
	Serial.println(msg);
#endif
}


/* Peripheral hardware routines */

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

	reportCount = REPORT_EVERY;     // report right away for easy debugging
	//scheduler.timer(HANDSHAKE, 0);
	scheduler.timer(MEASURE, 0);    // start the measurement loop going
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

void runScheduler(char task)
{
	switch (task) {

		case HANDSHAKE:
			Serial.println("HANDSHAKE");
			Serial.print(strlen(node_id));
			Serial.println();
			if (strlen(node_id) > 0) {
				Serial.print("Node: ");
				Serial.println(node_id);
				//scheduler.timer(MEASURE, 0);
			} else {
				doAnnounce();
				//scheduler.timer(HANDSHAKE, 100);
			}
			serialFlush();
			break;

		case MEASURE:
			// reschedule these measurements periodically
			debugline("MEASURE");
			scheduler.timer(MEASURE, MEASURE_PERIOD);
			doMeasure();
			// every so often, a report needs to be sent out
			if (++reportCount >= REPORT_EVERY) {
				reportCount = 0;
				scheduler.timer(REPORT, 0);
			}
			serialFlush();
			break;

		case REPORT:
			debugline("REPORT");
//			payload.msgtype = REPORT_MSG;
			doReport();
			serialFlush();
			break;

		case STDBY:
			debugline("STDBY");
			serialFlush();
			break;

		case WAITING:
			scheduler.timer(STDBY, STDBY_PERIOD);
			break;
		
		case IDLE:
			scheduler.timer(STDBY, STDBY_PERIOD);
			break;
		
	}
}


/* InputParser handlers */


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
	char task = scheduler.pollWaiting();
	if (task == 0xFF) {} // -1
	else if (task == 0xFE) {} // -2
	else runScheduler(task);
}

