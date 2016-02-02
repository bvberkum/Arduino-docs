/*
 */



/* *** Globals and sketch configuration *** */
#define SERIAL          1 /* Enable serial */
#define DEBUG           1 /* Enable trace statements */
#define DEBUG_MEASURE   1

#define _MEM            1  // Report free memory
#define _AM2321         1  // AOSONG AM2321 temp./rhum
#define _BMP085         1  // Bosch BMP085/BMP180 pressure/temp.
#define _LCD            0
#define _LCD84x48       0
#define _RTC            0
#define _NRF24          0

#define REPORT_EVERY    1   // report every N measurement cycles
#define SMOOTH          5   // smoothing factor used for running averages
#define MEASURE_PERIOD  10  // how often to measure, in tenths of seconds
//#define TEMP_OFFSET     -57
//#define TEMP_K          1.0
#define ANNOUNCE_START  0
#define CONFIG_VERSION "wn1"
#define CONFIG_EEPROM_START 12
#define NRF24_CHANNEL   90





#include <Wire.h>
#include <Adafruit_Sensor.h>
//#include <SoftwareSerial.h>
#include <EEPROM.h>
#include <JeeLib.h>
#include <DotmpeLib.h>
#include <mpelib.h>
#include "printf.h"


const String sketch = "WeatherNode";
const int version = 0;

char node[] = "wn";
// determined upon handshake
char node_id[7];


/* IO pins */
//              RXD      0
//              TXD      1
//              INT0     2
//              MOSI     11
//              MISO     12
//             SCK      13    NRF24
#       define _DBG_LED 13 // SCK


MpeSerial mpeser (57600);

/* *** InputParser *** {{{ */

Stream& cmdIo = Serial;
extern InputParser::Commands cmdTab[];
InputParser parser (50, cmdTab);


/* *** /InputParser }}} *** */

/* *** Report variables *** {{{ */


static byte reportCount;    // count up until next report, i.e. packet send

/* Data reported by this sketch */
struct {
#if LDR_PORT
	byte light      :8;     // light sensor: 0..255
#endif
#if PIR_PORT
	byte moved      :1;  // motion detector: 0..1
#endif

#if _DHT
	int rhum    :7;  // 0..100 (avg'd)
// XXX : dht temp
#if DHT_HIGH
/* DHT22/AM2302: 20% to 100% @ 2% rhum, -40C to 80C @ ~0.5 temp */
	int temp    :10; // -500..+500 (int value of tenths avg)
#else
/* DHT11: 20% to 80% @ 5% rhum, 0C to 50C @ ~2C temp */
	int temp    :10; // -500..+500 (tenths, .5 resolution)
#endif // DHT_HIGH
#endif //_DHT

	int ctemp       :10; // atmega temperature: -500..+500 (tenths)
#if _AM2321
	unsigned int am2321temp        :9;   // Range: -10--41.2, 512 value
	unsigned int am2321rhum        :7;  // Range: 0-100
#endif //_AM2321
#if _BMP085
	unsigned int bmp085pressure    :12;  // Weather Range: 990--1030.96, 4096 values
	unsigned int bmp085temp        :9;   // Weather Range: -10--41.2, 512 values
#endif//_BMP085
#if _MEM
	int memfree     :16;
#endif
#if _RFM12LOBAT
	byte lobat      :1;  // supply voltage dropped under 3.1V: 0..1
#endif
} payload;

// 2**6  64
// 2**9  512
// 2**10 1024
// 2**11 2048
// 2**12 4096
// 2**13 8192

// nRF24L01 max length: 32 bytes? RF24Network header? - iow. 256 bits


/* *** /Report variables *** }}} */

/* *** Scheduled tasks *** {{{ */

enum {
	ANNOUNCE,
	MEASURE,
	REPORT,
	TASK_END
};
// Scheduler.pollWaiting returns -1 or -2
static const char SCHED_WAITING = 0xFF; // -1: waiting to run
static const char SCHED_IDLE = 0xFE; // -2: no tasks running

static word schedbuf[TASK_END];
Scheduler scheduler (schedbuf, TASK_END);

// has to be defined because we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }


/* *** /Scheduled tasks *** }}} */

/* *** EEPROM config *** {{{ */


struct Config {
	char node[3];
	int node_id;
	int version;
	char config_id[4];
	signed int temp_offset;
	float temp_k;
} static_config = {
	/* default values */
	{ node[0], node[1], node[2], }, 0, version,
	CONFIG_VERSION ,
	TEMP_OFFSET, TEMP_K
};

Config config;

bool loadConfig(Config &c)
{
	unsigned int w = sizeof(c);

	if (
			//EEPROM.read(CONFIG_EEPROM_START + w - 1) == c.config_id[3] &&
			//EEPROM.read(CONFIG_EEPROM_START + w - 2) == c.config_id[2] &&
			EEPROM.read(CONFIG_EEPROM_START + w - 3) == c.config_id[1] &&
			EEPROM.read(CONFIG_EEPROM_START + w - 4) == c.config_id[0]
	) {

		for (unsigned int t=0; t<w; t++)
		{
			*((char*)&c + t) = EEPROM.read(CONFIG_EEPROM_START + t);
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

		EEPROM.write(CONFIG_EEPROM_START + t, *((char*)&c + t));

		// verify
		if (EEPROM.read(CONFIG_EEPROM_START + t) != *((char*)&c + t))
		{
			// error writing to EEPROM
#if SERIAL && DEBUG
			Serial.println("Error writing "+ String(t)+" to EEPROM");
#endif
		}
	}
}



/* *** /EEPROM config *** }}} */

/* *** Peripheral devices *** {{{ */

#if SHT11_PORT
#endif

#if LDR_PORT
Port ldr (LDR_PORT);
#endif

/* *** PIR support *** {{{ */
#if PIR_PORT
#endif
/* *** /PIR support }}} *** */

#if _DHT
/* DHTxx: Digital temperature/humidity (Adafruit) */
#if DHT_HIGH
DHT dht (DHT_PIN, DHT21);
//DHT dht (DHT_PIN, DHT22);
// AM2301
// DHT21, AM2302? AM2301?
#else
DHT dht(DHT_PIN, DHT11);
#endif
#endif // DHT
#if _AM2321
#include <Wire.h>
#include <AM2321.h>

AM2321 ac;
#endif

#if _BMP085
#include <Adafruit_BMP085_U.h>

Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);

#endif

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

#endif // DS

#if _RFM12B
/* HopeRF RFM12B 868Mhz digital radio */


#endif // RFM12B

#if _NRF24
/* nRF24L01+: nordic 2.4Ghz digital radio  */


#endif // NRF24

#if _RTC
/* DS1307: Real Time Clock over I2C */
#endif // RTC

#if _HMC5883L
/* Digital magnetometer I2C module */


#endif // HMC5883L


/* *** /Peripheral devices *** }}} */

/* *** Peripheral hardware routines *** {{{ */


#if LDR_PORT
#endif

/* *** PIR support *** {{{ */
#if PIR_PORT
#endif // PIR_PORT
/* *** /PIR support *** }}} */


#if _DHT
/* DHT temp/rh sensor routines (AdafruitDHT) */

#endif // DHT

#if _LCD
#endif //_LCD

#if _LCD84x48


#endif // LCD84x48

#if _DS
/* Dallas DS18B20 thermometer routines */


#endif // DS

#if _RFM12B
/* HopeRF RFM12B 868Mhz digital radio routines */


#endif // RFM12B

#if _NRF24
/* Nordic nRF24L01+ radio routines */


#endif // NRF24 funcs

#if _RTC
#endif // RTC

#if _HMC5883L
/* Digital magnetometer I2C routines */


#endif // HMC5883L


/* *** /Peripheral hardware routines *** }}} */

/* *** UART commands *** {{{ */

#if SERIAL

void ser_helpCmd(void) {
	cmdIo.println("n: print Node ID");
	cmdIo.println("t: internal temperature");
	cmdIo.println("T: set offset");
	cmdIo.println("o: temperature config");
	cmdIo.println("r: report");
	cmdIo.println("M: measure");
	cmdIo.println("E: erase EEPROM!");
	cmdIo.println("x: reset");
	cmdIo.println("?/h: this help");
}

// forward declarations
void doReset(void);
void doReport(void);
void doMeasure(void);

void reset_sercmd() {
	doReset();
}

void report_sercmd() {
	doReport();
}

void measure_sercmd() {
	doMeasure();
}

void ser_configCmd() {
	cmdIo.print("c ");
	cmdIo.print(config.node);
	cmdIo.print(" ");
	cmdIo.print(config.node_id);
	cmdIo.print(" ");
	cmdIo.print(config.version);
	cmdIo.print(" ");
	cmdIo.print(config.config_id);
	cmdIo.println();
}

void ser_tempConfig(void) {
	Serial.print("Offset: ");
	Serial.println(config.temp_offset);
	Serial.print("K: ");
	Serial.println(config.temp_k);
	Serial.print("Raw: ");
	Serial.println(internalTemp());
}

void ser_tempOffset(void) {
	char c;
	parser >> c;
	int v = c;
	if( v > 127 ) {
		v -= 256;
	}
	config.temp_offset = v;
	Serial.print("New offset: ");
	Serial.println(config.temp_offset);
	writeConfig(config);
}

void ser_temp(void) {
	double t = ( internalTemp() + config.temp_offset ) * config.temp_k ;
	Serial.println( t );
}

static void eraseEEPROM() {
	cmdIo.print("! Erasing EEPROM..");
	for (int i = 0; i<EEPROM_SIZE; i++) {
		char b = EEPROM.read(i);
		if (b != 0x00) {
			EEPROM.write(i, 0);
			cmdIo.print('.');
		}
	}
	cmdIo.println(' ');
	cmdIo.print("E ");
	cmdIo.println(EEPROM_SIZE);
}

#endif


/* *** UART commands *** }}} */

/* *** Initialization routines *** {{{ */

void initConfig(void)
{
	// See Prototype/Node
	sprintf(node_id, "%s%i", static_config.node, static_config.node_id);
}

void doConfig(void)
{
	/* load valid config or reset default config */
	if (!loadConfig(config)) {
		writeConfig(static_config);
	}
	initConfig();
}

void initLibs()
{
#if _RTC
	rtc_init();
#endif
#if _DHT
	dht.begin();
#if DEBUG
	Serial.println("Initialized DHT");
	float t = dht.readTemperature();
	Serial.println(t);
	float rh = dht.readHumidity();
	Serial.println(rh);
#endif
#endif

#if _AM2321
//ac.uid()
#endif //_AM2321

#if _BMP085
	if(!bmp.begin())
	{
#if SERIAL
		Serial.print("Missing BMP085");
#endif
	}
#endif

#if _HMC5883L
#endif //_HMC5883L

#if SERIAL && DEBUG
	printf_begin();
#endif
}


/* *** /Initialization routines *** }}} */

/* *** Run-time handlers *** {{{ */

void doReset(void)
{
	doConfig();

#ifdef _DBG_LED
	pinMode(_DBG_LED, OUTPUT);
#endif
	tick = 0;
	reportCount = REPORT_EVERY;     // report right away for easy debugging
	scheduler.timer(ANNOUNCE, ANNOUNCE_START); // get the measurement loop going
}

bool doAnnounce(void)
{
/* see CarrierCase */
#if SERIAL && DEBUG
	cmdIo.print("\n[");
	cmdIo.print(sketch);
	cmdIo.print(".");
	cmdIo.print(version);
	cmdIo.println("]");
#endif // SERIAL && DEBUG
	cmdIo.println(node_id);
#if LDR_PORT
	Serial.print("light:8 ");
#endif
#if PIR
	Serial.print("moved:1 ");
#endif
#if _DHT
	Serial.print(F("rhum:7 "));
	Serial.print(F("temp:10 "));
#endif
	Serial.print(F("ctemp:10 "));
#if _AM2321
	Serial.print(F("am2321rhum:7 "));
	Serial.print(F("am2321temp:9 "));
#endif //_AM2321
#if _BMP085
	Serial.print(F("bmp085pressure:12 "));
	Serial.print(F("bmp085temp:9 "));
#endif //_BMP085
#if _MEM
	Serial.print(F("memfree:16 "));
#endif
#if _RFM12LOBAT
	Serial.print(F("lobat:1 "));
#endif //_RFM12LOBAT
	return false;
}


// readout all the sensors and other values
void doMeasure(void)
{
	byte firstTime = payload.ctemp == 0; // special case to init running avg

	int ctemp = internalTemp();
	payload.ctemp = smoothedAverage(payload.ctemp, ctemp, firstTime);
#if SERIAL && DEBUG_MEASURE
	Serial.print("Payload size: ");
	Serial.println(sizeof(payload));
	Serial.print("AVR T new/avg ");
	Serial.print(ctemp);
	Serial.print(' ');
	Serial.println(payload.ctemp);
#endif

#if _AM2321
	int am2321 = ac.read();
	if (am2321 == 1) {
		payload.am2321temp = 10 + ac.temperature;
		payload.am2321rhum = round(ac.humidity/10);
#if SERIAL && DEBUG_MEASURE
		Serial.print("AM2321 Raw rh/t: ");
		Serial.print(ac.humidity);
		Serial.print(' ');
		Serial.print(ac.temperature);
		Serial.println();
#endif
	} else {
#if SERIAL
		Serial.println("Error reading AM2321");
#endif
	}
#if SERIAL && DEBUG_MEASURE
	Serial.print("AM2321 Measure rh/t: ");
	Serial.print( payload.am2321rhum );
	Serial.print(' ');
	Serial.print( ( (float) payload.am2321temp - 10 ) / 10 ) ;
	Serial.println();
#endif
#endif //_AM2321

#if _BMP085
	sensors_event_t event;
	bmp.getEvent(&event);
	if (event.pressure) {
		payload.bmp085pressure = ( (event.pressure - 990 ) * 100 ) ;
#if SERIAL && DEBUG_MEASURE
		Serial.print("BMP085 Measure pressure-raw/-ranged: ");
		Serial.print( event.pressure );
		Serial.print(' ');
		Serial.print( payload.bmp085pressure );
		Serial.println();
#endif
	} else {
#if SERIAL
		Serial.println("Failed rading BMP085");
#endif
	}
	float bmp085temperature;
	bmp.getTemperature(&bmp085temperature);
	payload.bmp085temp = ( bmp085temperature + 10 ) * 10;
#if SERIAL && DEBUG_MEASURE
		Serial.print("BMP085 Measure temp-raw/-ranged: ");
		Serial.print(bmp085temperature);
		Serial.print(' ');
		Serial.println( payload.bmp085temp );
#endif
#if SERIAL && DEBUG_MEASURE
	Serial.print("BMP085 Pressure/temp: ");
	Serial.print( (float) 990 + ( (float) payload.bmp085pressure / 100 ) );
	Serial.print(' ');
	Serial.print( (float) ( (float) payload.bmp085temp / 10 ) - 10 );
	Serial.println();
#endif
#endif //_BMP085

#if _DHT
	float h = dht.readHumidity();
	float t = dht.readTemperature();
	if (isnan(h)) {
#if SERIAL | DEBUG
		Serial.println(F("Failed to read DHT11 humidity"));
#endif
	} else {
		int rh = h * 10;
		payload.rhum = smoothedAverage(payload.rhum, rh, firstTime);
#if SERIAL && DEBUG_MEASURE
		Serial.print(F("DHT RH new/avg "));
		Serial.print(rh);
		Serial.print(' ');
		Serial.println(payload.rhum);
#endif
	}
	if (isnan(t)) {
#if SERIAL | DEBUG
		Serial.println(F("Failed to read DHT11 temperature"));
#endif
	} else {
		payload.temp = smoothedAverage(payload.temp, t * 10, firstTime);
#if SERIAL && DEBUG_MEASURE
		Serial.print(F("DHT T new/avg "));
		Serial.print(t);
		Serial.print(' ');
		Serial.println(payload.temp);
#endif
	}
#endif // _DHT

#if LDR_PORT
	ldr.digiWrite2(1);  // enable AIO pull-up
	byte light = ~ ldr.anaRead() >> 2;
	ldr.digiWrite2(0);  // disable pull-up to reduce current draw
	payload.light = smoothedAverage(payload.light, light, firstTime);
#if SERIAL && DEBUG_MEASURE
	Serial.print(F("LDR new/avg "));
	Serial.print(light);
	Serial.print(' ');
	Serial.println(payload.light);
#endif
#endif // LDR_PORT

#if _MEM
	payload.memfree = freeRam();
#if SERIAL && DEBUG_MEASURE
	Serial.print("MEM free ");
	Serial.println(payload.memfree);
#endif
#endif //_MEM

#if _RFM12LOBAT
	payload.lobat = rf12_lowbat();
#if SERIAL && DEBUG_MEASURE
	if (payload.lobat) {
		Serial.println("Low battery");
	}
#endif
#endif //_RFM12LOBAT
}


// periodic report, i.e. send out a packet and optionally report on serial port
void doReport(void)
{
#if _RFM12B
#endif //_RFM12B

#if _NRF24
#endif // NRF24

#if SERIAL
	Serial.print(node_id);
	Serial.print(' ');
	Serial.print((int) payload.ctemp);
#if _AM2321
	Serial.print(' ');
	Serial.print((int) payload.am2321temp);
	Serial.print(' ');
	Serial.print((int) payload.am2321rhum);
#endif //_AM2321
#if _BMP085
	Serial.print(' ');
	Serial.print((int) payload.bmp085temp);
	Serial.print(' ');
	Serial.print((int) payload.bmp085pressure);
#endif //_BMP085
#if _MEM
	Serial.print(' ');
	Serial.print((int) payload.memfree);
#endif //_MEM
#if _RFM12LOBAT
	Serial.print(' ');
	Serial.print((int) payload.lobat);
#endif //_RFM12LOBAT

	Serial.println();
#endif //SERIAL
}


void runScheduler(char task)
{
	switch (task) {

		case ANNOUNCE:
			debugline("ANNOUNCE");
			Serial.print(strlen(node_id));
			Serial.println();
			if (strlen(node_id) > 0) {
				Serial.print("Node: ");
				Serial.println(node_id);
				//scheduler.timer(MEASURE, 0);
			} else {
				doAnnounce();
				scheduler.timer(MEASURE, 0); //schedule next step
			}
			serialFlush();
			break;

		case MEASURE:
			// reschedule these measurements periodically
			scheduler.timer(MEASURE, MEASURE_PERIOD);
			doMeasure();

			// every so often, a report needs to be sent out
			if (++reportCount >= REPORT_EVERY) {
				reportCount = 0;
				scheduler.timer(REPORT, 0);
			}
#if SERIAL
			serialFlush();
#endif
#ifdef _DBG_LED
			blink(_DBG_LED, 2, 25);
#endif
			break;

		case REPORT:
			doReport();
			serialFlush();
			break;

#if DEBUG && SERIAL
		case SCHED_WAITING:
		case SCHED_IDLE:
			Serial.print("!");
			serialFlush();
			break;

		default:
			Serial.print("0x");
			Serial.print(task, HEX);
			Serial.println(" ?");
			serialFlush();
			break;
#endif

	}
}


/* *** /Run-time handlers *** }}} */

/* *** InputParser handlers *** {{{ */

#if SERIAL

InputParser::Commands cmdTab[] = {
	{ '?', 0, ser_helpCmd },
	{ 'h', 0, ser_helpCmd },
	{ 'c', 0, ser_configCmd},
	{ 'o', 0, ser_tempConfig },
	{ 'T', 1, ser_tempOffset },
	{ 't', 0, ser_temp },
	{ 'r', 0, reset_sercmd },
	{ 'x', 0, report_sercmd },
	{ 'M', 0, measure_sercmd },
	{ 'E', 0, eraseEEPROM },
	{ 0 }
};

#endif // SERIAL


/* *** /InputParser handlers *** }}} */



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
#ifdef _DBG_LED
	blink(_DBG_LED, 3, 10);
#endif
	debug_ticks();

	if (cmdIo.available()) {
		parser.poll();
		return;
	}

	char task = scheduler.poll();
	if (-1 < task && task < SCHED_IDLE) {
		runScheduler(task);
	}

		serialFlush();
		task = scheduler.pollWaiting();
		if (-1 < task && task < SCHED_IDLE) {
			runScheduler(task);
		}

}


