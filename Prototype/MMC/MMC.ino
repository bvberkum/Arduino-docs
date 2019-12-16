/*
   Atmega MMC/SD card routines

   Need library. See Misc/MMC working but largish?

 */
#include <DotmpeLib.h>
#include <mpelib.h>


/** Globals and sketch configuration  */
#define DEBUG           1 /* Enable trace statements */
#define SERIAL          1 /* Enable serial */

#define MAXLENLINE      79


static String sketch = "X-MMC";
static String version = "0";

static int tick = 0;
static int pos = 0;

/* IO pins */
static const byte ledPin = 13;

MpeSerial mpeser (57600);



/* *** Initialization routines *** {{{ */

void doConfig(void)
{
}

void initLibs()
{
}


/* Run-time handlers */

void doReset(void)
{
	tick = 0;
}

bool doAnnounce()
{
}

void doMeasure()
{
}

// periodic report, i.e. send out a packet and optionally report on serial port
void doReport(void)
{
}

void runCommand()
{
}

void runScheduler(char task)
{
	switch (task) {
	}
}

/* Main */

void setup(void)
{
#if SERIAL
	mpeser.begin();
	mpeser.startAnnounce(sketch, version);
	serialFlush();
#endif

	initLibs();

	doReset();
}

void loop(void)
{
	//blink(ledPin, 1, 15);
	//debug_ticks();
	//serialFlush();
}
