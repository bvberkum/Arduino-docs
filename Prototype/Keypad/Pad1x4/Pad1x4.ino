/**

  Keypad1x4

Pinout:

	1. COMMON (or ROW1)
	2. key B (or COL2)
	3. key A (or COL1)
	4. key D (or COL4)
	5. key C (or COL3)

**/



/* *** Globals and sketch configuration *** */
#define SERIAL          1 /* Enable serial */
#define DEBUG           1 /* Enable trace statements */
							
#define UI_SCHED_IDLE         4000  // tenths of seconds idle time, ...
#define UI_STDBY        8000  // ms
							
#define MAXLENLINE      79
							


#include <JeeLib.h>
#include <DotmpeLib.h>
#include <mpelib.h>
#include <Keypad.h>




const String sketch = "Keypad1x4";
const int version = 0;

char node[] = "p4k";

volatile bool ui_irq;
bool ui;

const byte ROWS = 1;
const byte COLS = 4; 
//define the cymbols on the buttons of the keypads
char pad1x3[ROWS][COLS] = {
  {'A','B','C','D'},
};

/* IO pins */
//#       define ROW4_PIN  0 // RXD
//#       define ROW3_PIN  1 // TXD
//#       define ROW2_PIN  2 // INT0
#       define ROW1_PIN  3
#       define COL4_PIN  4
#       define COL3_PIN  5
#       define COL2_PIN  6
#       define COL1_PIN  7
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

/* *** /InputParser }}} *** */

/* *** Scheduled tasks *** {{{ */

// XXX: no scheduled events
enum {
	TASK_END
};
// Scheduler.pollWaiting returns -1 or -2
static const char SCHED_WAITING = 0xFF; // -1: waiting to run
static const char SCHED_IDLE = 0xFE; // -2: no tasks running

static word schedbuf[TASK_END];
Scheduler scheduler (schedbuf, TASK_END);

// has to be defined because we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }
/* *** /Scheduled tasks }}} *** */

/* *** EEPROM config *** {{{ */



/* *** /EEPROM config }}} *** */

/* *** Peripheral devices *** {{{ */


byte rowPins[ROWS] = { ROW1_PIN };
byte colPins[COLS] = { COL1_PIN, COL2_PIN, COL3_PIN, COL4_PIN };

//initialize an instance of class NewKeypad
Keypad customKeypad = Keypad( makeKeymap(pad1x3), rowPins, colPins, ROWS, COLS); 

/* *** /Peripheral devices }}} *** */

/* *** Peripheral hardware routines *** {{{ */

/* *** /Peripheral hardware routines }}} *** */

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

void doMeasure()
{
}

void doReport(void)
{
}

void uiStart()
{
	idle.set(UI_SCHED_IDLE);
	if (!ui) {
		ui = true;
	}
}

void runScheduler(char task)
{
	switch (task) {

		// FIXME: scheduler is not needed for SerialNode?

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


/* *** /Run-time handlers }}} *** */

/* *** Main *** {{{ */


void setup(void)
{
#if SERIAL
	mpeser.begin();
	mpeser.startAnnounce(sketch, String(version));
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
	char customKey = customKeypad.getKey();
	if (customKey){
		Serial.println(customKey);
	}
	return; /*
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
	if (-1 < task && task < SCHED_IDLE) {
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
		if (-1 < task && task < SCHED_IDLE) {
			runScheduler(task);
		}
	} */
}

/* }}} *** */

