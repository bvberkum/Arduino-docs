/*

AtmegaEEPROM extends Node

 */



/* *** Globals and sketch configuration *** */
#define SERIAL          1 /* Enable serial */
#define DEBUG           1 /* Enable trace statements */

#define MAXLENLINE      79



#include <JeeLib.h>
#include <OneWire.h>
#include <EEPROM.h>
#include <DotmpeLib.h>
#include <mpelib.h>




const String sketch = "X-AtmegaEEPROM";
const int version = 0;

char node[] = "nx";
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
byte* buffer = (byte*) malloc(50);
InputParser parser (buffer, 50, cmdTab);


/* *** /InputParser }}} *** */

/* *** Report variables *** {{{ */




/* *** /Report variables }}} *** */

/* *** Scheduled tasks *** {{{ */




/* *** /Scheduled tasks *** }}} */

/* *** EEPROM config *** {{{ */


#define CONFIG_VERSION "at1"
#define CONFIG_START 0


struct Config {
	char node[4];
	int node_id;
	int version;
	char config_id[4];
} static_config = {
	/* default values */
	{ node[0], node[1], }, 0, version,
	CONFIG_VERSION
};

Config config;

bool loadConfig(Config &c)
{
	unsigned int w = sizeof(c);

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

/* *** Peripheral devices *** {{{ */

#if LDR_PORT
#endif

#if _DHT
#endif // DHT

#if _LCD84x48
#endif // LCD84x48

#if _DS
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
/* Dallas DS18B20 thermometer routines */


#endif // DS

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
	cmdIo.println("n: print Node ID");
	cmdIo.println("c: print config");
	cmdIo.println("v: print version");
	cmdIo.println("N: set Node (3 byte char)");
	cmdIo.println("C: set Node ID (1 byte int)");
	cmdIo.println("W: load/save config EEPROM");
	cmdIo.println("E: erase EEPROM!");
	cmdIo.println("?/h: this help");
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
	//Serial.println("n " + node_id);
	//Serial.print('n');
	//Serial.print(' ');
	//Serial.print(node_id);
	//Serial.print('\n');
}

static void configVersionCmd(void) {
	cmdIo.print("v ");
	cmdIo.println(static_config.version);
	//Serial.print("v ");
	//showNibble(static_config.version);
	//Serial.println("");
}

// fwd decl.
void initConfig(void);

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
#endif


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
	//doConfig();
	loadConfig(static_config);

	tick = 0;
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


// readout all the sensors and other values
void doMeasure()
{
}

void runScheduler(char task)
{
	switch (task) {
	}
}


/* *** /Run-time handlers *** }}} */

/* *** InputParser handlers *** {{{ */

#if SERIAL

void handshakeCmd() {
	int v;
	//char buf[7];
	//node.toCharArray(buf, 7);
	parser >> v;
	sprintf(node_id, "%s%i", node, v);
	Serial.print("handshakeCmd:");
	Serial.println(node_id);
	serialFlush();
}

InputParser::Commands cmdTab[] = {
	{ '?', 0, helpCmd },
	{ 'h', 0, helpCmd },
	{ 'A', 0, handshakeCmd },
	{ 'c', 0, configCmd },
	{ 'n', 0, configNodeCmd },
	{ 'v', 0, configVersionCmd },
	{ 'N', 3, configSetNodeCmd },
	{ 'C', 1, configNodeIDCmd },
	{ 'W', 1, configEEPROM },
	{ 'E', 0, eraseEEPROM },
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
	if (cmdIo.available()) {
		parser.poll();
		return;
	}

		serialFlush();
		//char task = scheduler.pollWaiting();
		//if (-1 < task && task < SCHED_IDLE) {
		//	runScheduler(task);
		//}

}

/* *** }}} */

