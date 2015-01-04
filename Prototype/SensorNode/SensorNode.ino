/*
	Base sketch SensorNode.

	boilerplate for ATmega node with Serial interface
	uses JeeLib scheduler like JeeLab's roomNode.
	No radio.

	TODO: test sketch, serial. 
		Simple protocol for control, data retrieval sessions?
*/


/* *** Globals and sketch configuration *** */
#define DEBUG           1 /* Enable trace statements */
#define SERIAL          1 /* Enable serial */
							
#define _MEM            1   // Report free memory 
#define _DHT            0
#define LDR_PORT        4   // defined if LDR is connected to a port's AIO pin
#define DHT_HIGH        1   // enable for DHT22/AM2302, low for DHT11
#define _DS             0
#define _LCD84x48       0
#define _NRF24          0
							
#define MEASURE_PERIOD  50  // how often to measure, in tenths of seconds
#define REPORT_EVERY    5   // report every N measurement cycles
#define SMOOTH          5   // smoothing factor used for running averages
#define STDBY_PERIOD    60
							

#include <EEPROM.h>
#include <JeeLib.h>
#include <DotmpeLib.h>
#include <mpelib.h>



const String sketch = "X-SensorNode";
const int version = 0;

char node[] = "nx";
// determined upon handshake 
char node_id[7];


/* IO pins */
static const byte ledPin = 13;
#if _DHT
static const byte DHT_PIN = 7;
#endif
#if _DS
static const byte DS_PIN = 8;
#endif

MpeSerial mpeser (57600);


/* InputParser {{{ */
/* }}} *** */

/* *** Report variables *** {{{ */


static byte reportCount;    // count up until next report, i.e. packet send

// This defines the structure of the packets which get sent out by wireless:
struct {
} payload;


/* *** /Report variables *** }}} */

/* *** Scheduled tasks *** {{{ */

enum {
	HANDSHAKE,
	MEASURE,
	REPORT,
	STDBY,
	END
};
// Scheduler.pollWaiting returns -1 or -2
static const char WAITING = 0xFF; // -1: waiting to run
static const char IDLE = 0xFE; // -2: no tasks running

static word schedbuf[END];
Scheduler scheduler (schedbuf, END);

// has to be defined because we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }



/* *** /Scheduled tasks *** }}} */

/* *** Peripheral devices *** {{{ */

#if LDR_PORT
Port ldr (LDR_PORT);
#endif

#if _DHT
#endif //_DHT

#if _LCD84x48
/* Nokkia 5110 display */
#endif // LCD84x48

#if _DS
/* Dallas OneWire bus with registration for DS18B20 temperature sensors */

OneWire ds(DS_PIN);

uint8_t ds_count = 0;
uint8_t ds_search = 0;

//uint8_t ds_addr[ds_count][8] = {
//	{ 0x28, 0xCC, 0x9A, 0xF4, 0x03, 0x00, 0x00, 0x6D }, // In Atmega8TempGaurd
//	{ 0x28, 0x8A, 0x5B, 0xDD, 0x03, 0x00, 0x00, 0xA6 },
//	{ 0x28, 0x45, 0x94, 0xF4, 0x03, 0x00, 0x00, 0xB3 },
//	{ 0x28, 0x08, 0x76, 0xF4, 0x03, 0x00, 0x00, 0xD5 },
//	{ 0x28, 0x82, 0x27, 0xDD, 0x03, 0x00, 0x00, 0x4B },
//};
enum { DS_OK, DS_ERR_CRC };


#endif // _DS

#if _NRF24
/* nRF24L01+: nordic 2.4Ghz digital radio  */

#endif //_NRF24

#if _HMC5883L
/* Digital magnetometer I2C module */

#endif //_HMC5883L

#if _NRF24
/* nRF24L01+: nordic 2.4Ghz digital radio  */


#endif //_NRF24

#if _LCD
#endif //_LCD

#if _RTC
#endif //_RTC

#if _HMC5883L
/* Digital magnetometer I2C module */

#endif //_HMC5883L


/* *** /Peripheral devices *** }}} */

/* *** EEPROM config *** {{{ */

#define CONFIG_VERSION "sn1"
#define CONFIG_START 0


struct Config {
	char node[3];
	int node_id; 
	int version;
	char config_id[4];
} static_config = {
	/* default values */
	{ node[0], node[1], 0, }, 0, version, 
	{ CONFIG_VERSION[0], CONFIG_VERSION[1], CONFIG_VERSION[2], }
};

Config config;

bool loadConfig(Config &c) 
{
	int w = sizeof(c);

	if (
			EEPROM.read(CONFIG_START + w - 1) == c.config_id[3] &&
			EEPROM.read(CONFIG_START + w - 2) == c.config_id[2] &&
			EEPROM.read(CONFIG_START + w - 3) == c.config_id[1] &&
			EEPROM.read(CONFIG_START + w - 4) == c.config_id[0]
	) {

		for (unsigned int t=0; t<w; t++)
		{
			*((char*)&c + t) = EEPROM.read(CONFIG_START + t);
		}
		return true;

	} else {
#if SERIAL && DEBUG
		Serial.println("No valid data in eeprom");
#endif
		return false;
	}
}

void writeConfig(Config &c)
{
	for (unsigned int t=0; t<sizeof(c); t++) {
		
		EEPROM.write(CONFIG_START + t, *((char*)&c + t));

		// verify
		if (EEPROM.read(CONFIG_START + t) != *((char*)&c + t))
		{
			// error writing to EEPROM
#if SERIAL && DEBUG
			Serial.println("Error writing "+ String(t)+" to EEPROM");
#endif
		}
	}
}


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

#if _LCD84x48
#endif //_LCD84x48

#if _DS
/* Dallas DS18B20 thermometer routines */


#endif // DS

#if _NRF24
/* Nordic nRF24L01+ routines */

#endif // RF24 funcs

#if _LCD
#endif //_LCD

#if _RTC
#endif //_RTC

#if _HMC5883L
/* Digital magnetometer I2C module */
#endif //_HMC5883L


/* *** /Peripheral hardware routines }}} *** */

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
#if _DHT
	dht.begin();
#if DEBUG 
	Serial.println("Initialized DHT");
	float t = dht.readTemperature();
	Serial.println(t);
#endif
#endif

#if _HMC5883L
#endif //_HMC5883L
}


/* *** /Initialization routines *** }}} */

/* *** Run-time handlers *** {{{ */

void doReset(void)
{
	tick = 0;

	doConfig();

	reportCount = REPORT_EVERY;     // report right away for easy debugging
	//scheduler.timer(HANDSHAKE, 0);
	scheduler.timer(MEASURE, 0);    // start the measurement loop going
}

bool doAnnounce()
{
	return false;
}

// readout all the sensors and other values
void doMeasure()
{
	// TODO doMeasure
}

// periodic report, i.e. send out a packet and optionally report on serial port
void doReport(void)
{
	// none, see RadioNode
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
		case IDLE:
			scheduler.timer(STDBY, STDBY_PERIOD);
			break;
		
	}
}


/* *** /Run-time handlers *** }}} */

/* *** InputParser handlers **** {{{ */



/* *** /InputParser handlers **** }}} */

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
}

void loop(void)
{
	blink(ledPin, 1, 15);
	debug_ticks();
	serialFlush();
	char task = scheduler.pollWaiting();
	runScheduler(task);
}

/* }}} *** */

