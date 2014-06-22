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
// Definition of interrupt names
#include < avr/io.h >
// ISR interrupt service routine
#include < avr/interrupt.h >

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
#define MEASURE_PERIOD  300  // how often to measure, in tenths of seconds
#define UI_IDLE         3000  // tenths of seconds idle time, ...
#define UI_STDBY        8000  // ms
#define MAXLENLINE      79


static String sketch = "X-LogReader84x48";
static String version = "0";
static String node = "logreader-1";

static const byte ledPin = 13;
static const byte backlightPin = 9;

MpeSerial mpeser (57600);

MilliTimer idle, stdby;

extern InputParser::Commands cmdTab[] PROGMEM;
byte* buffer = (byte*) malloc(50);
InputParser parser (buffer, 50, cmdTab);

/* Scheduled tasks */
enum { MEASURE, REPORT, PING };
static word schedbuf[PING];
Scheduler scheduler (schedbuf, PING);
// has to be defined because we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

bool printSchedRunning()
{
	for (int i=0;i<PING;i++) {
		Serial.print(i);
		Serial.print(' ');
		Serial.print(schedbuf[i]);
		Serial.println();
	}
}
bool schedRunning()
{
	for (int i=0;i<PING;i++) {/*
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

int tick = 0;
int pos = 0;

volatile bool ui_irq;

static bool ui;



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

void blink (int led, int count, int length) {
	for (int i=0;i<count;i++) {
		digitalWrite (led, HIGH);
		delay(length);
		digitalWrite (led, LOW);
		delay(length);
	}
}

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

// utility code to perform simple smoothing as a running average
static int smoothedAverage(int prev, int next, byte firstTime =0) {
	if (firstTime)
		return next;
	return ((SMOOTH - 1) * prev + next + SMOOTH / 2) / SMOOTH;
}

void debug(String msg) {
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

static bool doAnnounce()
{
}

static void doMeasure()
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

static void doReport(void)
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

void lcd_printWelcome(void)
{
	lcd.setCursor(6, 0);
	lcd.print(node);
	lcd.setCursor(0, 5);
	lcd.print(sketch);
}

void lcd_printTicks(void)
{
	lcd.setCursor(30, 2);
	lcd.print(tick);
	lcd.setCursor(0, 3);
	lcd.print(idle.remaining());
	lcd.setCursor(42, 3);
	lcd.print(stdby.remaining());
}

void start_ui() 
{
	debug("Irq");
	ui_irq = false;
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
		//case WAKE:
		//	ui = true;
		//	scheduler.timer(IDLE, IDLE_DELAY);
		//	break;
		case MEASURE:
			debug("MEASURE");
			//printSchedRunning();
			scheduler.timer(MEASURE, MEASURE_PERIOD);
			scheduler.timer(PING, 10);
			//printSchedRunning();
			doMeasure();
			if (++reportCount >= REPORT_EVERY) {
				reportCount = 0;
				scheduler.timer(REPORT, 0);
			}
			serialFlush();
			break;
		case REPORT:
			debug("REPORT");
			doReport();
			serialFlush();
			break;
		case PING:
			debug("PING");
			serialFlush();
			break;
		case 0xFF:
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

static void reset(void)
{
	reportCount = REPORT_EVERY;     // report right away for easy debugging
	scheduler.timer(MEASURE, 0);
	pinMode(ledPin, OUTPUT);
	pinMode(backlightPin, OUTPUT);
	digitalWrite(backlightPin, LOW);
	ui_irq = false;
	tick = 0;
}

void debug_task(char task) {
	//if ((tick % 20) == 0) {
		if (task == -2) { // nothing running
			Serial.print('n');
		} else if (task == -1) { // waiting
			Serial.print('w');
		}
	//}
}

void debug_ticks(void)
{
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
}

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
	{ 'x', 0, reset },
};

#endif // SERIAL

/* Main */

void setup(void)
{
	mpeser.begin();
	mpeser.startAnnounce(sketch, version);
	serialFlush();
	delay(500);
	reset();
	attachInterrupt(INT0, irq0, RISING);
	ui_irq = true;
}

void loop(void)
{
	if (ui_irq) {
		start_ui();
	}
	debug_ticks();
	char task = scheduler.poll();
	debug_task(task);
	if (0 < task && task < 0xFF) {
		runScheduler(task);
	} else if (ui) {
		parser.poll();
		if (idle.poll()) {
			debug("Idle");
			digitalWrite(backlightPin, 0);
			analogWrite(backlightPin, 0x1f);
			stdby.set(UI_STDBY);
		} else if (stdby.poll()) {
			debug("StdBy");
			digitalWrite(backlightPin, LOW);
			lcd.clear();
			lcd.stop();
			ui = false;
		} else if (!stdby.idle()) {
			// XXX toggle UI stdby Power, got to some lower power mode..
			delay(30);
		}
		lcd_printTicks();
	} else {
		blink(ledPin, 1, 15);
		debug("Sleep");
		serialFlush();
		char task = scheduler.pollWaiting();
		debug("WakeUp");
		debug_task(task);
		if (0 < task && task < 0xFF) {
			runScheduler(task);
		}
	}
}


