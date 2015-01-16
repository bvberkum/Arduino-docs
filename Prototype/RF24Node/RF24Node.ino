/* 

RF24Node


2015-01-15 - Started based on LogReader84x48,
		integrating RF24Network helloworld_tx example.

 */


/* *** Globals and sketch configuration *** */
#define DEBUG           1 /* Enable trace statements */
#define SERIAL          1 /* Enable serial */
							
#define _MEM            1   // Report free memory 
#define _LCD84x48       1
#define _NRF24          1
							
#define REPORT_EVERY    2   // report every N measurement cycles
#define SMOOTH          5   // smoothing factor used for running averages
#define MEASURE_PERIOD  3  // how often to measure, in tenths of seconds
#define SCHEDULER_DELAY 100 //ms
#define UI_DELAY        10000
#define UI_IDLE         4000  // tenths of seconds idle time, ...
#define UI_STDBY        8000  // ms
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


/* IO pins */
#       define SCLK 3
#       define SDIN 4
#       define DC 5
#       define RESET 6
#       define SCE 7
#       define CS       8
#       define CE       9
static const byte ledPin = 13;
static const byte backlightPin = 11;


volatile bool ui_irq;

static bool ui;

byte bl_level = 0xff;

int out = 20;

MpeSerial mpeser (57600);

MilliTimer idle, stdby;

/* *** InputParser {{{ */

extern InputParser::Commands cmdTab[] PROGMEM;
byte* buffer = (byte*) malloc(50);
InputParser parser (buffer, 50, cmdTab);

/* }}} *** */

/* *** Report variables *** {{{ */


static byte reportCount;    // count up until next report, i.e. packet send

// This defines the structure of the packets which get sent out by wireless:
struct {
	int ctemp       :10; // atmega temperature: -500..+500 (tenths)
#if _MEM
	int memfree     :16;
#endif
} payload;


/* *** /Report variables *** }}} */

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


/* *** /Scheduled tasks *** }}} */

bool hex_output = true;

static void showNibble (byte nibble) {
    char c = '0' + (nibble & 0x0F);
    if (c > '9')
        c += 7;
    Serial.print(c);
}

static void showByte (byte value) {
    if (hex_output) {
        showNibble(value >> 4);
        showNibble(value);
    } else
        Serial.print((word) value);
}

void debug_task(char task) {
	if (task == -2) { // nothing running
		Serial.print('n');
	} else if (task == -1) { // waiting
		Serial.print('w');
	}
}

/* *** Peripheral devices *** {{{ */

#if LDR_PORT
Port ldr (LDR_PORT);
#endif

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

static PCD8544 lcd84x48(SCLK, SDIN, DC, RESET, SCE); /* SCLK, SDIN, DC, RESET, SCE */


#endif // LCD84x48

#if _DS
/* Dallas OneWire bus with registration for DS18B20 temperature sensors */



#endif // DS

#if _NRF24
/* nRF24L01+: nordic 2.4Ghz digital radio  */

RF24 radio(CE, CS);

// Network uses that radio
RF24Network network(radio);

// Address of our node
const uint16_t this_node = 1;

// Address of the other node
const uint16_t other_node = 0;

#endif // NRF24

#if _LCD
#endif //_LCD

#if _RTC
#endif //_RTC

#if _HMC5883L
/* Digital magnetometer I2C module */

#endif // HMC5883L


/* *** /Peripheral devices }}} *** */

/* *** EEPROM config *** {{{ */

/* *** /EEPROM config *** }}} */


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

#if _LCD84x48


void lcd_start()
{
	lcd84x48.begin(LCD_WIDTH, LCD_HEIGHT);

	// Register the custom symbol...
	lcd84x48.createChar(DEGREES_CHAR, degrees_glyph);

	lcd84x48.send( LOW, 0x21 );  // LCD Extended Commands on
	lcd84x48.send( LOW, 0x07 );  // Set Temp coefficent. //0x04--0x07
    lcd84x48.send( LOW, 0x14 );  // LCD bias mode 1:48. //0x13
    lcd84x48.send( LOW, 0x0C );  // LCD in normal mode.
	lcd84x48.send( LOW, 0x20 );  // LCD Extended Commands toggle off

	pinMode(backlightPin, OUTPUT);
	analogWrite(backlightPin, 0x1f);
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
	lcd84x48.setCursor(30, 2);
	lcd84x48.print(tick);
	lcd84x48.setCursor(0, 3);
	lcd84x48.print(idle.remaining());
	lcd84x48.setCursor(42, 3);
	lcd84x48.print(stdby.remaining());
}
#endif //_LCD84x48

#if _DS
/* Dallas OneWire bus with registration for DS18B20 temperature sensors */


#endif // DS

#if _NRF24
/* Nordic nRF24L01+ radio routines */

#endif // RF24 funcs

#if _LCD
#endif //_LCD

#if _RTC
#endif //_RTC

#if _HMC5883L
/* Digital magnetometer I2C routines */
#endif //_HMC5883L


/* *** /Peripheral hardware routines }}} *** */

/* *** Initialization routines *** {{{ */

void doConfig(void)
{
	/* load valid config or reset default config */
	//if (!loadConfig(static_config)) {
	//	writeConfig(static_config);
	//}
}

void initLibs()
{
#if _NRF24
	SPI.begin();
	radio.begin();
	network.begin(/*channel*/ 90, /*node address*/ this_node);
#endif // NRF24
}


/* *** /Initialization routines *** }}} */

/* *** Run-time handlers *** {{{ */

void doReset(void)
{
	doConfig();

	pinMode(ledPin, OUTPUT);
	pinMode(backlightPin, OUTPUT);
	digitalWrite(backlightPin, LOW);
	attachInterrupt(INT0, irq0, RISING);
	ui_irq = false;
	ui_irq = true;
	tick = 0;
	reportCount = REPORT_EVERY;     // report right away for easy debugging
	scheduler.timer(MEASURE, 0);    // get the measurement loop going
}

bool doAnnounce()
{
	return false;
}

void doMeasure()
{
	byte firstTime = payload.ctemp == 0; // special case to init running avg

	payload.ctemp = smoothedAverage(payload.ctemp, internalTemp(), firstTime);

#if _MEM
	payload.memfree = freeRam();
#if SERIAL && DEBUG_MEASURE
	Serial.print("MEM free ");
	Serial.println(payload.memfree);
#endif
#endif

	serialFlush();
}

// periodic report, i.e. send out a packet and optionally report on serial port
void doReport(void)
{
#if SERIAL
	Serial.print(node);
	Serial.print(" ");
	Serial.print((int) payload.ctemp);
	Serial.print(' ');
#if _MEM
	Serial.print((int) payload.memfree);
	Serial.print(' ');
#endif
	Serial.println();
	serialFlush();
#endif//SERIAL

#if _NRF24
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
		digitalWrite(backlightPin, HIGH);
		lcd_start();
		lcd_printWelcome();
	}
}

void runScheduler(char task)
{
	switch (task) {

		case MEASURE:
			debugline("MEASURE");
			scheduler.timer(MEASURE, MEASURE_PERIOD);
			doMeasure();

			if (++reportCount >= REPORT_EVERY) {
				reportCount = 0;
				scheduler.timer(REPORT, 0);
			}
			serialFlush();
			break;

		case REPORT:
			debugline("REPORT");
			doReport();
			serialFlush();
			break;

		case WAITING:
			break;

		default:
			Serial.print(task, HEX);
			Serial.println('?');
	}
}


/* *** /Run-time handlers *** }}} */

/* *** InputParser handlers *** {{{ */

#if SERIAL

static void helpCmd() {
	Serial.println("Help!");
	idle.set(UI_IDLE);
}

static void valueCmd () {
	int v;
	parser >> v;
	Serial.print("value = ");
	Serial.println(v);
	idle.set(UI_IDLE);
}

static void reportCmd () {
	doReport();
	idle.set(UI_IDLE);
}

static void measureCmd() {
	doMeasure();
	idle.set(UI_IDLE);
}

static void stdbyCmd() {
	ui = false;
	digitalWrite(backlightPin, LOW);
}

InputParser::Commands cmdTab[] = {
	{ 'h', 0, helpCmd },
	{ 'v', 2, valueCmd },
	{ 'm', 0, measureCmd },
	{ 'r', 0, reportCmd },
	{ 's', 0, stdbyCmd },
	{ 'x', 0, doReset },
};

#endif // SERIAL

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
	//network.update();
	if (ui_irq) {
		debugline("Irq");
		ui_irq = false;
		uiStart();
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
			digitalWrite(backlightPin, 0);
			analogWrite(backlightPin, 0xaf);
			stdby.set(UI_STDBY);
		} else if (stdby.poll()) {
			debugline("StdBy");
			digitalWrite(backlightPin, HIGH);
			lcd84x48.clear();
			lcd84x48.stop();
			ui = false;
		}
		lcd_printTicks();
	} else {
		blink(ledPin, 1, 15);
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

