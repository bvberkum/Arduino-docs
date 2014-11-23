/**

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

#include <SoftwareSerial.h>
#include <DotmpeLib.h>
#include <EEPROM.h>
//#include <util/crc16.h>

#include <JeeLib.h>


/* *** Globals and sketch configuration *** */
#define DEBUG           1 /* Enable trace statements */
#define SERIAL          1 /* Enable serial */
							
#define _MEM            1   // Report free memory 
							
#if defined(__AVR_ATtiny84__) || defined(__AVR_ATtiny85__)
#define SRAM_SIZE       512
#define EEPROM_SIZE     512
#elif defined(__AVR_ATmega168__)
#define SRAM_SIZE       1024
#define EEPROM_SIZE     512
#elif defined(__AVR_ATmega328__) || defined(__AVR_ATmega328P__)
#define SRAM_SIZE       2048
#define EEPROM_SIZE     1024
#endif
							

const String sketch = "Node";
const int version = 0;

char node[] = "nx";
// determined upon handshake 
char node_id[7];


/* IO pins */
static const byte ledPin = 13;

//SoftwareSerial virtSerial(11, 12); // RX, TX


/* Command parser */
Stream& cmdIo = Serial;//virtSerial;
extern InputParser::Commands cmdTab[] PROGMEM;
byte* buffer = (byte*) malloc(50);
InputParser parser (buffer, 50, cmdTab);//, virtSerial);

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
	char node[4];
	int node_id;
	int version;
	char config_id[4];
} static_config = {
	/* default values */
	{ node[0], node[1], 0, 0 }, 0, version, CONFIG_VERSION
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

/* }}} *** */

/* *** Run-time handlers *** {{{ */

void doReset(void)
{
	doConfig();
}

bool doAnnounce()
{
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

void runScheduler(char task)
{
	// no-op, see SensorNode
}

/* }}} *** */


/* InputParser handlers */

static void helpCmd() {
	cmdIo.println("n: print Node ID");
	cmdIo.println("c: print config");
	cmdIo.println("v: print version");
	cmdIo.println("m: print free and used memory");
	cmdIo.println("N: set Node (3 byte char)");
	cmdIo.println("C: set Node ID (1 byte int)");
	cmdIo.println("W: load/save EEPROM");
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

static void configNodeCmd() {
	cmdIo.print("n ");
	cmdIo.println(node_id);
}

static void configVersionCmd() {
	cmdIo.print("v ");
	cmdIo.println(static_config.version);
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

static void memStat() {
	int free = freeRam();
	int used = usedRam();
	cmdIo.print("m ");
	cmdIo.print(free);
	cmdIo.print(' ');
	cmdIo.print(used);
	cmdIo.print(' ');
	cmdIo.println();
}

InputParser::Commands cmdTab[] = {
	{ '?', 0, helpCmd },
	{ 'h', 0, helpCmd },
	{ 'c', 0, configCmd},
	{ 'm', 0, memStat},
	{ 'n', 0, configNodeCmd },
	{ 'v', 0, configVersionCmd },
	{ 'N', 3, configSetNodeCmd },
	{ 'C', 1, configNodeIDCmd },
	{ 'W', 1, configEEPROM },
	{ 'E', 0, eraseEEPROM },
	{ 0 }
};

/* }}} *** */

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

	cmdIo.print("\n[");
	cmdIo.print(sketch);
	cmdIo.print(".");
	cmdIo.print(version);
	cmdIo.println("]");

	cmdIo.println(node_id);
}

void loop(void)
{
	// FIXME: seems virtSerial receive is not working
	//if (virtSerial.available())
	//	virtSerial.write(virtSerial.read());
	parser.poll();
}

/* }}} *** */

