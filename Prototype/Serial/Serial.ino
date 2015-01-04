/*
	boilerplate for ATmega node with Serial interface
*/


/* *** Globals and sketch configuration *** */
#define DEBUG           1 /* Enable trace statements */
#define SERIAL          1 /* Enable serial */
							
#define MAXLENLINE      79
							


#include <DotmpeLib.h>
#include <mpelib.h>

const String sketch = "X-Serial";
const int version = 0;

/* IO pins */
static const byte ledPin = 13;

MpeSerial mpeser (57600);

/* *** InputParser {{{ */
/* }}} *** */

/* *** Report variables *** {{{ */




/* *** /Report variables *** }}} */

/* *** Scheduled tasks *** {{{ */

/* *** /Scheduled tasks *** }}} */

/* *** Peripheral devices *** {{{ */

/* *** /Peripheral devices *** }}} */

/* *** EEPROM config *** {{{ */

/* *** /EEPROM config *** }}} */

/* *** Peripheral hardware routines *** {{{ */

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
}


/* *** /Initialization routines *** }}} */

/* *** Run-time handlers *** {{{ */

void doReset(void)
{
	tick = 0;

	doConfig();
}

bool doAnnounce()
{
}

/* }}} *** */

/* InputParser handlers */


/* *** /InputParser handlers *** }}} */

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
	//blink(ledPin, 1, 15);
	debug_ticks();
	//serialFlush();
}

/* }}} *** */

