/* 
Atmega EEPROM routines */
#include <OneWire.h>


/** Globals and sketch configuration  */
#define DEBUG           1 /* Enable trace statements */
#define SERIAL          1 /* Enable serial */
#define MAXLENLINE      79
							

static String sketch = "X-AtmegaEEPROM";
static String node = "x_atmeeprom";
static String vesion = "0";

static int tick = 0;
static int pos = 0;


MpeSerial mpeser (57600);

extern InputParser::Commands cmdTab[] PROGMEM;
InputParser parser (50, cmdTab);

/* Atmega EEPROM stuff */
const long maxAllowedWrites = 100000; /* if evenly distributed, the ATmega328 EEPROM 
should have at least 100,000 writes */
const int memBase          = 0;
//const int memCeiling       = EEPROMSizeATmega328;

/** Generic routines */

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

/* Initialization routines */

void setupLibs()
{
}

void reset(void)
{
	tick = 0;
	reportCount = REPORT_EVERY;     // report right away for easy debugging
	//scheduler.timer(HANDSHAKE, 0);
}

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

/* Run-time handlers */

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


/* Main */

void setup(void)
{
	mpeser.begin();
	mpeser.startAnnounce(sketch, version);
	serialFlush();

	setupLibs();

	reset();
}

void loop(void)
{
	serialFlush();
	char task = scheduler.pollWaiting();
	runScheduler(task);
}
