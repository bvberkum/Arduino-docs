/*
*/
#include <DotmpeLib.h>
#include <JeeLib.h>


/* *** Globals and sketch configuration *** */
#define DEBUG           1 /* Enable trace statements */
#define DEBUG_MEASURE   1
#define DEBUG_RH    1
#define SERIAL          1 /* Enable serial */
							
#define _DHT        1    // define to use JeeLib DHT for DHT

#define _MEM        1
#define REPORT_EVERY    5   // report every N measurement cycles
#define SMOOTH          5   // smoothing factor used for running averages
#define MEASURE_PERIOD  50 // how often to measure, in tenths of seconds

#define RETRY_PERIOD    10   // how soon to retry if ACK didn't come in
#define RETRY_LIMIT     2   // maximum number of times to retry
#define ACK_TIME        10  // number of milliseconds to wait for an ack
#define RADIO_SYNC_MODE 2
							
#define MAXLENLINE      79
							
#if defined(__AVR_ATtiny84__)
#define SRAM_SIZE       512
#elif defined(__AVR_ATmega168__)
#define SRAM_SIZE       1024
#elif defined(__AVR_ATmega328__) || defined(__AVR_ATmega328P__)
#define SRAM_SIZE       2048
#endif
							
#if SERIAL || DEBUG

const String sketch = "DHT11Test_JeeLib";
const int version = 0;

int pos = 0;


/* IO pins */
#if _DHT
#define DHT_PIN     14   // DIO for DHTxx
DHTxx dht(DHT_PIN);
#endif


enum { ANNOUNCE, DISCOVERY, MEASURE, REPORT, TASK_END };
static word schedbuf[TASK_END];
Scheduler scheduler (schedbuf, TASK_END);
static byte reportCount;    // count up until next report, i.e. packet send

enum { ANNOUNCE_MSG, REPORT_MSG };

// has to be defined because we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }


/** Generic routines */

#if SERIAL || DEBUG
static void serialFlush () {
    #if ARDUINO >= 100
        Serial.flush();
    #endif  
    delay(2); // make sure tx buf is empty before going back to sleep
}
#endif

// utility code to perform simple smoothing as a running average
static int smoothedAverage(int prev, int next, byte firstTime =0) {
	if (firstTime)
		return next;
	return ((SMOOTH - 1) * prev + next + SMOOTH / 2) / SMOOTH;
}

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

int freeRam () {
	extern int __heap_start, *__brkval; 
	int v; 
	return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

/* *** Initialization routines *** {{{ */

void doConfig(void)
{
	/* load valid config or reset default config */
	if (!loadConfig(static_config)) {
		writeConfig(static_config);
	}
}

void initLibs()
{
	rf12_initialize(1, RF12_868MHZ, 4);
}

/* }}} *** */

/* *** Run-time handlers *** {{{ */

void doReset(void)
{
	tick = 0;

	doConfig();

	reportCount = REPORT_EVERY;     // report right away for easy debugging
	scheduler.timer(ANNOUNCE, 0);
}

bool doAnnounce()
{
}


// radio announce packed payload
struct {
#if _DHT
	byte rhum     :7;  // humidity: 0..100
	int temp     :10; // temperature: -500..+500 (tenths)
#endif
	int ctemp    :10; // temperature: -500..+500 (tenths)
#if _MEM
	int memfree;
#endif
	byte lobat  :1;  // supply voltage dropped under 3.1V: 0..1
} payload;

static bool doAnnounce() {
/* see CarrierCase */
#if SERIAL || DEBUG
	Serial.println();
#if _DHT
	Serial.print("dht_temp dht_rhum ");
#endif
	Serial.print("attemp ");
#if _MEM
	Serial.print("memfree ");
#endif
	Serial.print("lobat ");
	serialFlush();
#endif // SERIAL || DEBUG
	return true;
}

static void doMeasure() {
	byte firstTime = payload.ctemp == 0; // special case to init running avg

	int ctemp = internalTemp() * 10;
	payload.ctemp = smoothedAverage(payload.ctemp, ctemp, firstTime);
#if SERIAL && DEBUG_MEASURE
	Serial.print("AVR T new/avg ");
	Serial.print(ctemp);
	Serial.print(' ');
	Serial.println(payload.ctemp);
#endif

#if _DHT
	int t, h;
	bool v = dht.reading(t, h);

	if (!v) {
#if SERIAL || DEBUG
		Serial.println(F("Failed to read DHT humidity"));
#endif
	} else {
		h = h/10;
		payload.rhum = smoothedAverage(payload.rhum, h, firstTime);
#if SERIAL && ( DEBUG_MEASURE || DEBUG_RH )
		Serial.print(F("DHT RH new/avg "));
		Serial.print(h);
		Serial.print(' ');
		Serial.println((int)payload.rhum);
#endif
	}
	if (!v) {
#if SERIAL || DEBUG
		Serial.println(F("Failed to read DHT temperature"));
#endif
	} else {
		payload.temp = smoothedAverage(payload.temp, t, firstTime);
#if SERIAL && ( DEBUG_MEASURE || DEBUG_T )
		Serial.print(F("DHT T new/avg "));
		Serial.print(t);
		Serial.print(' ');
		Serial.println(payload.temp);
#endif
	}
#endif

#if _MEM
	payload.memfree = freeRam();
#if SERIAL && DEBUG_MEASURE
	Serial.print("MEM free ");
	Serial.println(payload.memfree);
#endif
#endif
}

static void doReport() {
	rf12_sleep(RF12_WAKEUP);
	rf12_sendNow(0, &payload, sizeof payload);
	rf12_sendWait(RADIO_SYNC_MODE);
	rf12_sleep(RF12_SLEEP);
#if SERIAL || DEBUG
	Serial.println();
#if _DHT
	Serial.print((int) payload.temp);
	Serial.print(' ');
	Serial.print((int) payload.rhum);
	Serial.print(' ');
#endif
	Serial.print((int) payload.ctemp);
	Serial.print(' ');
#if _MEM
	Serial.print((int) payload.memfree);
	Serial.print(' ');
#endif
	Serial.print((int) payload.lobat);
	serialFlush();
#endif // SERIAL || DEBUG
}


/* }}} *** */

/* *** Main *** {{{ */

void setup(void)
{
#if SERIAL
	mpeser.begin();
	mpeser.startAnnounce(sketch, String(version));
	serialFlush();
#endif

	initLibs();

	doReset();
}

void loop() {
#if DEBUG
	Serial.print('.');
	pos++;
	if (pos > 9) {
		pos = 0;
		Serial.println();
	}
	serialFlush();
#endif

	switch (scheduler.pollWaiting()) {

		case ANNOUNCE:
			if (doAnnounce()) {
				scheduler.timer(MEASURE, 0);
			} else {
				scheduler.timer(ANNOUNCE, 100);
			}
			break;

		case MEASURE:
			// reschedule these measurements periodically
			scheduler.timer(MEASURE, MEASURE_PERIOD);
			doMeasure();
			serialFlush();
			// every so often, a report needs to be sent out
			if (++reportCount >= REPORT_EVERY) {
				reportCount = 0;
				scheduler.timer(REPORT, 0);
			}
			break;

		case REPORT:
//			payload.msgtype = REPORT_MSG;
			doReport();
			break;

		case DISCOVERY:
			Serial.print("discovery? ");
			break;

		case TASK_END:
			Serial.print("task? ");
			break;
	}
}

