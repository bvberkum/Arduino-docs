/*

	AdafruitDHT 
	based on 
		- ThermoLog84x48 but doesnt use Lcd84x48
		- AtmegaEEPROM
*/

#include <DotmpeLib.h>
#include <JeeLib.h>
#include <DHT.h> // Adafruit DHT


/* *** Globals and sketch configuration *** */
#define DEBUG           1 /* Enable trace statements */
#define SERIAL          1 /* Enable serial */
#define DEBUG_MEASURE   0
							
#define _RFM12B         0
#define _MEM            1   // Report free memory 
#define _DHT            1
#define DHT_HIGH        1   // enable for DHT22/AM2302, low for DHT11
#define _RFM12LOBAT     0   // Use JeeNode lowbat measurement
							
#define MEASURE_PERIOD  50  // how often to measure, in tenths of seconds
#define REPORT_EVERY    5   // report every N measurement cycles
#define SMOOTH          5   // smoothing factor used for running averages
#define STDBY_PERIOD    60
							
#define MAXLENLINE      79
#define SRAM_SIZE       0x800 // atmega328, for debugging
							
#if defined(__AVR_ATtiny84__)
#define SRAM_SIZE       512
#elif defined(__AVR_ATmega168__)
#define SRAM_SIZE       1024
#elif defined(__AVR_ATmega328__) || defined(__AVR_ATmega328P__)
#define SRAM_SIZE       2048
#endif
							

const String sketch = "AdafruitDHT";
const int version = 0;

char node[] = "adadht";
// determined upon handshake 
char node_id[7];

int tick = 0;
int pos = 0;

/* IO pins */
static const byte ledPin = 13;
#if _DHT
static const byte DHT_PIN = 7;
#endif

MpeSerial mpeser (57600);

// has to be defined because we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

/* Command parser */
const extern InputParser::Commands cmdTab[] PROGMEM;
InputParser parser (50, cmdTab);

/* *** Device params *** {{{ */

#if _DHT
/* DHT temp/rh sensor 
 - AdafruitDHT
*/
//DHT dht(DHT_PIN, DHTTYPE); // Adafruit DHT
//DHTxx dht (DHT_PIN); // JeeLib DHT
//#define DHTTYPE DHT11   // DHT 11 
//#define DHTTYPE DHT22   // DHT 22  (AM2302)
#if DHT_HIGH
DHT dht (DHT_PIN, DHT22);
#else
DHT dht (DHT_PIN, DHT11);
#endif
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

/* }}} */

/* *** Report variables *** {{{ */

static byte reportCount;    // count up until next report, i.e. packet send

// This defines the structure of the packets which get sent out by wireless:
struct {
#if _DHT
	int rhum    :7;  // 0..100 (avg'd)
#if DHT_HIGH
/* DHT22/AM2302: 20% to 100% @ 2% rhum, -40C to 80C @ ~0.5 temp */
	int temp    :10; // -500..+500 (int value of tenths avg)
#else
/* DHT11: 20% to 80% @ 5% rhum, 0C to 50C @ ~2C temp */
	int temp    :10; // -500..+500 (tenths, .5 resolution)
#endif // DHT_HIGH
#endif //_DHT
	int ctemp   :10; // atmega temperature: -500..+500 (tenths)
#if _MEM
	int memfree :16;
#endif
#if _RFM12LOBAT
#endif
} payload;

/* }}} */

/* *** Scheduled tasks *** {{{ */

enum { MEASURE, REPORT, STDBY };
// Scheduler.pollWaiting returns -1 or -2
static const char WAITING = 0xFF; // -1: waiting to run
static const char IDLE = 0xFE; // -2: no tasks running

static word schedbuf[STDBY];
Scheduler scheduler (schedbuf, STDBY);

/* }}} *** */

/* *** EEPROM config *** {{{ */


/* }}} *** */

/* *** AVR routines *** {{{ */

int freeRam () {
	extern int __heap_start, *__brkval; 
	int v;
	return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

int usedRam () {
	return SRAM_SIZE - freeRam();
}

/* }}} *** */

/* *** ATmega routines *** {{{ */

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

/* }}} *** */

/* *** Generic routines *** {{{ */

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

/* }}} *** */

/* *** Peripheral hardware routines *** {{{ */

// none


/* }}} *** */

/* *** Initialization routines *** {{{ */

void doConfig(void)
{
}

void initLibs()
{
#if _DHT
	dht.begin();
#endif

#if _RFM12B
#endif
}

/* }}} *** */

/* InputParser handlers */

static void helpCmd() {
	Serial.println("Help!");
}

static void handshakeCmd() {
	int v;
	char buf[7];
	node.toCharArray(buf, 7);
	parser >> v;
	sprintf(node_id, "%s%i", buf, v);
	Serial.print("handshakeCmd:");
	Serial.println(node_id);
	serialFlush();
}

InputParser::Commands cmdTab[] = {
	{ 'h', 0, helpCmd },
	{ 'A', 0, handshakeCmd }
};


/* *** Run-time handlers *** {{{ */

void doReset(void)
{
	tick = 0;

	doConfig();

#if _NRF24
	rf24_init();
#endif //_NRF24

	reportCount = REPORT_EVERY;     // report right away for easy debugging
	scheduler.timer(MEASURE, 0);    // start the measurement loop going
}

bool doAnnounce()
{
/* see CarrierCase */
#if SERIAL && DEBUG
	mpeser.startAnnounce(sketch, version);
#if _DHT
#if DHT_HIGH
	Serial.print("dht22_temp dht22_rhum ");
#else
	Serial.print("dht11_temp dht11_rhum ");
#endif
#endif
	Serial.print("attemp ");
#if _MEM
	Serial.print("memfree ");
#endif
#if _RFM12LOBAT
#endif
	
	serialFlush();

	parser.poll();
	delay(100);
	return false;

	//SerMsg msg;

	//int wait = 500;
	//mpeser.readHandshakeWaiting(msg, wait);
	//Serial.println(msg.type, HEX);
	//Serial.println(msg.value, HEX);
	//serialFlush();

	//return false;
#endif // SERIAL && DEBUG
}

// readout all the sensors and other values
void doMeasure()
{
	byte firstTime = payload.ctemp == 0; // special case to init running avg

	int ctemp = internalTemp();
	payload.ctemp = smoothedAverage(payload.ctemp, ctemp, firstTime);
#if SERIAL && DEBUG_MEASURE
	Serial.println();
	Serial.print("AVR T new/avg ");
	Serial.print(ctemp);
	Serial.print(' ');
	Serial.println(payload.ctemp);
#endif

#if _RFM12LOBAT
#endif

#if _DHT
	float h = dht.readHumidity();
	float t = dht.readTemperature();
	if (isnan(h)) {
#if SERIAL && DEBUG
		Serial.println(F("Failed to read DHT11 humidity"));
#endif
	} else {
		payload.rhum = smoothedAverage(payload.rhum, round(h), firstTime);
#if SERIAL && DEBUG_MEASURE
		Serial.print(F("DHT RH new/avg "));
		Serial.print(h);
		Serial.print(' ');
		Serial.println(payload.rhum);
#endif
	}
	if (isnan(t)) {
#if SERIAL && DEBUG
		Serial.println(F("Failed to read DHT11 temperature"));
#endif
	} else {
		payload.temp = smoothedAverage(payload.temp, (int)(t * 10), firstTime);
#if SERIAL && DEBUG_MEASURE
		Serial.print(F("DHT T new/avg "));
		Serial.print(t);
		Serial.print(' ');
		Serial.println(payload.temp);
#endif
	}
#endif // _DHT

#if _MEM
	payload.memfree = freeRam();
#if SERIAL && DEBUG_MEASURE
	Serial.print("MEM free ");
	Serial.println(payload.memfree);
#endif
#endif
}

// periodic report, i.e. send out a packet and optionally report on serial port
void doReport(void)
{
#if SERIAL || DEBUG
	Serial.print(node);
#if _DHT
	Serial.print(' ');
	Serial.print((int) payload.temp);
	Serial.print(' ');
	Serial.print((int) payload.rhum);
#endif
	Serial.print(' ');
	Serial.print((int) payload.ctemp);
#if _MEM
	Serial.print(' ');
	Serial.print((int) payload.memfree);
#endif
#if _RFM12LOBAT
#endif
	Serial.println();
#endif // SERIAL || DEBUG
}

void runScheduler(char task)
{
	switch (task) {

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


/* }}} *** */

/* *** Main *** {{{ */

void setup(void)
{
#if SERIAL
	mpeser.begin();
	mpeser.startAnnounce(sketch, String(version));
#if DEBUG || _MEM
	Serial.print(F("Free RAM: "));
	Serial.println(freeRam());
#endif
	serialFlush();
#endif

	initLibs();

	doReset();
}

void loop(void)
{
	//doMeasure();
	//doReport();
	//delay(1500);
	//return;
	
	blink(ledPin, 1, 15);
	debug_ticks();
	serialFlush();
	char task = scheduler.pollWaiting();
	runScheduler(task);
}

/* }}} *** */

