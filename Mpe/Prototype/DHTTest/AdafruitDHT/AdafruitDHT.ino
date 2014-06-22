/*

	AdafruitDHT 
	based on 
		- ThermoLog84x48 but doesnt use Lcd84x48

#include <DotmpeLib.h>
#include <JeeLib.h>
#include <DHT.h> // Adafruit DHT


/** Globals and sketch configuration  */
#define DEBUG           1 /* Enable trace statements */
#define SERIAL          1 /* Enable serial */
#define DEBUG_MEASURE   0
							
#define _RFM12           1
#define _MEM            1   // Report free memory 
#define _DHT            1
#define DHT_HIGH        1   // enable for DHT22/AM2302, low for DHT11
#define _RFM12LOBAT      1   // Use JeeNode lowbat measurement
							
#define REPORT_EVERY    5   // report every N measurement cycles
#define SMOOTH          5   // smoothing factor used for running averages
#define MEASURE_PERIOD  50  // how often to measure, in tenths of seconds
#define RETRY_PERIOD    10  // how soon to retry if ACK didn't come in
#define RETRY_LIMIT     2   // maximum number of times to retry
#define ACK_TIME        10  // number of milliseconds to wait for an ack
#define RADIO_SYNC_MODE 2
#define MAXLENLINE      79
							

static String sketch = "AdafruitDHT";
static String version = "0";
static String node = "adadht";

// determined upon handshake 
char node_id[7];

static int tick = 0;
static int pos = 0;

static const byte ledPin = 13;
#if _DHT
static const byte DHT_PIN = 7;
#endif

MpeSerial mpeser (57600);

extern InputParser::Commands cmdTab[] PROGMEM;
InputParser parser (50, cmdTab);

/* Scheduled tasks */
enum { HANDSHAKE, MEASURE, REPORT, TASK_END };
static word schedbuf[TASK_END];
Scheduler scheduler (schedbuf, TASK_END);
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

#if _DHT
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

/* Report variables */

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
#endif
#endif
	int ctemp   :10; // atmega temperature: -500..+500 (tenths)
#if _MEM
	int memfree :16;
#endif
#if _RFM12LOBAT
	byte lobat  :1;  // supply voltage dropped under 3.1V: 0..1
#endif
} payload;

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

// utility code to perform simple smoothing as a running average
static int smoothedAverage(int prev, int next, byte firstTime =0) {
	if (firstTime)
		return next;
	return ((SMOOTH - 1) * prev + next + SMOOTH / 2) / SMOOTH;
}

void debug(String msg) {
#if DEBUG
	Serial.println(msg);
#endif
}

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

void setup_peripherals()
{
#if _DHT
	dht.begin();
#endif

#if _RFM12
	rf12_initialize(1, RF12_868MHZ, 4);
#endif
}

static bool doAnnounce()
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
	Serial.print("lobat ");
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
#endif // SERIAL 
}

static void doMeasure()
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
	payload.lobat = rf12_lowbat();
#if SERIAL && DEBUG_MEASURE
	if (payload.lobat) {
		Serial.println("Low battery");
	}
#endif
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
static void doReport(void)
{
#if _RFM12
	rf12_sleep(RF12_WAKEUP);
	rf12_sendNow(0, &payload, sizeof payload);
	rf12_sendWait(RADIO_SYNC_MODE);
	rf12_sleep(RF12_SLEEP);
#endif
#if SERIAL || DEBUG
	Serial.println();
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
	Serial.print(' ');
	Serial.print((int) payload.lobat);
#endif
	Serial.println();
#endif // SERIAL || DEBUG
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
				scheduler.timer(MEASURE, 0);
			} else {
				doAnnounce();
				scheduler.timer(HANDSHAKE, 100);
			}
			serialFlush();
			break;

		case MEASURE:
			// reschedule these measurements periodically
			debug("MEASURE");
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
			debug("REPORT");
//			payload.msgtype = REPORT_MSG;
			doReport();
			serialFlush();
			break;

		default:
			Serial.print("0x");
			Serial.print(task, HEX);
			Serial.println(" ?");
			serialFlush();
			break;

	}
}

static void reset(void)
{
	ui_irq = false;
	tick = 0;
	reportCount = REPORT_EVERY;     // report right away for easy debugging
	scheduler.timer(HANDSHAKE, 0);
	//scheduler.timer(MEASURE, 0);    // start the measurement loop going
}

void debug_ticks(void)
{
#if DEBUG
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

/* Main */

void setup(void)
{
	mpeser.begin();
	mpeser.startAnnounce(sketch, version);
	serialFlush();

	setup_peripherals();

	reset();
}

void loop(void)
{
	//doMeasure();
	//doReport();
	//delay(15000);
	//return;
	debug_ticks();
	char task = scheduler.poll();
	if (0 < task && task < 0xFF) {
		runScheduler(task);
	} else {
		blink(ledPin, 1, 15);
		serialFlush();
		char task = scheduler.pollWaiting();
		if (0 < task && task < 0xFF) {
			runScheduler(task);
		}
	}
}
