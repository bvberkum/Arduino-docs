/*

AtmegaTemp extends Node
  - Dumps internal-temperature values to serial,
    to ferify it reads correctly,

 */



/* *** Globals and sketch configuration *** */
#define SERIAL          1 /* Enable serial */
#define DEBUG           0 /* Enable trace statements */
#define DEBUG_MEASURE   0

#define _MEM            1   // Report free memory


// SensorNode: -57
// -68
//#define TEMP_OFFSET     -57
//#define TEMP_K          1.0
#define ANNOUNCE_START  0
#define REPORT_START    25
#define REPORT_PERIOD   20
#define CONFIG_VERSION "nx1"
#define CONFIG_EEPROM_START 0





// Definition of interrupt names
//#include <avr/io.h>
// ISR interrupt service routine
//#include <avr/interrupt.h>

//#include <SoftwareSerial.h>
#include <EEPROM.h>
//#include <util/crc16.h>
#include <JeeLib.h>
//#include <SPI.h>
//#include <RF24.h>
//#include <RF24Network.h>
// Adafruit DHT
//#include <DHT.h>
#include <DotmpeLib.h>
#include <mpelib.h>




const String sketch = "AtmegaTemp";
const int version = 0;

char node[] = "attemp";
// determined upon handshake
char node_id[7];


/* IO pins */
//              RXD      0
//              TXD      1
//              INT0     2
//              MOSI     11
//              MISO     12
//#       define _DBG_LED 13 // SCK



MpeSerial mpeser (57600);



/* *** InputParser *** {{{ */

Stream& cmdIo = Serial;
extern InputParser::Commands cmdTab[];
InputParser parser (50, cmdTab);


/* *** /InputParser }}} *** */

/* *** Report variables *** {{{ */


static byte reportCount;    // count up until next report, i.e. packet send

// This defines the structure of the packets which get sent out by wireless:
struct {
	int ctemp       :10; // atmega temperature: -500..+500 (tenths)
#if _MEM
	int memfree     :16;
#endif
} payload;


/* *** /Report variables }}} *** */

/* *** Scheduled tasks *** {{{ */

enum {
	ANNOUNCE,
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

// See Prototype Node or SensorNode

struct Config {
	char node[4];
	int node_id;
	int version;
	char config_id[4];
	signed int temp_offset;
	float temp_k;
} static_config = {
	/* default values */
	{ node[0], node[1], }, 0, version,
	CONFIG_VERSION, TEMP_OFFSET, TEMP_K
};

Config config;

bool loadConfig(Config &c)
{
	unsigned int w = sizeof(c);

	if (
			EEPROM.read(CONFIG_EEPROM_START + w - 1) == c.config_id[3] &&
			EEPROM.read(CONFIG_EEPROM_START + w - 2) == c.config_id[2] &&
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


/* *** /Peripheral devices *** }}} */

/* *** Peripheral hardware routines *** {{{ */



#if _RFM12B
/* HopeRF RFM12B 868Mhz digital radio */

#endif //_RFM12B

#if _NRF24
/* Nordic nRF24L01+ radio routines */


#endif // NRF24 funcs

#if _RTC
#endif //_RTC

#if _HMC5883L
/* Digital magnetometer I2C routines */


#endif // HMC5883L



/* *** /Peripheral hardware routines *** }}} */

/* *** UI *** {{{ */






/* *** /UI *** }}} */

/* UART commands {{{ */

#if SERIAL
static void helpCmd(void) {
	cmdIo.println("t: internal temperatuer");
	cmdIo.println("T: set offset");
	cmdIo.println("?/h: this help");
}

void ser_neg(void) {
	char v;
  parser >> v;
  // correct sign, not sure but casting doesn't work
  int i = v;
  if( i > 127 ) {
  	  i -= 256;
	}
  Serial.println( i );
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
}

void ser_tempConfig(void) {
	Serial.print("Offset: ");
	Serial.println(config.temp_offset);
	Serial.print("K: ");
	Serial.println(config.temp_k);
	Serial.print("Raw: ");
  Serial.println(internalTemp());
}

void ser_temp(void) {
  double t = ( internalTemp() + config.temp_offset ) * config.temp_k ;
  Serial.println( t );
}

#endif


/* UART commands }}} */

/* *** Initialization routines *** {{{ */

void initConfig(void)
{
	sprintf(node_id, "%s%i", static_config.node, static_config.node_id);
	if (config.temp_k == 0) {
		config.temp_k = 1.0;
	}
}

void doConfig(void)
{
	/* load valid config or reset default config */
	if (!loadConfig(static_config)) {
		writeConfig(static_config);
	}
	initConfig();
}

void initLibs()
{
}


/* *** /Initialization routines *** }}} */

/* *** Run-time handlers *** {{{ */

void doReset(void)
{
	//doConfig();
	loadConfig(static_config);
	initConfig();

#ifdef _DBG_LED
	pinMode(_DBG_LED, OUTPUT);
#endif
	tick = 0;

	scheduler.timer(ANNOUNCE, ANNOUNCE_START);
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

	cmdIo.println(node_id);
#endif // SERIAL && DEBUG
	return false;
}

void runScheduler(char task)
{
	switch (task) {

		case ANNOUNCE:
				doAnnounce();
				scheduler.timer(REPORT, REPORT_START); //schedule next step
			serialFlush();
			break;

		case REPORT:
			Serial.println( ( internalTemp() + config.temp_offset ) * config.temp_k);
			serialFlush();
			scheduler.timer(REPORT, REPORT_PERIOD);
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
	{ '?', 0, helpCmd },
	{ 'h', 0, helpCmd },
	{ 'o', 0, ser_tempConfig },
	{ 'X', 1, ser_neg },
	{ 'T', 1, ser_tempOffset },
	{ 't', 0, ser_temp },
	{ 'x', 0, doReset },
	{ 0 }
};

#endif // SERIAL


/* *** /InputParser handlers *** }}} */

/* *** Main *** {{{ */


void setup(void)
{
#if SERIAL
	mpeser.begin();
	mpeser.startAnnounce(sketch, String(version));
	//virtSerial.begin(4800);
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
	blink(_DBG_LED, 1, 25);
#endif

	if (cmdIo.available()) {
		parser.poll();
	}

	char task = scheduler.poll();
	if (-1 < task && task < SCHED_IDLE) {
		runScheduler(task);
	}

	return; // no sleep
		serialFlush();
		task = scheduler.pollWaiting();
		if (-1 < task && task < SCHED_IDLE) {
			runScheduler(task);
		}
}

/* *** }}} */

