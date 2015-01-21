/**
* I2C PIR
* can I2C do the announce (PIR trigger)?

See Kicad i2cpir schema/brd


TODO: add/wire up power jack (5v, no vreg) to box
TODO: wire PIR and left data-jack
TODO: test DS bus, serial autodetect?
TODO: wire up other jacks, figure out some link/through/detect scheme (no radio
	so needs to mate with device that does. A RelayBox probably.)
*/



/* *** Globals and sketch configuration *** */
#define SERIAL          1 /* Enable serial */
#define DEBUG           1 /* Enable trace statements */
							
#define _MEM            1   // Report free memory 
#define PIR_PORT        4   // defined if PIR is connected to a port's DIO pin
#define DHT_PIN         7   // defined if DHTxx data is connected to a DIO pin
#define LDR_PORT        4   // defined if LDR is connected to a port's AIO pin
#define WIRE_PIN        9   // I2C slave I/O pin
							
#define REPORT_EVERY    3   // report every N measurement cycles
#define SMOOTH          5   // smoothing factor used for running averages
#define MEASURE_PERIOD  100  // how often to measure, in tenths of seconds
#define TASK_END_PERIOD    60
#define AVR_CTEMP_ADJ   0
#define PIR_HOLD_TIME   15  // hold PIR value this many seconds after change
#define PIR_PULLUP      0   // set to one to pull-up the PIR input pin
#define PIR_INVERTED    0   // 0 or 1, to match PIR reporting high or low
							

#include <util/atomic.h>

#include <EEPROM.h>
#include <Wire.h>
#include <JeeLib.h>
#include <DotmpeLib.h>
#include <mpelib.h>


const String sketch = "Node";
const int version = 0;

char node[] = "wpr";
// determined upon handshake 
char node_id[7];

// this operates in I2C, meaning it waits for either data or request events from
// the master. upon determining the event requested this is primed to a command
// code, and then reset to 
char nextEvent = 0x0;

/* IO pins */
//              MOSI     11
//              MISO     12
//             SCK      13    NRF24
//#       define _DBG_LED 13 // SCK
//#       define          A0
//#       define          A1
//#       define          A2
//#       define          A3
//#       define          A4
//#       define          A5

MpeSerial mpeser (57600);

MilliTimer idle, stdby;

// TODO state payload config in propery type-bytes
//char c[];
//#if LDR_PORT
//c += 'l'
//#endif
//#if DHT_PIN
//c += 'h'
//#endif
//#if MEMREPORT
//c += 'm'
//#endif

/* *** InputParser {{{ */

Stream& cmdIo = Serial;//virtSerial;
extern InputParser::Commands cmdTab[] PROGMEM;
InputParser parser (50, cmdTab);
// See Node for serial


/* }}} *** */

/* *** Report variables *** {{{ */


static byte reportCount;    // count up until next report, i.e. packet send

// This defines the structure of the packets which get sent out by wireless:
struct {
#if LDR_PORT
	byte light  :8;     // light sensor: 0..255
#endif
	byte moved  :1;  // motion detector: 0..1
#if DHT_PIN
	int rhum    :7;   // rhumdity: 0..100 (4 or 5% resolution?)
	int temp    :10; // temperature: -500..+500 (tenths, .5 resolution)
#endif
	int ctemp  :10; // atmega temperature: -500..+500 (tenths)
#if _MEM
	int memfree :16;
#endif
	byte lobat  :1;  // supply voltage dropped under 3.1V: 0..1
} payload;


/* *** /Report variables *** }}} */

/* *** Scheduled tasks *** {{{ */

enum {
	HANDSHAKE,
	MEASURE,
	REPORT,
	TASK_END
};
// Scheduler.pollWaiting returns -1 or -2
static const char WAITING = 0xFF; // -1: waiting to run
static const char IDLE = 0xFE; // -2: no tasks running

static word schedbuf[TASK_END];
Scheduler scheduler (schedbuf, TASK_END);

// has to be defined because we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }


/* *** /Scheduled tasks *** }}} */

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


/* *** /EEPROM config *** }}} */

/* *** Peripheral devices *** {{{ */

#if LDR_PORT
    Port ldr (LDR_PORT);
#endif

/* *** PIR support *** {{{ */
#if PIR_PORT
/// Interface to a Passive Infrared motion sensor.
class PIR : public Port {
	volatile byte value, changed;
	volatile uint32_t lastOn;
public:
	PIR (byte portnum)
		: Port (portnum), value (0), changed (0), lastOn (0) {}

	// this code is called from the pin-change interrupt handler
	void poll() {
		// see http://talk.jeelabs.net/topic/811#post-4734 for PIR_INVERTED
		byte pin = digiRead() ^ PIR_INVERTED;
		// if the pin just went on, then set the changed flag to report it
		if (pin) {
			if (!state()) {
				changed = 1;
			}
			lastOn = millis();
		}
		value = pin;
	}

	// state is true if curr value is still on or if it was on recently
	byte state() const {
		byte f = value;
		if (lastOn > 0)
			ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
				if (millis() - lastOn < 1000 * PIR_HOLD_TIME)
					f = 1;
			}
		return f;
	}

	// return true if there is new motion to report
	byte triggered() {
		byte f = changed;
		changed = 0;
		return f;
	}
};

PIR pir (PIR_PORT);

// the PIR signal comes in via a pin-change interrupt
ISR(PCINT2_vect) { 
	pir.poll(); }
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


/* *** /Peripheral hardware routines }}} *** */

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

static void ser_test_cmd(void) {
	Serial.println("Foo");
}


/* UART commands }}} */

/* Wire init {{{ */

uint8_t *buffer, limit, fill, top, next;
uint8_t instring, hexmode, hasvalue;
uint32_t value;


void wirecmd_reset(void) {
	fill = next = 0;
	instring = hexmode = hasvalue = 0;
	top = limit;
}

void wirecmd_init(int buffersize) {
	buffer = (uint8_t*) malloc(buffersize);
	limit = buffersize;
	wirecmd_reset();
}

/* Wire init }}} */

/* Wire commands {{{ */
    
typedef struct {
	char code;      // one-letter command code
	byte bytes;     // number of bytes required as input
	void (*fun)();  // code to call for this command
} WireCommand;

static void wire_announce_cmd(void) {
	//Wire.write();
}

static void wire_report_cmd(void) {
	//Wire.write();
}

const WireCommand wirecmd_tab[] PROGMEM = {
	{ 0x01, 0, wire_announce_cmd },
	{ 0x02, 0, wire_report_cmd },
	{ 0 }
};

void process_wire_cmd(char req, int fill, char *buffer) {
	Serial.println(sizeof wirecmd_tab);
//	const WireCommand p;
//	for (p = cmds; ; ++p) {
//		char code = pgm_read_byte(&p->code);
//		if (code == 0)
//			break;
//		if (req == code) {
//			uint8_t bytes = pgm_read_byte(&p->bytes);
//			if (fill < bytes) {
//				//Serial.print("Not enough data: ");
//				//showNibble(fill);
//				//Serial.print(", need ");
//				//showNibble(bytes);
//				//Serial.println(" bytes");
//			} else {
//				memset(buffer + fill, 0, top - fill);
//				((void (*)(void)) pgm_read_word(&p->fun))();
//			}
//			wirecmd_reset();
//			return;
//		}
//	}
}

/* Wire commands }}} */;

/* Wire handling {{{ */

// send a message back to master. master will want to know beforehand how many
// bytes to expect. it does not seem to be possible for arguments or incremental
// requests
// XXX maybe useful for initial announce and some specs
void requestEvent()
{

}

// receive data from master. this can be used to 'prime' the node to
// emit certain data on request i think

void receiveEvent(int count)
{
	char c[count-1];
	int cnt = 0;
	while (1 < Wire.available())
	{
		c[cnt++] = Wire.read();
	}
	char code = Wire.read();

	process_wire_cmd(code, count - 1, c);
}

/* Wire Handling }}} */

/* *** Initialization routines *** {{{ */


void initConfig(void)
{
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
	//Wire.begin(WIRE_PIN);
	//Wire.onRequest(requestEvent);
	//Wire.onReceive(receiveEvent);
	//wirecmd_init(10);
}


/* *** /Initialization routines *** }}} */

/* *** Run-time handlers *** {{{ */

void doReset(void)
{
	doConfig();

#if _DBG_LED
	pinMode(_DBG_LED, OUTPUT);
#endif
	ui_irq = false;
	tick = 0;

	//wirecmd_reset();
	
	//pir_init
	//pinMode(PIR_PORT + 3, INPUT);
	pir.digiWrite(PIR_PULLUP);
//#ifdef PCMSK2
	bitSet(PCMSK2, PIR_PORT + 3);
	bitSet(PCICR, PCIE2);
//#endif

	reportCount = REPORT_EVERY;     // report right away for easy debugging
	scheduler.timer(MEASURE, 0);    // get the measurement loop going
}

bool doAnnounce()
{
/* see CarrierCase */
#if SERIAL && DEBUG
#endif // SERIAL && DEBUG
	return false;
}

// readout all the sensors and other values
void doMeasure()
{
	// none, see  SensorNode
}

// periodic report, i.e. send out a packet and optionally report on serial port
void doReport(void)
{
    #if DEBUG
	Serial.print(node);
	Serial.print(" ");
	Serial.print(pir.state());
	Serial.print(" ");
	Serial.print(pir.triggered());
	Serial.println("");
    #endif
}

void doTrigger(void)
{
	payload.moved = pir.state();
    #if DEBUG
        Serial.print("PIR ");
//        Serial.print((int) );
    #endif
	doReport();
}

void runScheduler(char task)
{
	switch (task) {

		case MEASURE:
			debugline("MEASURE");
			// reschedule these measurements periodically
			scheduler.timer(MEASURE, MEASURE_PERIOD);
			doMeasure();

			// every so often, a report needs to be sent out
			if (++reportCount >= REPORT_EVERY) {
				reportCount = 0;
				scheduler.timer(REPORT, 0);
			}
			serialFlush();
			break;

		case REPORT:
			debugline("REPORT");
//			payload.msgtype = REPORT_MSG;
			doReport();
			serialFlush();
			break;

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
	}
}


/* *** /Run-time handlers *** }}} */

/* *** InputParser handlers *** {{{ */


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

// XXX just looking at UART handling here for ref/comparison. See AVRNode
InputParser::Commands cmdTab[] = {
	{ '?', 0, ser_helpCmd },
	{ 'h', 0, ser_helpCmd },
//	{ 'c', 0, configCmd },
	{ 'm', 0, memStat },
	{ 'n', 0, ser_configNodeCmd },
//	{ 'v', 0, configVersionCmd },
//	{ 'N', 3, configSetNodeCmd },
//	{ 'C', 1, configNodeIDCmd },
//	{ 'W', 1, configEEPROM },
//	{ 'E', 0, eraseEEPROM },
	{ 't', 0, ser_test_cmd},
	{ 0 }
};

/* }}} *** */

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
	if (ui_irq) {
		debugline("Irq");
		ui_irq = false;
		uiStart();
		analogWrite(BL, 0xAF ^ BL_INVERTED);
	}
	debug_ticks();
	if (pir.triggered()) {
		doTrigger();
	}
	char task = scheduler.poll();
	if (-1 < task && task < IDLE) {
		runScheduler(task);
	}
	if (ui) {
		parser.poll();
	} else {
#ifdef _DBG_LED
		blink(_DBG_LED, 1, 15);
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

