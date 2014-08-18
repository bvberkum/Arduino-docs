/* 
Atmega EEPROM routines */
#include <OneWire.h>


/* *** Globals and sketch configuration *** */
#define DEBUG           1 /* Enable trace statements */
#define SERIAL          1 /* Enable serial */
#define MAXLENLINE      79
							

const String sketch = "X-AtmegaEEPROM";
const int version = 0;

char node[] = "nx";
// determined upon handshake 
char node_id[7];


static int tick = 0;
static int pos = 0;


MpeSerial mpeser (57600);

extern InputParser::Commands cmdTab[] PROGMEM;
InputParser parser (50, cmdTab);

/* *** EEPROM config *** {{{ */

#define CONFIG_VERSION "nx1"
#define CONFIG_START 0

struct Config {
	char node[3];
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

void debug(String msg) {
#if DEBUG
	Serial.println(msg);
#endif
}

/* }}} *** */

/* *** Peripheral hardware routines *** {{{ */

/* }}} *** */

/* *** Initialization routines *** {{{ */

void doConfig(void)
{
	/* load valid config or reset default config */
	if (!loadConfig(static_config)) {
		writeConfig(static_config);
	}
}

void setupLibs()
{
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
	doConfig();
	tick = 0;
	reportCount = REPORT_EVERY;     // report right away for easy debugging
	//scheduler.timer(HANDSHAKE, 0);
}

bool doAnnounce()
{
/* see CarrierCase */
#if SERIAL && DEBUG
#endif // SERIAL && DEBUG
}

void doMeasure()
{
}

void runScheduler(char task)
{
	switch (task) {
	}
}


/* }}} *** */

/* *** Main *** {{{ */

void setup(void)
{
	mpeser.begin();
	mpeser.startAnnounce(sketch, version);
	serialFlush();

	setupLibs();

	doReset();
}

void loop(void)
{
	serialFlush();
	char task = scheduler.pollWaiting();
	runScheduler(task);
}

/* }}} *** */

