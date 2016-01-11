/*

Basetype Node
	+ r/w config from EEPROM

	- node_id    xy-1
	- sketch_id  XY
	- version    0

- Subtypes of this prototype: Serial.

- Depends on JeeLib Ports InputParser, but not all of ports compiles on t85.

XXX: SoftwareSerial support 20, 16, 8Mhz only. Does not compile for m8.

See
- AtmegaEEPROM
- Cassette328P

Serial protocol: [value[,arg2] ]command

Serial config
	Set node to 'aa'::

		"aa" N

	Set node id to 2::

		2,0 C

	Write to EEPROM::

		1 W

	Reload from EEPROM (reset changes)::

		0 W

 */


/* *** Globals and sketch configuration *** */
#define SERIAL          1 /* Enable serial */
#define DEBUG           1 /* Enable trace statements */
#define DEBUG_MEASURE   0

#define _MEM            1   // Report free memory
#define _RFM12B         0
#define _RFM12LOBAT     0   // Use JeeNode lowbat measurement

#define UI_IDLE         4000  // tenths of seconds idle time, ...
#define UI_STDBY        8000  // ms



// Definition of interrupt names
//#include <avr/io.h>
// ISR interrupt service routine
//#include <avr/interrupt.h>

#include <SoftwareSerial.h>
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




const String sketch = "Node";
const int version = 0;

char node[] = "nx";
// determined upon handshake
char node_id[7];

volatile bool ui_irq;
bool ui;


/* IO pins */
//              RXD      0
//              TXD      1
//              INT0     2
//              MOSI     11
//              MISO     12
//#       define _DBG_LED 13 // SCK


//SoftwareSerial virtSerial(11, 12); // RX, TX

MpeSerial mpeser (57600);

MilliTimer idle, stdby;

/* *** InputParser *** {{{ */

Stream& cmdIo = Serial;
extern InputParser::Commands cmdTab[];
byte* buffer = (byte*) malloc(50);
InputParser parser (buffer, 50, cmdTab);//, virtSerial);


/* *** /InputParser }}} *** */

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

	int ctemp       :10; // atmega temperature: -500..+500 (tenths)
#if _MEM
	int memfree     :16;
#endif
#if _RFM12LOBAT
	byte lobat      :1;  // supply voltage dropped under 3.1V: 0..1
#endif
} payload;


/* *** /Report variables }}} *** */

/* *** Scheduled tasks *** {{{ */

enum {
	ANNOUNCE,
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
#define CONFIG_VERSION "nx1"
#define CONFIG_START 0


struct Config {
	char node[3];
	int node_id;
	int version;
	char config_id[4];
} static_config = {
	/* default values */
	{ node[0], node[1], 0 }, 0, version, CONFIG_VERSION
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
		cmdIo.println("No valid data in eeprom");
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
			cmdIo.println("Error writing "+ String(t)+" to EEPROM");
#endif
		}
	}
}



/* *** /EEPROM config *** }}} */

/* *** Peripheral devices *** {{{ */

#if SHT11_PORT
SHT11 sht11 (SHT11_PORT);
#endif


#if LDR_PORT
Port ldr (LDR_PORT);
#endif

#if _DHT
/* DHT temp/rh sensor
 - AdafruitDHT
*/
#endif // DHT

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


/* *** /Peripheral devices *** }}} */

/* *** Peripheral hardware routines *** {{{ */


#if LDR_PORT
#endif

/* *** PIR support *** {{{ */
#if PIR_PORT
#endif // PIR_PORT
/* *** /PIR support *** }}} */

#if _DHT
/* DHT temp/rh sensor
 - AdafruitDHT
*/

#endif // DHT

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

/* *** UI *** {{{ */


//ISR(INT0_vect)
void irq0()
{
	ui_irq = true;
	//Sleepy::watchdogInterrupts(0);
}

/* *** /UI }}} *** */

/* UART commands {{{ */

#if SERIAL
static void helpCmd(void) {
	cmdIo.println("n: print Node ID");
	cmdIo.println("c: print config");
	cmdIo.println("v: print version");
	cmdIo.println("m: print free and used memory");
	cmdIo.println("N: set Node (3 byte char)");
	cmdIo.println("C: set Node ID (1 byte int)");
	cmdIo.println("W: load/save config EEPROM");
	cmdIo.println("E: erase EEPROM!");
	cmdIo.println("?/h: this help");
}

void memStat() {
	int free = freeRam();
	int used = usedRam();

	cmdIo.print("m ");
	cmdIo.print(free);
	cmdIo.print(' ');
	cmdIo.print(used);
	cmdIo.print(' ');
	cmdIo.println();
}

static void configCmd() {
	cmdIo.print("c ");
	cmdIo.print(static_config.node);
	cmdIo.print(" ");
	cmdIo.print(static_config.node_id);
	cmdIo.print(" ");
	cmdIo.print(static_config.version);
	cmdIo.print(" ");
	cmdIo.print(static_config.config_id);
	cmdIo.println();
}

static void configNodeCmd(void) {
	Serial.println("n " + node_id);
}

static void configVersionCmd(void) {
	cmdIo.print("v ");
	cmdIo.println(static_config.version);
}

static void configSetNodeCmd() {
	const char *node;
	parser >> node;
	static_config.node[0] = node[0];
	static_config.node[1] = node[1];
	static_config.node[2] = node[2];
	initConfig();
	cmdIo.print("N ");
	cmdIo.println(static_config.node);
}

static void configNodeIDCmd() {
	parser >> static_config.node_id;
	initConfig();
	cmdIo.print("C ");
	cmdIo.println(node_id);
	serialFlush();
}

static void configEEPROM() {
	int write;
	parser >> write;
	if (write) {
		writeConfig(static_config);
	} else {
		loadConfig(static_config);
		initConfig();
	}
	cmdIo.print("W ");
	cmdIo.println(write);
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



/* UART commands }}} */

/* *** Initialization routines *** {{{ */

void initConfig(void)
{
	sprintf(node_id, "%s%i", static_config.node, static_config.node_id);
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
	doConfig();

#ifdef _DBG_LED
	pinMode(_DBG_LED, OUTPUT);
#endif
	attachInterrupt(INT0, irq0, RISING);
	ui_irq = false;
	tick = 0;

	scheduler.timer(ANNOUNCE, 0);
}

bool doAnnounce(void)
{

	cmdIo.print("\n[");
	cmdIo.print(sketch);
	cmdIo.print(".");
	cmdIo.print(version);
	cmdIo.println("]");

	cmdIo.println(node_id);
	return false;
}


// readout all the sensors and other values
void doMeasure()
{
	// none, see  SensorNode
}

// periodic report, i.e. send out a packet and optionally report on serial port
void doReport(void)
{
	// none, see RadioNode
}

void uiStart()
{
	idle.set(UI_IDLE);
	if (!ui) {
		ui = true;
	}
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
				//scheduler.timer(MEASURE, 0); schedule next step
			} else {
				doAnnounce();
				//scheduler.timer(ANNOUNCE, 100);
			}
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

InputParser::Commands cmdTab[] = {
	{ '?', 0, helpCmd },
	{ 'h', 0, helpCmd },
	{ 'c', 0, configCmd },
	{ 'm', 0, memStat },
	{ 'n', 0, configNodeCmd },
	{ 'v', 0, configVersionCmd },
	{ 'N', 3, configSetNodeCmd },
	{ 'C', 1, configNodeIDCmd },
	{ 'W', 1, configEEPROM },
	{ 'E', 0, eraseEEPROM },
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
	blink(_DBG_LED, 3, 10);
#endif
	// FIXME: seems virtSerial receive is not working
	//if (virtSerial.available())
	//	virtSerial.write(virtSerial.read());
	if (ui_irq) {
		debugline("Irq");
		ui_irq = false;
		uiStart();
	}
	debug_ticks();

	if (cmdIo.available()) {
		parser.poll();
		return;
	}

	char task = scheduler.poll();
	if (-1 < task && task < SCHED_IDLE) {
		runScheduler(task);
	}

	if (ui) {
		if (idle.poll()) {
			stdby.set(UI_STDBY);
		} else if (stdby.poll()) {
			ui = false;
		} else if (!stdby.idle()) {
			// XXX toggle UI stdby Power, got to some lower power mode..
			delay(30);
		}
	} else {
#ifdef _DBG_LED
		blink(_DBG_LED, 1, 25);
#endif
		serialFlush();
		char task = scheduler.pollWaiting();
		if (-1 < task && task < SCHED_IDLE) {
			runScheduler(task);
		}
	}
}

/* *** }}} */

