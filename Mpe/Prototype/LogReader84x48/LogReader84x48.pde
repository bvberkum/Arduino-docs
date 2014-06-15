/* 
SD Card log reader with 84x48 display.

2014-07-15

Current targets:
	- Display menu
	- SD card file log
	- Serial mpe-bus slave version
 */
#include <OneWire.h>
#include <JeeLib.h>
#include <DotmpeLib.h>
// Definition of interrupt names
#include < avr/io.h >
// ISR interrupt service routine
#include < avr/interrupt.h >

#define DEBUG       1 /* Enable trace statements */
#define SERIAL      1 /* Enable serial */

#define SCHEDULER_DELAY 1000
#define UI_DELAY        1
#define UI_STDBY        20  // tenths of seconds idle time, before user-interface 
							// polling shuts down
#define REPORT_EVERY    5   // report every N measurement cycles
#define SMOOTH          5   // smoothing factor used for running averages
#define MEASURE_PERIOD  5  // how often to measure, in tenths of seconds
							
#define _MEM            1


static String sketch = "X-LogReader84x48";
static String node = "logreader-1";
static String version = "0";

MpeSerial mpeser (57600);

enum { USER, USER_POLL, USER_IDLE, MEASURE, REPORT, TASK_END };
static word schedbuf[TASK_END];
Scheduler scheduler (schedbuf, TASK_END);
// has to be defined because we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

static byte reportCount;    // count up until next report, i.e. packet send

extern InputParser::Commands cmdTab[] PROGMEM;
byte* buffer = (byte*) malloc(50);
InputParser parser (buffer, 50, cmdTab);

byte backlight = 9;

struct {
	int ctemp       :10; // atmega temperature: -500..+500 (tenths)
#if _MEM
	int memfree     :16;
#endif
} payload;

int freeRam () {
	extern int __heap_start, *__brkval; 
	int v; 
	return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

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

volatile bool ui_irq;

//ISR(INT0_vect) 
void irq0()
{
	ui_irq = true;
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



/** Generic routines */

static void serialFlush () {
#if SERIAL
#if ARDUINO >= 100
	Serial.flush();
#endif
	delay(2); // make sure tx buf is empty before going back to sleep
#endif
}

void blink(int led, int count, int length) {
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

static void doMeasure()
{
	byte firstTime = payload.ctemp == 0; // special case to init running avg
	payload.ctemp = smoothedAverage(payload.ctemp, internalTemp(), firstTime);
#if _MEM
	payload.memfree = freeRam();
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

/* InputParser handlers */

byte stdby;
static byte STDBY = 0xff;

static void helpCommand () {
	Serial.println("Help!");
	stdby = STDBY;
}

static void valueCmd () {
	int v;
	parser >> v;
	Serial.print("value = ");
	Serial.println(v);
	stdby = STDBY;
}

static void reset(void)
{
	reportCount = REPORT_EVERY;     // report right away for easy debugging
	scheduler.timer(USER, 0);
	scheduler.timer(MEASURE, 0);
	pinMode(backlight, OUTPUT);
	digitalWrite(backlight, HIGH);
	stdby = STDBY;
	ui_irq = false;
}

InputParser::Commands cmdTab[] = {
	{ 'h', 0, helpCommand },
	{ 'v', 2, valueCmd },
	{ 'r', 0, reset },
};

/* Main */

void setup(void)
{
	mpeser.begin(sketch, version);
	serialFlush();
	reset();
	attachInterrupt(INT0, irq0, RISING);
}

void loop(void)
{
	blink(13, 1, 15);
	if (ui_irq) {
		ui_irq = false;
	}
	switch (scheduler.poll()) {
		case USER:
			Serial.println("USER");
			digitalWrite(backlight, HIGH);
			scheduler.cancel(USER_IDLE);
			scheduler.timer(USER_IDLE, UI_STDBY);
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
			scheduler.cancel(USER);
			scheduler.cancel(USER_POLL);
			digitalWrite(backlight, LOW);
			serialFlush();
			break;
		case MEASURE:
			Serial.println("MEASURE");
			scheduler.timer(MEASURE, MEASURE_PERIOD);
			doMeasure();
			if (++reportCount >= REPORT_EVERY) {
				reportCount = 0;
				scheduler.timer(REPORT, 0);
			}
			serialFlush();
			break;
		case REPORT:
			Serial.println("REPORT");
			doReport();
			serialFlush();
			break;
		case TASK_END:
			Serial.print("task? ");
			break;
		case -1: // waiting
		case -2: // nothing
			Serial.print(";");
			serialFlush();
			Sleepy::loseSomeTime(SCHEDULER_DELAY);
			Sleepy::powerDown();
		default:
			Serial.print(".");
			//Serial.print(scheduler.idle(USER));
			//Serial.print(' ');
			//Serial.print(scheduler.idle(USER_IDLE));
			//Serial.print(' ');
			//Serial.print(scheduler.idle(MEASURE));
			//Serial.print(' ');
			//Serial.print(scheduler.idle(REPORT));
			//Serial.print(' ');
			//Serial.println(scheduler.idle(TASK_END));
			serialFlush();
			Sleepy::loseSomeTime(SCHEDULER_DELAY);
			serialFlush();
			//if (scheduler.idle(USER)) {
			//	Sleepy::loseSomeTime(100);
			//}
			if (ui_irq) {
				ui_irq = false;
				scheduler.timer(USER, 0);
				Serial.println("USR IRQ");
				serialFlush();
			}
	}
}
