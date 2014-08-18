/**

Basetype Node
	+ r/w config from EEPROM 

	- sketch_id  XYFullName
	- node_id    xy-1
	- version    0

- Should compile but does not give usable node, iow. util functions / init only.
- Subtypes of this prototype: SerialNode.

See 
- AtmegaEEPROM
- Cassette328P


InputParser invocation: [value[,arg2] ]command

Serial config
	Set node to 'aa'::
		
		"aa" N

	Set node id to 2::

		2,0 C

  */

#include <EEPROM.h>
//#include <util/crc16.h>

#include <JeeLib.h>


/* *** Globals and sketch configuration *** */
#define DEBUG           1 /* Enable trace statements */
#define SERIAL          1 /* Enable serial */
							
#if defined(__AVR_ATtiny84__)
#define SRAM_SIZE       512
#elif defined(__AVR_ATmega168__)
#define SRAM_SIZE       1024
#elif defined(__AVR_ATmega328__) || defined(__AVR_ATmega328P__)
#define SRAM_SIZE       2048
#endif
							

const String sketch = "Node";
const int version = 0;

char node[] = "nx";
// determined upon handshake 
char node_id[7];


/* IO pins */
static const byte ledPin = 13;


/* Command parser */
extern InputParser::Commands cmdTab[] PROGMEM;
byte* buffer = (byte*) malloc(50);
InputParser parser (buffer, 50, cmdTab);

/* *** Device params *** {{{ */


/* }}} */

/* *** Report variables *** {{{ */


/* }}} */

/* *** Scheduled tasks *** {{{ */


/* }}} *** */

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

void initLibs()
{
}

/* }}} *** */

/* InputParser handlers */

static void helpCmd() {
	Serial.println("c: print config");
	Serial.println("C: set Node (2 byte char)");
	Serial.println("N: set Node ID (2 byte int)");
	Serial.println("?/h: this help");
}

static void configCmd() {
	Serial.print("c ");
	Serial.print(static_config.node);
	Serial.print(" ");
	Serial.print(static_config.node_id);
	Serial.print(" ");
	Serial.print(static_config.version);
	Serial.print(" ");
	Serial.print(static_config.config_id);
	Serial.println();
}

static void configNodeCmd() {
	const char *node;
	parser >> node;
	static_config.node[0] = node[0];
	static_config.node[1] = node[1];
	static_config.node[2] = node[2];
	Serial.print("N ");
	Serial.println(static_config.node);
}

static void configNodeIDCmd() {
	parser >> static_config.node_id;
	sprintf(node_id, "%s%i", node, static_config.node_id);
	Serial.print("C ");
	Serial.println(node_id);
	serialFlush();
}

static void configEEPROM() {
	int write;
	parser >> write;
	if (write) {
		writeConfig(static_config);
	} else {
		loadConfig(static_config);
	}
	Serial.print("W ");
	Serial.println(write);
}

InputParser::Commands cmdTab[] = {
	{ '?', 0, helpCmd },
	{ 'h', 0, helpCmd },
	{ 'c', 0, configCmd},
	{ 'N', 1, configNodeCmd },
	{ 'C', 2, configNodeIDCmd },
	{ 'W', 1, configEEPROM },
	{ 0 }
};


/* *** Run-time handlers *** {{{ */

void doReset(void)
{
	doConfig();
}


/* }}} *** */

/* *** Main *** {{{ */

void setup(void)
{
#if SERIAL
	Serial.begin(57600);
	Serial.println(SERIAL);
#endif

	initLibs();

	doReset();
}

void loop(void)
{
    parser.poll();
}

/* }}} *** */

