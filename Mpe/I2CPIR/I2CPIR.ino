/**
* I2C PIR
* can I2C do the announce (PIR trigger)?

- Includes UI loop, not for PIR but for 
  wire protocols. 
  
See Kicad i2cpir schema/brd

TODO: See how wire protos can generate irq.
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
#define UI_IDLE         4000  // tenths of seconds idle time, ...
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

volatile bool ui_irq;
bool ui;

/* IO pins */
//              RXD      0
//              TXD      1
//              INT0     2     UI IRQ
//              MOSI     11
//              MISO     12
//              SCK      13
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

/* *** InputParser *** {{{ */

Stream& cmdIo = Serial;//virtSerial;
extern InputParser::Commands cmdTab[] PROGMEM;
InputParser parser (50, cmdTab);
// See Prototype/Serial sketch for serial UI


/* *** /InputParser }}} *** */

/* *** Report variables *** {{{ */


static byte reportCount;    // count up until next report, i.e. packet send

// This defines the structure of the packets which get sent out by wireless:
struct {
#if LDR_PORT
	byte light      :8;     // light sensor: 0..255
#endif
#if PIR_PORT
	byte moved      :1;  // motion detector: 0..1
#endif
#ifdef _DHT
	int rhum        :7;  // rhumdity: 0..100 (4 or 5% resolution?)
	int temp        :10; // temperature: -500..+500 (tenths, .5 resolution)
#endif
	int ctemp       :10; // atmega temperature: -500..+500 (tenths)
#ifdef _MEM
	int memfree     :16;
#endif
#ifdef _RFM12BLOBAT
	byte lobat      :1;  // supply voltage dropped under 3.1V: 0..1
#endif
} payload;


/* *** /Report variables }}} *** */

/* *** Scheduled tasks *** {{{ */

enum {
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


/* *** /Scheduled tasks }}} *** */

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
}

void writeConfig(Config &c)
{
}


/* *** /EEPROM config }}} *** */

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
ISR(PCINT2_vect) { pir.poll(); }

#endif
/* *** /PIR support }}} *** */

#if _DHT
/* DHT temp/rh sensor 
 - AdafruitDHT
*/

#endif // DHT

#if _RFM12B
/* HopeRF RFM12B 868Mhz digital radio */

#endif //_RFM12B

#if _LCD84x48
/* Nokkia 5110 display */


#endif // LCD84x48

#if _DS
/* Dallas OneWire bus with registration for DS18B20 temperature sensors */
OneWire ds(DS_PIN);

uint8_t ds_count = 0;
uint8_t ds_search = 0;
volatile int ds_value[ds_count];
volatile int ds_value[8]; // take on 8 DS sensors in report

enum { DS_OK, DS_ERR_CRC };

#endif // DS

#if _NRF24
/* nRF24L01+: nordic 2.4Ghz digital radio  */

RF24 radio(CE, CS);

// Network uses that radio
RF24Network/*Debug*/ network(radio);

// Address of our node
const uint16_t this_node = 1;

// Address of the other node
const uint16_t other_node = 0;

#endif // NRF24

#if _RTC
#endif // RTC

#if _HMC5883L
/* Digital magnetometer I2C module */

#endif // HMC5883L


/* *** /Peripheral devices }}} *** */

/* *** Peripheral hardware routines *** {{{ */

#if LDR_PORT
#endif

/* *** PIR support *** {{{ */
#if PIR_PORT
#endif
/* *** /PIR support *** }}} */

#if _DHT
/* DHT temp/rh sensor 
 - AdafruitDHT
*/

#endif // DHT

#if _RFM12B
/* HopeRF RFM12B 868Mhz digital radio routines */


#endif // RFM12B

#if _LCD84x48


#endif // LCD84x48

#if _DS
/* Dallas DS18B20 thermometer routines */

static int ds_readdata(uint8_t addr[8], uint8_t data[12]) {
	byte i;
	byte present = 0;

	ds.reset();
	ds.select(addr);
	ds.write(0x44,1);         // start conversion, with parasite power on at the end

	serialFlush();
	Sleepy::loseSomeTime(800); 
	//delay(1000);     // maybe 750ms is enough, maybe not
	// we might do a ds.depower() here, but the reset will take care of it.

	present = ds.reset();
	ds.select(addr);
	ds.write(0xBE);         // Read Scratchpad

#if SERIAL && DEBUG_DS
	Serial.print(F("Data="));
	Serial.print(present,HEX);
	Serial.print(" ");
#endif
	for ( i = 0; i < 9; i++) {           // we need 9 bytes
		data[i] = ds.read();
#if SERIAL && DEBUG_DS
		Serial.print(i);
		Serial.print(':');
		Serial.print(data[i], HEX);
		Serial.print(" ");
#endif
	}

	uint8_t crc8 = OneWire::crc8( data, 8);

#if SERIAL && DEBUG_DS
	Serial.print(F(" CRC="));
	Serial.print( crc8, HEX);
	Serial.println();
	serialFlush();
#endif

	if (crc8 != data[8]) {
		return DS_ERR_CRC; 
	} else { 
		return DS_OK; 
	}
}

static int ds_conv_temp_c(uint8_t data[8], int SignBit) {
	int HighByte, LowByte, TReading, Tc_100;
	LowByte = data[0];
	HighByte = data[1];
	TReading = (HighByte << 8) + LowByte;
	SignBit = TReading & 0x8000;  // test most sig bit
	if (SignBit) // negative
	{
		TReading = (TReading ^ 0xffff) + 1; // 2's comp
	}
	Tc_100 = (6 * TReading) + TReading / 4;    // multiply by (100 * 0.0625) or 6.25
	return Tc_100;
}

// FIXME: returns 8500 value at times, drained parasitic power?
static int readDS18B20(uint8_t addr[8]) {
	byte data[12];
	int SignBit;

	int result = ds_readdata(addr, data);	
	
	if (result != DS_OK) {
#if SERIAL
		Serial.println(F("CRC error in ds_readdata"));
		serialFlush();
#endif
		return 0;
	}

	int Tc_100 = ds_conv_temp_c(data, SignBit);

	if (SignBit) {
		return 0 - Tc_100;
	} else {
		return Tc_100;
	}
}

static uint8_t readDSCount() {
	uint8_t ds_count = EEPROM.read(3);
	if (ds_count == 0xFF) return 0;
	return ds_count;
}

static void updateDSCount(uint8_t new_count) {
	if (new_count != ds_count) {
		EEPROM.write(3, new_count);
		ds_count = new_count;
	}
}

static void writeDSAddr(uint8_t addr[8]) {
	int l = 4 + ( ( ds_search-1 ) * 8 );
	for (int i=0;i<8;i++) {
		EEPROM.write(l+i, addr[i]);
	}
}

static void readDSAddr(int a, uint8_t addr[8]) {
	int l = 4 + ( a * 8 );
	for (int i=0;i<8;i++) {
		addr[i] = EEPROM.read(l+i);
	}
}

static void printDSAddrs(void) {
	for (int q=0;q<ds_count;q++) {
		Serial.print("Mem Address=");
		int l = 4 + ( q * 8 );
		int r[8];
		for (int i=0;i<8;i++) {
			r[i] = EEPROM.read(l+i);
			Serial.print(i);
			Serial.print(':');
			Serial.print(r[i], HEX);
			Serial.print(' ');
		}
		Serial.println();
	}
}

static void findDS18B20s(void) {
	byte i;
	byte addr[8];

	if (!ds.search(addr)) {
#if SERIAL && DEBUG_DS
		Serial.println("No more addresses.");
#endif
		ds.reset_search();
		if (ds_search != ds_count) {
#if DEBUG || DEBUG_DS
			Serial.print("Found ");
			Serial.print(ds_search );
			Serial.println(" addresses.");
			Serial.print("Previous search found ");
			Serial.print(ds_count);
			Serial.println(" addresses.");
#endif
		}
		updateDSCount(ds_search);
		ds_search = 0;
		return;
	}

	if ( OneWire::crc8( addr, 7) != addr[7]) {
#if DEBUG || DEBUG_DS
		Serial.println("CRC is not valid!");
#endif
		return;
	}

	ds_search++;

#if DEBUG || DEBUG_DS
	Serial.print("New Address=");
	for( i = 0; i < 8; i++) {
		Serial.print(i);
		Serial.print(':');
		Serial.print(addr[i], HEX);
		Serial.print(" ");
	}
	Serial.println();
#endif

	writeDSAddr(addr);
}

#endif // DS

#if _NRF24
/* Nordic nRF24L01+ radio routines */


#endif // NRF24 funcs

#if _RTC
#endif // RTC

#if _HMC5883L
/* Digital magnetometer I2C routines */


#endif // HMC5883L


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
	idle.set(UI_IDLE);
}

static void ser_configVersionCmd(void) {
	//Serial.print("v ");
	//showNibble(static_config.version);
	//Serial.println("");
	idle.set(UI_IDLE);
}

void memStat() {
	int free = freeRam();
	int used = usedRam();
	cmdIo.print("m ");
	cmdIo.print(free);
	cmdIo.print(' ');
	cmdIo.print(used);
	cmdIo.print(' ');
	cmdIo.println();
	idle.set(UI_IDLE);
}

static void ser_test_cmd(void) {
	Serial.println("Foo");
	idle.set(UI_IDLE);
}


/* UART commands }}} */

/* *** Wire *** {{{ */

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


/* Wire commands */

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

/* *** Wire }}} *** */

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


/* *** /Initialization routines }}} *** */

/* *** Run-time handlers *** {{{ */

void doReset(void)
{
	doConfig();

#ifdef _DBG_LED
	pinMode(_DBG_LED, OUTPUT);
#endif
	//pinMode(BL, OUTPUT);
	//digitalWrite(BL, LOW ^ BL_INVERTED);
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

#if PIR_PORT

// send packet and wait for ack when there is a motion trigger
void doTrigger(void)
{
	payload.moved = pir.state();
    #if DEBUG
        Serial.print("PIR ");
//        Serial.print((int) );
    #endif
	doReport();
}
#endif // PIR_PORT

void uiStart()
{
	idle.set(UI_IDLE);
	if (!ui) {
		ui = true;
	}
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
#if SERIAL
			serialFlush();
#endif
#ifdef _DBG_LED
			blink(_DBG_LED, 2, 25);
#endif
			break;

		case REPORT:
			debugline("REPORT");
//			payload.msgtype = REPORT_MSG;
			doReport();
			serialFlush();
			break;

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
//	{ 'c', 0, configCmd },
	{ 'm', 0, memStat },
	{ 'n', 0, ser_configNodeCmd },
//	{ 'v', 0, configVersionCmd },
//	{ 'N', 3, configSetNodeCmd },
//	{ 'C', 1, configNodeIDCmd },
//	{ 'W', 1, configEEPROM },
//	{ 'E', 0, eraseEEPROM },
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
#if _DS
	bool ds_reset = digitalRead(7);
	if (ds_search || ds_reset) {
		if (ds_reset) {
			Serial.println("Reset triggered");
		}
		findDS18B20s();
		return;
	}
#endif
	if (ui_irq) {
		debugline("Irq");
		ui_irq = false;
		uiStart();
		//analogWrite(BL, 0xAF ^ BL_INVERTED);
	}
	debug_ticks();

#if PIR_PORT
	if (pir.triggered()) {
		doTrigger();
	}
#endif

	char task = scheduler.poll();
	if (-1 < task && task < IDLE) {
		runScheduler(task);
	}
	if (ui) {
		parser.poll();
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

