/* 

SD Card log reader with 84x48 display.

Current targets:
	- Display menu
	- SD card file log
	- Serial mpe-bus slave version

2014-06-15 - Started
2014-06-22 - Created Lcd84x48 based on this.

 */
// Definition of interrupt names
#include <avr/io.h>
// ISR interrupt service routine
#include <avr/interrupt.h>

#include <DotmpeLib.h>
#include <JeeLib.h>
#include <OneWire.h>
#include <PCD8544.h>


/** Globals and sketch configuration  */
#define DEBUG           1 /* Enable trace statements */
#define SERIAL          1 /* Enable serial */
							
#define _MEM            1   // Report free memory 
							
#define REPORT_EVERY    5   // report every N measurement cycles
#define SMOOTH          5   // smoothing factor used for running averages
#define MEASURE_PERIOD  100  // how often to measure, in tenths of seconds
#define SCHEDULER_DELAY 100 //ms
#define UI_DELAY        10000
#define UI_IDLE         4000  // tenths of seconds idle time, ...
#define UI_STDBY        8000  // ms
#define MAXLENLINE      79
//#define SRAM_SIZE       


String sketch = "X-LogReader84x48";
String version = "0";
String node = "logreader-1";

int tick = 0;
int pos = 0;


/* IO pins */
static const byte ledPin = 13;
static const byte backlightPin = 9;

MpeSerial mpeser (57600);

MilliTimer idle, stdby;

/* Command parser */
extern InputParser::Commands cmdTab[] PROGMEM;
byte* buffer = (byte*) malloc(50);
InputParser parser (buffer, 50, cmdTab);

/* Scheduled tasks */
enum { 
	USER,
	USER_POLL,
	USER_IDLE,
	USER_STDBY,
	MEASURE,
	REPORT,
	TASK_END };
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

volatile bool ui_irq;

static bool ui;

byte level = 0xff;

int out = 20;

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

static PCD8544 lcd(3, 4, 5, 6, 7); /* SCLK, SDIN, DC, RESET, SCE */

/* Report variables */

static byte reportCount;    // count up until next report, i.e. packet send

struct {
	int ctemp       :10; // atmega temperature: -500..+500 (tenths)
#if _MEM
	int memfree     :16;
#endif
} payload;

/** AVR routines */

int freeRam () {
	extern int __heap_start, *__brkval; 
	int v;
	return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

//int usedRam () {
//	return SRAM_SIZE - freeRam();
//}


/** ATmega routines */

double internalTemp(void)
{
	unsigned int wADC;
	double t;

	// The internal temperature has to be used
	// with the internal reference of 1.1V.
	// Channel 8 can not be selected with
	// the analogRead function yet.

	// Set the internal reference and mux.
	ADMUX = (_BV(REFS1) | _BV(REFS0) | _BV(MUX3));
	ADCSRA |= _BV(ADEN);  // enable the ADC

	delay(20);            // wait for voltages to become stable.

	ADCSRA |= _BV(ADSC);  // Start the ADC

	// Detect end-of-conversion
	while (bit_is_set(ADCSRA,ADSC));

	// Reading register "ADCW" takes care of how to read ADCL and ADCH.
	wADC = ADCW;

	// The offset of 324.31 could be wrong. It is just an indication.
	t = (wADC - 311 ) / 1.22;

	// The returned temperature is in degrees Celcius.
	return (t);
}


/** Generic routines */

static void serialFlush () {
#if SERIAL
#if ARDUINO >= 100
	Serial.flush();
#endif
	delay(2); // make sure tx buf is empty before going back to sleep
#endif
}

void blink(int led, int count, int length, int length_off=-1) {
	for (int i=0;i<count;i++) {
		digitalWrite (led, HIGH);
		delay(length);
		digitalWrite (led, LOW);
		(length_off > -1) ? delay(length_off) : delay(length);
	}
}
/*
static void showNibble (byte nibble) {
	char c = '0' + (nibble & 0x0F);
	if (c > '9')
		c += 7;
	Serial.print(c);
}

bool useHex = 0;

static void showByte (byte value) {
	if (useHex) {
		showNibble(value >> 4);
		showNibble(value);
	} else {
		Serial.print((int) value);
	}
}
*/

// utility code to perform simple smoothing as a running average
static int smoothedAverage(int prev, int next, byte firstTime =0) {
	if (firstTime)
		return next;
	return ((SMOOTH - 1) * prev + next + SMOOTH / 2) / SMOOTH;
}

void debugline(String msg) {
#if DEBUG
	Serial.println(msg);
#endif
}

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

/* Peripheral hardware routines */


/* Initialization routines */

void doConfig(void)
{
}

void lcd_start() 
{
	lcd.begin(LCD_WIDTH, LCD_HEIGHT);

	// Register the custom symbol...
	lcd.createChar(DEGREES_CHAR, degrees_glyph);

	lcd.send( LOW, 0x21 );  // LCD Extended Commands on
	lcd.send( LOW, 0x07 );  // Set Temp coefficent. //0x04--0x07
//    lcd.send( LOW, 0x14 );  // LCD bias mode 1:48. //0x13
//    lcd.send( LOW, 0x0C );  // LCD in normal mode.
	lcd.send( LOW, 0x20 );  // LCD Extended Commands toggle off

	pinMode(backlightPin, OUTPUT);
	analogWrite(backlightPin, 0xaf);
}


/* Run-time handlers */

void doReset(void)
{
	pinMode(backlightPin, OUTPUT);
	digitalWrite(backlightPin, LOW);
	ui_irq = false;

	tick = 0;
	reportCount = REPORT_EVERY;     // report right away for easy debugging
	//scheduler.timer(USER, 0);
	scheduler.timer(MEASURE, 0);
}

//bool doAnnounce()
//{
//}

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
}

void runScheduler()
{
	switch (scheduler.pollWaiting()) {
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
		case 0xFF:
			break;
	}
	return;
	if (ui_irq) {
		ui_irq = false;
	}
	switch (scheduler.poll()) {
		case USER:
			Serial.println("USER");
			level = 0xff;
			digitalWrite(backlightPin, HIGH);
			scheduler.cancel(USER_IDLE);
			scheduler.timer(USER_IDLE, UI_IDLE);
			//parser.poll();
			//if (keyActive()) {
			//	printKeys();
			//}
			//scheduler.cancel(USER_POLL);
			//scheduler.timer(USER_POLL, 1);//UI_DELAY);
			serialFlush();
			break;
		case USER_POLL:
			while (keysamples) {
				readKeys();
				//delay(10);
				keysamples--;
			}
			//scheduler.timer(USER, 1);
			break;
		case USER_IDLE:
			Serial.println("IDLE");
			while (level > 0x1f) {
				analogWrite(backlightPin, level);
				delay(10);
				level--;
			}
			digitalWrite(backlightPin, LOW);
			scheduler.cancel(USER_STDBY);
			scheduler.timer(USER_STDBY, UI_STDBY/10);
			serialFlush();
			break;
		case USER_STDBY:
#if DEBUG
			Serial.println("STDBY");
			serialFlush();
#endif
			break;
		case MEASURE:
#if DEBUG
			Serial.println("MEASURE");
#endif
			scheduler.timer(MEASURE, MEASURE_PERIOD);
			doMeasure();
			if (++reportCount >= REPORT_EVERY) {
				reportCount = 0;
				scheduler.timer(REPORT, 0);
			}
			serialFlush();
			break;
		case REPORT:
#if DEBUG
			Serial.println("REPORT");
#endif
			doReport();
			serialFlush();
			break;
		case TASK_END:
			Serial.print("task? ");
			break;
//		case -1: // waiting
//		case -2: // nothing
//			Serial.print(";");
//			serialFlush();
//			Sleepy::loseSomeTime(SCHEDULER_DELAY);
		default:
			out--;
			if (out == 0) {
				out=20;
				Serial.print(".");
			//Serial.print(scheduler.idle(USER));
			//Serial.print(' ');
			//Serial.print(scheduler.idle(USER_POLL));
			//Serial.print(' ');
			//Serial.print(scheduler.idle(USER_IDLE));
			//Serial.print(' ');
			//Serial.print(scheduler.idle(USER_STDBY));
			//Serial.print(' ');
			//Serial.print(scheduler.idle(MEASURE));
			//Serial.print(' ');
			//Serial.print(scheduler.idle(REPORT));
			//Serial.print(' ');
			//Serial.println(scheduler.idle(TASK_END));
				serialFlush();
			}
			if (scheduler.idle(USER_IDLE) && scheduler.idle(USER_STDBY)) {
				serialFlush();
				//scheduler.cancel(MEASURE);
				Sleepy::powerDown();
				//Sleepy::loseSomeTime(60000);
				//scheduler.timer(MEASURE, 0);
			}
			if (scheduler.idle(USER)) {
				Sleepy::loseSomeTime(UI_DELAY);
			} else {
				Sleepy::loseSomeTime(SCHEDULER_DELAY);
			}
			if (ui_irq) {
				ui_irq = false;
				scheduler.timer(USER, 0);
				Serial.println("USR IRQ");
				serialFlush();
			}
	}
}

static byte STDBY = 0xff;

#if SERIAL

/* InputParser handlers */

static void helpCommand () {
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
	{ 'h', 0, helpCommand },
	{ 'v', 2, valueCmd },
	{ 'm', 0, measureCmd },
	{ 'r', 0, reportCmd },
	{ 's', 0, stdbyCmd },
	{ 'x', 0, doReset },
};

#endif // SERIAL

/* Main */

void setup(void)
{
	mpeser.begin();
	mpeser.startAnnounce(sketch, version);
	delay(500);
	serialFlush();
	doReset();
	attachInterrupt(INT0, irq0, RISING);
	ui_irq = true;
}

void loop(void)
{
	if (ui_irq) { 
		debugline("Irq");
		ui_irq = false;
		ui = true;
		idle.set(UI_IDLE);
		digitalWrite(backlightPin, HIGH);
		lcd_start();
		lcd.setCursor(6, 0);
		lcd.print(node);
		lcd.setCursor(0, 5);
		lcd.print(sketch);
	}
#if DEBUG
	tick++;
	if ((tick % 20) == 0) {
		Serial.print('.');
		pos++;
	}
	if (pos > MAXLENLINE) {
		pos = 0;
		Serial.println();
	}
	serialFlush();
#endif
	if (ui) {
		parser.poll();
		if (idle.poll()) {
			debugline("Idle");
			digitalWrite(backlightPin, 0);
			analogWrite(backlightPin, 0x1f);
			stdby.set(UI_STDBY);
		} else if (stdby.poll()) {
			debugline("StdBy");
			digitalWrite(backlightPin, LOW);
			ui = false;
		}
		lcd.setCursor(12, 2);
		lcd.print(tick);
	} else {
		blink(ledPin, 1, 15);
		runScheduler();
		if (!schedRunning()) {
			debugline("Sleep");
			digitalWrite(backlightPin, LOW);
			lcd.stop();
			serialFlush();
			Sleepy::loseSomeTime(10000);
			debugline("WakeUp");
		}
	}
}

