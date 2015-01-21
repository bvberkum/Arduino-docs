/*

	AdafruitDHT 
	based on 
		- ThermoLog84x48 but doesnt use Lcd84x48
		- AtmegaEEPROM
*/


/* *** Globals and sketch configuration *** */
#define DEBUG           1 /* Enable trace statements */
#define SERIAL          1 /* Enable serial */
#define DEBUG_MEASURE   0
							
#define UART_BAUD_RATE  57600
#define _RFM12B         1
#define _MEM            1   // Report free memory 
#define _DHT            1
#define DHT_HIGH        1   // enable for DHT22/AM2302, low for DHT11
#define _DHT2           0   // 1:DHT11, 2:DHT22
#define _RFM12LOBAT     0   // Use JeeNode lowbat measurement
							
#define MEASURE_PERIOD  300  // how often to measure, in tenths of seconds
#define REPORT_EVERY    1   // report every N measurement cycles
#define SMOOTH          5   // smoothing factor used for running averages
#define STDBY_PERIOD    60
							
#define RADIO_SYNC_MODE 2

#include <JeeLib.h>
// Adafruit DHT
#include <DHT.h>
#include <DotmpeLib.h>
#include <mpelib.h>


const String sketch = "AdafruitDHT";
const int version = 0;

char node[] = "adadht";
// determined upon handshake 
char node_id[7];

/* IO pins */
#if _DHT
static const byte DHT_PIN = 7;
#endif
#if _DHT2
static const byte DHT2_PIN = 6;
#endif
#       define _DBG_LED 13 // SCK


MpeSerial mpeser (57600);

/* *** InputParser {{{ */

extern InputParser::Commands cmdTab[] PROGMEM;
InputParser parser (50, cmdTab);


/* }}} *** */

/* *** Report variables *** {{{ */


static byte reportCount;    // count up until next report, i.e. packet send

// This defines the structure of the packets which get sent out by wireless:
struct {
#if _DHT
	int rhum    :7;  // 0..100 (avg'd)
#if DHT_HIGH
/* DHT22/AM2302: 20% to 100% @ 2% rhum, -40C to 80C @ ~0.5 temp */
	int temp    :10; // -500..+500 (int value of tenths avg)
//#else
///* DHT11: 20% to 80% @ 5% rhum, 0C to 50C @ ~2C temp */
//	int temp    :10; // -500..+500 (tenths, .5 resolution)
#endif // DHT_HIGH
#endif //_DHT

#if _DHT2
	int rhum2    :7;  // 0..100 (avg'd)
	int temp2    :10; // -500..+500 (int value of tenths avg)
#endif //_DHT2

	int ctemp       :10; // atmega temperature: -500..+500 (tenths)
#if _MEM
	int memfree     :16;
#endif
#if _RFM12LOBAT
	byte lobat      :1;  // supply voltage dropped under 3.1V: 0..1
#endif
} payload;


/* *** /Report variables *** }}} */

/* *** Scheduled tasks *** {{{ */

enum {
	MEASURE,
	REPORT,
	TASK_END
};
// Scheduler.pollWaiting returns -1 or -2
static const char WAITING = 0xFF; // -1: waiting to run
static const char IDLE = 0xFE; // -2: no tasks running

static word schedbuf[TASK_END];
Scheduler scheduler (schedbuf, TASK_END);

// has to be defined because we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }


/* *** /Scheduled tasks *** }}} */

/* *** Peripheral devices *** {{{ */

#if LDR_PORT
Port ldr (LDR_PORT);
#endif

#if _DHT
/* DHT temp/rh sensor 
 - AdafruitDHT
*/
//DHT dht(DHT_PIN, DHTTYPE); // Adafruit DHT
//DHTxx dht (DHT_PIN); // JeeLib DHT
//#define DHTTYPE DHT11   // DHT 11 
//#define DHTTYPE DHT22   // DHT 22  (AM2302)
#if DHT_HIGH 
DHT dht (DHT_PIN, DHT21); 
//DHT dht (DHT_PIN, DHT22); 
// AM2301
// DHT21, AM2302?
#else
DHT dht (DHT_PIN, DHT11);
#endif
#endif // DHT

#if _DHT2
DHT dht2 (DHT2_PIN, DHT11);
#endif

#if _LCD84x48
/* Nokkia 5110 display */
#endif // LCD84x48

#if _DS
/* Dallas OneWire bus with registration for DS18B20 temperature sensors */



#endif // DS

#if _NRF24
/* nRF24L01+: nordic 2.4Ghz digital radio  */

#endif // NRF24

#if _RTC
#endif //_RTC

#if _HMC5883L
/* Digital magnetometer I2C module */

#endif // HMC5883L


/* *** /Peripheral devices }}} *** */

/* *** EEPROM config *** {{{ */

/* *** /EEPROM config *** }}} */

/* *** Peripheral hardware routines *** {{{ */

#if LDR_PORT
#endif

/* *** PIR support *** {{{ */
#if PIR_PORT
#endif
/* *** /PIR support *** }}} */

#if _DHT
/* DHT temp/rh sensor 
 - AdafruitDHT
*/

#endif // DHT

#if _RFM12B
/* HopeRF RFM12B 868Mhz digital radio */

#endif //_RFM12B

#if _LCD84x48
#endif // LCD84x48

#if _DS
/* Dallas OneWire bus with registration for DS18B20 temperature sensors */


#endif // DS

#if _NRF24
/* Nordic nRF24L01+ radio routines */

#endif // NRF24 funcs

#if _RTC
#endif //_RTC

#if _HMC5883L
/* Digital magnetometer I2C routines */
#endif //_HMC5883L


/* *** /Peripheral hardware routines }}} *** */

/* *** Initialization routines *** {{{ */

void doConfig(void)
{
}

void initLibs()
{
#if _RFM12B
	rf12_initialize(9, RF12_868MHZ, 5);
//#if SERIAL && DEBUG
//	myNodeID = rf12_config();
//	serialFlush();
//#else
//	myNodeID = rf12_config(0); // don't report info on the serial port
//#endif

	rf12_sleep(RF12_SLEEP); // power down
#endif // RFM12B

#if _DHT
	dht.begin();
#endif
#if _DHT2
	dht2.begin();
#endif
}


/* *** /Initialization routines *** }}} */

/* *** Run-time handlers *** {{{ */

void doReset(void)
{
	doConfig();

#if _DBG_LED
	pinMode(_DBG_LED, OUTPUT);
#endif
	tick = 0;

#if _NRF24
	rf24_init();
#endif //_NRF24

	reportCount = REPORT_EVERY;     // report right away for easy debugging
	scheduler.timer(MEASURE, 0);    // get the measurement loop going
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
#if _DHT2
	Serial.print("dht11_temp dht11_rhum ");
#endif
	Serial.print("attemp ");
#if _MEM
	Serial.print("memfree ");
#endif
#if _RFM12LOBAT
#endif
	
	serialFlush();

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
	if (isnan(h) || h > 100 || h < 0) {
#if SERIAL && DEBUG
		Serial.println(F("Failed to read DHT11 humidity"));
#endif
	} else {
		payload.rhum = round(h);//smoothedAverage(payload.rhum, round(h), firstTime);
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
		payload.temp = (int)(t*10);//smoothedAverage(payload.temp, (int)(t * 10), firstTime);
#if SERIAL && DEBUG_MEASURE
		Serial.print(F("DHT T new/avg "));
		Serial.print(t);
		Serial.print(' ');
		Serial.println(payload.temp);
#endif
	}
#endif // _DHT

#if _DHT2
	float h2 = dht2.readHumidity();
	float t2 = dht2.readTemperature();
	if (isnan(h2) || h2 > 100 || h2 < 0) {
#if SERIAL && DEBUG
		Serial.println(F("Failed to read DHT11 humidity"));
#endif
	} else {
		payload.rhum2 = round(h2);//smoothedAverage(payload.rhum2, round(h2), firstTime);
#if SERIAL && DEBUG_MEASURE
		Serial.print(F("DHT RH new/avg "));
		Serial.print(h2);
		Serial.print(' ');
		Serial.println(payload.rhum2);
#endif
	}
	if (isnan(t2)) {
#if SERIAL && DEBUG
		Serial.println(F("Failed to read DHT11 temperature"));
#endif
	} else {
		payload.temp2 = (int)(t2*10);//smoothedAverage(payload.temp2, (int)(t2 * 10), firstTime);
#if SERIAL && DEBUG_MEASURE
		Serial.print(F("DHT T new/avg "));
		Serial.print(t2);
		Serial.print(' ');
		Serial.println(payload.temp2);
#endif
	}
#endif // _DHT2

#if _MEM
	payload.memfree = freeRam();
#if SERIAL && DEBUG_MEASURE
	Serial.print("MEM free ");
	Serial.println(payload.memfree);
#endif
#endif

#if SERIAL
	serialFlush();
#endif
}

// periodic report, i.e. send out a packet and optionally report on serial port
void doReport(void)
{
#if SERIAL || DEBUG
	Serial.print(node);
#if _DHT
	Serial.print(' ');
	Serial.print((int) payload.rhum);
	Serial.print(' ');
	Serial.print((int) payload.temp);
#endif
#if _DHT2
	Serial.print(' ');
	Serial.print((int) payload.rhum2);
	Serial.print(' ');
	Serial.print((int) payload.temp2);
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
#endif //_RFM12LOBAT
	serialFlush();
#endif // SERIAL || DEBUG

#if _RFM12B
	rf12_sleep(RF12_WAKEUP);
	rf12_sendNow(0, &payload, sizeof payload);
	rf12_sendWait(RADIO_SYNC_MODE);
	rf12_sleep(RF12_SLEEP);
#endif
}

void runScheduler(char task)
{
	switch (task) {

		case MEASURE:
			// reschedule these measurements periodically
			debugline("MEASURE");
			scheduler.timer(MEASURE, MEASURE_PERIOD);
			doMeasure();

			if (++reportCount >= REPORT_EVERY) {
				reportCount = 0;
				scheduler.timer(REPORT, 0);
			}
			serialFlush();
			break;

		case REPORT:
			debugline("REPORT");
			doReport();
			serialFlush();
			break;

		case WAITING:
			break;

		default:
			Serial.print(task, HEX);
			Serial.println('?');
	}
}


/* *** /Run-time handlers *** }}} */

/* *** InputParser handlers *** {{{ */

static void helpCmd() {
	Serial.println("n: print Node ID");
	Serial.println("c: print config");
	Serial.println("v: print version");
	Serial.println("m: print free and used memory");
	Serial.println("N: set Node (3 byte char)");
	Serial.println("C: set Node ID (1 byte int)");
	Serial.println("W: load/save EEPROM");
	Serial.println("E: erase EEPROM!");
	Serial.println("?/h: this help");
}

static void handshakeCmd() {
	//int v;
	//char buf[7];
	//node.toCharArray(buf, 7);
	//parser >> v;
	//sprintf(node_id, "%s%i", buf, v);
	//Serial.print("handshakeCmd:");
	//Serial.println(node_id);
	//serialFlush();
}

InputParser::Commands cmdTab[] = {
	{ '?', 0, helpCmd },
	{ 'h', 0, helpCmd },
	{ 'A', 0, handshakeCmd },
	{ 0 }
};


/* *** /InputParser handlers *** }}} */

/* *** Main *** {{{ */


void setup(void)
{
#if SERIAL
	mpeser.begin();
	mpeser.startAnnounce(sketch, version);
#if DEBUG || _MEM
	Serial.print(F("Free RAM: "));
	Serial.println(freeRam());
#endif
	serialFlush();
#endif

	initLibs();

	doReset();

//	doMeasure();
//	doReport();
//	serialFlush();
}

void loop(void)
{
#ifdef _DBG_LED
	blink(_DBG_LED, 1, 15);
#endif
	debug_ticks();
/*
	doMeasure();
	if ((tick % 100) == 0) {
		doReport();
	}
	delay(50);
	return;
*/	
		parser.poll();
		debugline("Sleep");
		serialFlush();
		char task = scheduler.pollWaiting();
		debugline("Wakeup");
		if (-1 < task && task < IDLE) {
			runScheduler(task);
		}
}

/* }}} *** */

