/*

	Lcd84x48

=Features
- Using Nokia 5110 display board and Arduino pro mini clone.
- Using to about 30mA with background fully lit.
- Low-power about 60 to 70 uA in standby mode.

=ToDo
- Start using a simple statemachine to enter into different power/function
  states.
- Investigate hardware for external interrupts. Standard specs do not seem to
  allow an wake-up from power-off using serial, but look at TWI address-match.
- Watchdog wakeup could use abit of work to do better scheduling.
- LCD dims when idle mode kicks in, would like to elaborate using an LDR or
  photov. cel.

2014-06-22 Started

=Description
Using the scheduler pollWaiting there is a continious measure and report tasks
cycle running using minimal power. This follows JeeLab's roomNode low power
sensor node and JeeLib. 

During low power, only INT0 or INT1, WDT, and TWI address-match events will wake
the Atmega. In this mode, loop halts at scheduler.pollWaiting where the
scheduler uses the WDT to wake up at some remote time.

One switch generates a RISING event for INTO, making the next loop enter into
high power mode with no delays. This enables waiting for other serial and button
events, starts up the LCD and the backlight.

Successive states can trim power-usage a bit in this active mode. Current code uses an
idle timer (that should be reset on ui/serial events), 
which on timeout sets a second timer stdby 
which finally on timeout will turn off the UI mode making next loop return to the
pollWaiting routines.

During the idle and stdby timers various schemes are possible, 
depends on the used hardware etc.

*/

/* *** Globals and sketch configuration *** */
#define DEBUG           1 /* Enable trace statements */
#define SERIAL          1 /* Enable serial */
							
#define _MEM            1   // Report free memory 
#define _LCD84x48       1
							
#define REPORT_EVERY    5   // report every N measurement cycles
#define SMOOTH          5   // smoothing factor used for running averages
#define MEASURE_PERIOD  300  // how often to measure, in tenths of seconds
#define UI_IDLE         3000  // tenths of seconds idle time, ...
#define UI_STDBY        8000  // ms
#define MAXLENLINE      79
							

// Definition of interrupt names
#include <avr/io.h>
// ISR interrupt service routine
#include <avr/interrupt.h>

#include <JeeLib.h>
#include <OneWire.h>
#include <PCD8544.h>
#include <DotmpeLib.h>
#include <mpelib.h>


const String sketch = "X-Lcd84x48";
const int version = 0;
static String node = "lcd84x48-1";
// determined upon handshake 
char node_id[7];


/* IO pins */
static const byte ledPin = 13;
static const byte backlightPin = 9;


volatile bool ui_irq;

static bool ui;

MpeSerial mpeser (57600);

MilliTimer idle, stdby;

/* *** InputParser {{{ */

extern InputParser::Commands cmdTab[] PROGMEM;
byte* buffer = (byte*) malloc(50);
InputParser parser (buffer, 50, cmdTab);

/* }}} *** */

/* *** Report variables *** {{{ */


static byte reportCount;    // count up until next report, i.e. packet send

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

bool printSchedRunning()
{
	for (int i=0;i<TASK_END;i++) {
		Serial.print(i);
		Serial.print(' ');
		Serial.print(schedbuf[i]);
		Serial.println();
	}
}
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

void debug_task(char task) {
	//if ((tick % 20) == 0) {
		if (task == -2) { // nothing running
			Serial.print('n');
		} else if (task == -1) { // waiting
			Serial.print('w');
		}
	//}
}

/* *** Peripheral devices *** {{{ */

#if LDR_PORT
#endif

#if _DHT
/* DHT temp/rh sensor */
#endif // DHT

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

static PCD8544 lcd84x48(3, 4, 5, 6, 7); /* SCLK, SDIN, DC, RESET, SCE */


#endif // LCD84x48

#if _DS
/* Dallas OneWire bus with registration for DS18B20 temperature sensors */



#endif // DS

#if _NRF24
/* nRF24L01+: nordic 2.4Ghz digital radio  */

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
//    lcd84x48.send( LOW, 0x14 );  // LCD bias mode 1:48. //0x13
//    lcd84x48.send( LOW, 0x0C );  // LCD in normal mode.
	lcd84x48.send( LOW, 0x20 );  // LCD Extended Commands toggle off

	pinMode(backlightPin, OUTPUT);
	analogWrite(backlightPin, 0xaf);
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
}

void initLibs()
{

}


/* *** /Initialization routines *** }}} */

/* *** Run-time handlers *** {{{ */

void doReset(void)
{
	doConfig();

	pinMode(ledPin, OUTPUT);
	pinMode(backlightPin, OUTPUT);
	digitalWrite(backlightPin, LOW);
	ui_irq = false;
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
}

void start_ui() 
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

		case STDBY:
			debugline("STDBY");
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
	Serial.print(F("Free RAM: "));
	Serial.println(freeRam());
#endif
	serialFlush();
#endif

	initLibs();

	doReset();
	attachInterrupt(INT0, irq0, RISING);
	ui_irq = true;
}

void loop(void)
{
	if (ui_irq) {
		debugline("Irq");
		ui_irq = false;
		start_ui();
	}
	debug_ticks();
	char task = scheduler.poll();
	debug_task(task);
	if (0 < task && task < 0xFF) {
		runScheduler(task);
	}
	else if (ui) {
		parser.poll();
		if (idle.poll()) {
			debugline("Idle");
			digitalWrite(backlightPin, 0);
			analogWrite(backlightPin, 0x1f);
			stdby.set(UI_STDBY);
		} else if (stdby.poll()) {
			debugline("StdBy");
			digitalWrite(backlightPin, LOW);
			lcd84x48.clear();
			lcd84x48.stop();
			ui = false;
		} else if (!stdby.idle()) {
			// XXX toggle UI stdby Power, got to some lower power mode..
			delay(30);
		}
		lcd_printTicks();
	} else {
		blink(ledPin, 1, 15);
		debugline("Sleep");
		serialFlush();
		char task = scheduler.pollWaiting();
		debugline("WakeUp");
		debug_task(task);
		if (0 < task && task < 0xFF) {
			runScheduler(task);
		}
	}
}

/* }}} *** */

