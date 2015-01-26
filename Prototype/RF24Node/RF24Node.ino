/* 

RF24Node

- Low power scheduled node with UI loop
  and serial UI.

TODO: get NRF24 client stuff running

2015-01-15 - Started based on LogReader84x48,
		integrating RF24Network helloworld_tx example.
2015-01-17 - Simple RF24Network send working together 
		with Lcd84x48 sketch.

 */


/* *** Globals and sketch configuration *** */
#define SERIAL          1   // set to 1 to enable serial interface
#define DEBUG           1 /* Enable trace statements */
							
#define _MEM            1   // Report free memory 
#define _LCD84x48       1
#define _NRF24          1
							
#define REPORT_EVERY    2   // report every N measurement cycles
#define SMOOTH          5   // smoothing factor used for running averages
#define MEASURE_PERIOD  30  // how often to measure, in tenths of seconds
#define UI_IDLE         4000  // tenths of seconds idle time, ...
#define UI_STDBY        8000  // ms
#define BL_INVERTED     0xFF
							
#define MAXLENLINE      79
							

// Definition of interrupt names
#include <avr/io.h>
// ISR interrupt service routine
#include <avr/interrupt.h>

#include <JeeLib.h>
#include <OneWire.h>
#include <PCD8544.h>
#include <SPI.h>
#include <RF24.h>
#include <RF24Network.h>
#include <DotmpeLib.h>
#include <mpelib.h>




const String sketch = "X-RF24Node";
const int version = 0;

char node[] = "rf24n";

volatile bool ui_irq;
bool ui;

byte bl_level = 0xff;

int out = 20;

/* IO pins */
//              RXD      0
//              TXD      1
//              INT0     2     UI IRQ
#       define SCLK     3  // LCD84x48
#       define SDIN     4  // LCD84x48
#       define DC       5  // LCD84x48
#       define RESET    6  // LCD84x48
#       define SCE      7  // LCD84x48
#       define CS       8  // NRF24
#       define CE       9  // NRF24
#       define BL       10 // PWM Backlight
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

/* *** InputParser *** {{{ */

extern InputParser::Commands cmdTab[] PROGMEM;
byte* buffer = (byte*) malloc(50);
InputParser parser (buffer, 50, cmdTab);


/* *** /InputParser }}} *** */

/* *** Report variables *** {{{ */


static byte reportCount;    // count up until next report, i.e. packet send

// This defines the structure of the packets which get sent out by wireless:
struct {
	int ctemp       :10; // atmega temperature: -500..+500 (tenths)
#ifdef _MEM
	int memfree     :16;
#endif
} payload;


/* *** /Report variables }}} *** */

/* *** Scheduled tasks *** {{{ */

enum {
	ANNOUNCE,
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

bool schedRunning()
{
	for (int i=0;i<TASK_END;i++) {/*
		Serial.print(i);
		Serial.print(' ');
		Serial.print(schedbuf[i]);
		Serial.println(); */
		if (schedbuf[i] != 0 && schedbuf[i] != 0xFFFF) {
			return true;
		}
	}
	return false;
}


/* *** /Scheduled tasks }}} *** */

/* *** EEPROM config *** {{{ */

// See Prototype Node or SensorNode
//#define CONFIG_VERSION "nx1"
//#define CONFIG_START 0


struct Config {
	char node[4];
	int node_id;
	int version;
	char config_id[4];
} static_config = {
	/* default values */
//	{ node[0], node[1], 0, 0 }, 0, version, CONFIG_VERSION
};

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
#endif
/* *** /PIR support }}} *** */

#if _DHT
/* DHT temp/rh sensor 
 - AdafruitDHT
*/
//DHT dht(DHT_PIN, DHTTYPE); // Adafruit DHT
//DHTxx dht (DHT_PIN); // JeeLib DHT
//#define DHTTYPE DHT11   // DHT 11 
//#define DHTTYPE DHT22   // DHT 22  (AM2302)
#if DHT_HIGH 
DHT dht (DHT_PIN, DHT21); 
//DHT dht (DHT_PIN, DHT22); 
// AM2301
// DHT21, AM2302?
#else
DHT dht (DHT_PIN, DHT11);
#endif
#endif // DHT

#if _DHT2
DHT dht2 (DHT2_PIN, DHT11);
#endif

#if _RFM12B
/* HopeRF RFM12B 868Mhz digital radio */

#endif //_RFM12B

#if _LCD84x48
/* Nokkia 5110 display */

// The dimensions of the LCD (in pixels)...
static const byte LCD_WIDTH = 84;
static const byte LCD_HEIGHT = 48;

// The number of lines for the temperature chart...
static const byte CHART_HEIGHT = 3;

// A custom "degrees" symbol...
static const byte DEGREES_CHAR = 1;
static const byte degrees_glyph[] = { 0x00, 0x07, 0x05, 0x07, 0x00 };

// A bitmap graphic (10x2) of a thermometer...
static const byte THERMO_WIDTH = 10;
static const byte THERMO_HEIGHT = 2;
static const byte thermometer[] = { 0x00, 0x00, 0x48, 0xfe, 0x01, 0xfe, 0x00, 0x02, 0x05, 0x02,
	0x00, 0x00, 0x62, 0xff, 0xfe, 0xff, 0x60, 0x00, 0x00, 0x00};

static PCD8544 lcd84x48(SCLK, SDIN, DC, RESET, SCE);


#endif // LCD84x48

#if _DS
/* Dallas OneWire bus with registration for DS18B20 temperature sensors */


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

void lcd_start()
{
	lcd84x48.begin(LCD_WIDTH, LCD_HEIGHT);

	// Register the custom symbol...
	lcd84x48.createChar(DEGREES_CHAR, degrees_glyph);

	lcd84x48.send( LOW, 0x21 );  // LCD Extended Commands on
	lcd84x48.send( LOW, 0x07 );  // Set Temp coefficent. //0x04--0x07
    lcd84x48.send( LOW, 0x12 );  // LCD bias mode 1:48. //0x13
    lcd84x48.send( LOW, 0x0C );  // LCD in normal mode.
	lcd84x48.send( LOW, 0x20 );  // LCD Extended Commands toggle off
}

void lcd_printWelcome(void)
{
	lcd84x48.setCursor(6, 0);
	lcd84x48.print(node);
	lcd84x48.setCursor(0, 5);
	lcd84x48.print(sketch);
}

void lcd_printTicks(void)
{
	lcd84x48.setCursor(10, 2);
	lcd84x48.print("tick ");
	lcd84x48.print(tick);
	lcd84x48.setCursor(10, 3);
	lcd84x48.print("idle ");
	lcd84x48.print(idle.remaining()/100);
	lcd84x48.setCursor(10, 4);
	lcd84x48.print("stdby ");
	lcd84x48.print(stdby.remaining()/100);
}


#endif // LCD84x48

#if _DS
/* Dallas DS18B20 thermometer routines */

static int ds_readdata(uint8_t addr[8], uint8_t data[12]) {
}

static int ds_conv_temp_c(uint8_t data[8], int SignBit) {
}

static int readDS18B20(uint8_t addr[8]) {

}

static uint8_t readDSCount() {
}

static void updateDSCount(uint8_t new_count) {
}

static void writeDSAddr(uint8_t addr[8]) {
}

static void readDSAddr(int a, uint8_t addr[8]) {
}

static void printDSAddrs(void) {
}

static void findDS18B20s(void) {
}


#endif // DS

#if _NRF24
/* Nordic nRF24L01+ radio routines */

void rf24_init()
{
}

void rf24_run()
{
}


#endif // NRF24 funcs

#if _RTC
#endif // RTC

#if _HMC5883L
/* Digital magnetometer I2C routines */

void Init_HMC5803L(void)
{
}

int HMC5803L_Read(byte Axis)
{
	int Result;

	return Result;
}
#endif // HMC5883L


/* *** /Peripheral hardware routines }}} *** */

/* *** UI *** {{{ */

//ISR(INT0_vect) 
void irq0()
{
	ui_irq = true;
	//Sleepy::watchdogInterrupts(0);
}

byte SAMPLES = 0x1f;
uint8_t keysamples = SAMPLES;
static uint8_t keyactive = 4;
static uint8_t keypins[4] = {
	8, 11, 10, 2
};
uint8_t keysample[4] = { 0, 0, 0, 0 };
bool keys[4];

void readKeys(void) {
	for (int i=0;i<4;i++) {
		if (digitalRead(keypins[i])) {
			keysample[i]++;
		}
	}
}

bool keyActive(void) {
	keysamples = SAMPLES;
	for (int i=0;i<4;i++) {
		keys[i] = (keysample[i] > keyactive);
		keysample[i] = 0;
	}
	for (int i=0;i<4;i++) {
		if (keys[i])
			return true;
	}
	return false;
}

void printKeys(void) {
	for (int i=0;i<4;i++) {
		Serial.print(keys[i]);
		Serial.print(" ");
	}
	Serial.println();
}


/* *** /UI }}} *** */

/* UART commands {{{ */

void helpCmd() {
	Serial.println("Help!");
	idle.set(UI_IDLE);
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

/* Wire init {{{ */

// See I2CPIR

/* Wire init }}} */

/* Wire commands {{{ */

/* Wire commands }}} */;

/* Wire handling {{{ */
/* Wire Handling }}} */

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
#if _NRF24
	SPI.begin();
	radio.begin();
	network.begin(/*channel*/ 90, /*node address*/ this_node);
#endif // NRF24
}


/* *** /Initialization routines }}} *** */

/* *** Run-time handlers *** {{{ */

void doReset(void)
{
	doConfig();

#ifdef _DBG_LED
	pinMode(_DBG_LED, OUTPUT);
#endif
	pinMode(BL, OUTPUT);
	digitalWrite(BL, LOW ^ BL_INVERTED);
	attachInterrupt(INT0, irq0, RISING);
	ui_irq = true;
	tick = 0;

#if _NRF24
	rf24_init();
#endif //_NRF24

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
	byte firstTime = payload.ctemp == 0; // special case to init running avg

	int ctemp = internalTemp();
	payload.ctemp = smoothedAverage(payload.ctemp, ctemp, firstTime);

#if _MEM
	payload.memfree = freeRam();
#if SERIAL && DEBUG_MEASURE
	Serial.print("MEM free ");
	Serial.println(payload.memfree);
#endif
#endif //_MEM
}

// periodic report, i.e. send out a packet and optionally report on serial port
void doReport(void)
{
#if SERIAL
	Serial.print(node);
	Serial.print(' ');
	Serial.print((int) payload.ctemp);
	Serial.print(' ');
#if _MEM
	Serial.print((int) payload.memfree);
	Serial.print(' ');
#endif
	Serial.println();
	serialFlush();
#endif// SERIAL

#ifdef _RFM12B
#endif

#ifdef _NRF24
	RF24NetworkHeader header(/*to node*/ other_node);
	bool ok = network.write(header, &payload, sizeof(payload));
	if (ok)
		debugline("ACK");
	else
		debugline("NACK");
#endif // NRF24
}

void uiStart()
{
	idle.set(UI_IDLE);
	if (!ui) {
		ui = true;
		lcd_start();
		lcd_printWelcome();
	}
}

void runScheduler(char task)
{
	switch (task) {

		case ANNOUNCE:
			// TODO: see SensorNode
			debugline("ANNOUNCE");
			Serial.print(node);
			Serial.println();
			serialFlush();
			break;

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
	{ 'h', 0, helpCmd },
	{ 'v', 2, valueCmd },
	{ 'm', 0, measureCmd },
	{ 'r', 0, reportCmd },
	{ 's', 0, stdbyCmd },
	{ 'x', 0, doReset },
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
	blink(_DBG_LED, 1, 15);
#endif
#if _NRF24
	// Pump the network regularly
	network.update();
#endif
	if (ui_irq) {
		debugline("Irq");
		ui_irq = false;
		uiStart();
		analogWrite(BL, 0xAF ^ BL_INVERTED);
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
			analogWrite(BL, 0x1F ^ BL_INVERTED);
			stdby.set(UI_STDBY);
		} else if (stdby.poll()) {
			debugline("StdBy");
			digitalWrite(BL, LOW ^ BL_INVERTED);
			lcd84x48.clear();
			lcd84x48.stop();
			ui = false;
		} else if (!stdby.idle()) {
			// XXX toggle UI stdby Power, got to some lower power mode..
			delay(30);
		}
		lcd_printTicks();
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

