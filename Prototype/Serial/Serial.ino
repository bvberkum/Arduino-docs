/*
	boilerplate for ATmega node with Serial interface
*/



/* *** Globals and sketch configuration *** */
#define SERIAL          1 /* Enable serial */
#define DEBUG           1 /* Enable trace statements */
							
#define UI_IDLE         4000  // tenths of seconds idle time, ...
#define UI_STDBY        8000  // ms
							
#define MAXLENLINE      79
							


#include <DotmpeLib.h>
#include <mpelib.h>




const String sketch = "X-Serial";
const int version = 0;

volatile bool ui_irq;
bool ui;


/* IO pins */
//              RXD      0
//              TXD      1
//              INT0     2
//              MOSI     11
//              MISO     12
#       define _DBG_LED 13 // SCK
//#       define          A0
//#       define          A1
//#       define          A2
//#       define          A3
//#       define          A4
//#       define          A5


MpeSerial mpeser (57600);

MilliTimer idle, stdby;

/* *** InputParser *** {{{ */

Stream& cmdIo = Serial;//virtSerial;
extern InputParser::Commands cmdTab[] PROGMEM;
InputParser parser (50, cmdTab);


/* *** /InputParser }}} *** */

/* *** Report variables *** {{{ */




/* *** /Report variables }}} *** */

/* *** Scheduled tasks *** {{{ */

/* *** /Scheduled tasks }}} *** */

/* *** EEPROM config *** {{{ */

// See Prototype Node or SensorNode
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
}

void writeConfig(Config &c)
{
}


/* *** /EEPROM config }}} *** */

/* *** Peripheral devices *** {{{ */

/* *** /Peripheral devices }}} *** */

/* *** Peripheral hardware routines *** {{{ */

/* *** /Peripheral hardware routines }}} *** */

/* *** UI *** {{{ */
/* *** /UI }}} *** */

/* UART commands {{{ */

static void ser_helpCmd(void) {
	Serial.println("n: print Node ID");
//	Serial.println("c: print config");
//	Serial.println("v: print version");
//	Serial.println("m: print free and used memory");
//	Serial.println("N: set Node (3 byte char)");
//	Serial.println("C: set Node ID (1 byte int)");
//	Serial.println("W: load/save EEPROM");
//	Serial.println("E: erase EEPROM!");
//	Serial.println("?/h: this help");
	idle.set(UI_IDLE);
}

static void ser_configNodeCmd(void) {
	//Serial.println("n " + node_id);
	//Serial.print('n');
	//Serial.print(' ');
	//Serial.print(node_id);
	//Serial.print('\n');
}

static void ser_configVersionCmd(void) {
	//Serial.print("v ");
	//showNibble(static_config.version);
	//Serial.println("");
}

void memStat() {
	int free = freeRam();
	int used = usedRam();
	Stream cmdIo = Serial;
	cmdIo.print("m ");
	cmdIo.print(free);
	cmdIo.print(' ');
	cmdIo.print(used);
	cmdIo.print(' ');
	cmdIo.println();
}

static void ser_test_cmd(void) {
	Serial.println("Foo");
}

void valueCmd () {
	int v;
	parser >> v;
	Serial.print("value = ");
	Serial.println(v);
	idle.set(UI_IDLE);
}

void reportCmd () {
	doReport();
	idle.set(UI_IDLE);
}

void measureCmd() {
	doMeasure();
	idle.set(UI_IDLE);
}

void stdbyCmd() {
	ui = false;
	digitalWrite(BL, LOW ^ BL_INVERTED);
}


/* UART commands }}} */

/* *** Initialization routines *** {{{ */

void initConfig(void)
{
	// See Prototype/Node
	//sprintf(node_id, "%s%i", static_config.node, static_config.node_id);
}

void doConfig(void)
{
	/* load valid config or reset default config */
	//if (!loadConfig(static_config)) {
	//	writeConfig(static_config);
	//}
	initConfig();
}

void initLibs()
{
}


/* *** /Initialization routines }}} *** */

/* *** Run-time handlers *** {{{ */

void doReset(void)
{
	doConfig();

#ifdef _DBG_LED
	pinMode(_DBG_LED, OUTPUT);
#endif
	ui_irq = false;
	tick = 0;
}

bool doAnnounce()
{
	return false;
}


void doReport(void)
{
}

void runScheduler(char task)
{
	switch (task) {

		// FIXME: scheduler is not needed for SerialNode?

#if DEBUG && SERIAL
		case WAITING:
		case IDLE:
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


/* *** /Run-time handlers }}} *** */

/* *** InputParser handlers *** {{{ */

#if SERIAL

InputParser::Commands cmdTab[] = {
	{ '?', 0, ser_helpCmd },
	{ 'h', 0, ser_helpCmd },
	{ 'v', 2, valueCmd },
	{ 'm', 0, measureCmd },
	{ 'r', 0, reportCmd },
	{ 's', 0, stdbyCmd },
	{ 'x', 0, doReset },
	{ 't', 0, ser_test_cmd },
	{ 0 }
};
#endif // SERIAL


/* *** /InputParser handlers }}} *** */

/* *** Main *** {{{ */


void setup(void)
{
#if SERIAL
	mpeser.begin();
	mpeser.startAnnounce(sketch, version);
#if DEBUG || _MEM
	Serial.print("Free RAM: ");
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
	if (ui_irq) {
		debugline("Irq");
		ui_irq = false;
		uiStart();
		//analogWrite(BL, 0xAF ^ BL_INVERTED);
	}
	debug_ticks();

	char task = scheduler.poll();
	if (-1 < task && task < IDLE) {
		runScheduler(task);
	}
	if (ui) {
		parser.poll();
		if (idle.poll()) {
			debugline("Idle");
			stdby.set(UI_STDBY);
		} else if (stdby.poll()) {
			debugline("StdBy");
			ui = false;
		} else if (!stdby.idle()) {
			// XXX toggle UI stdby Power, got to some lower power mode..
			delay(30);
		}
	} else {
#ifdef _DBG_LED
		blink(_DBG_LED, 1, 25);
#endif
		debugline("Sleep");
		serialFlush();
		char task = scheduler.pollWaiting();
		debugline("Wakeup");
		if (-1 < task && task < IDLE) {
			runScheduler(task);
		}
	}
}

/* }}} *** */

