/*
	boilerplate for ATmega node with Serial interface
*/


/* *** Globals and sketch configuration *** */
#define SERIAL          1 /* Enable serial */
#define DEBUG           1 /* Enable trace statements */
							
#define MAXLENLINE      79
							


#include <DotmpeLib.h>
#include <mpelib.h>


const String sketch = "X-Serial";
const int version = 0;

/* IO pins */
//             RXD      0
//             TXD      1
//             INT0     2
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

/* *** InputParser {{{ */
/* }}} *** */

/* *** Report variables *** {{{ */




/* *** /Report variables }}} *** */

/* *** Scheduled tasks *** {{{ */

/* *** /Scheduled tasks }}} *** */

/* *** EEPROM config *** {{{ */

/* *** /EEPROM config }}} *** */

/* *** Peripheral devices *** {{{ */

/* *** /Peripheral devices }}} *** */

/* *** UI *** {{{ */


/* *** /UI }}} *** */

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
	return false;
}


/* *** /Run-time handlers *** }}} */

/* *** InputParser handlers *** {{{ */



/* *** /InputParser handlers *** }}} */

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
	//blink(_DBG_LED, 1, 15);
	debug_ticks();
	//serialFlush();
}

/* }}} *** */

