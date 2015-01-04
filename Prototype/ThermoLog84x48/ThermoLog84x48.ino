/*

	ThermoLog84x48
	based on Lcd84x48

 - Experimenting with Scheduler vs. MilliTimer.
 - Running key-scan code, interrupts.	
 */

// Definition of interrupt names
#include < avr/io.h >
// ISR interrupt service routine
#include < avr/interrupt.h >

#include <mpelib.h>
#include <DotmpeLib.h>
#include <JeeLib.h>
#include <OneWire.h>
#include <PCD8544.h>
// Adafruit DHT
#include <DHT.h>


/** Globals and sketch configuration  */
#define DEBUG           1 /* Enable trace statements */
#define SERIAL          1 /* Enable serial */
							
#define _MEM            1   // Report free memory 
#define _DHT            1
#define DHT_HIGH        1   // enable for DHT22/AM2302, low for DHT11
#define _LCD            0
#define _LCD84x48       1
							
#define REPORT_EVERY    5   // report every N measurement cycles
#define SMOOTH          5   // smoothing factor used for running averages
#define MEASURE_PERIOD  600  // how often to measure, in tenths of seconds
#define UI_IDLE         2000  // tenths of seconds idle time, ...
#define UI_STDBY        4000  // ms
#define MAXLENLINE      79
#define ATMEGA_CTEMP_ADJ 311
							

static String sketch = "X-ThermoLog84x48";
static String version = "0";
static String node = "templg";

// determined upon handshake 
char node_id[7];

static const byte ledPin = 13;
static const byte backlightPin = 9;
#if _DHT
static const byte DHT_PIN = A3;
#endif

MpeSerial mpeser (57600);

MilliTimer debounce, idle, stdby;

/* Scheduled tasks */
enum { MEASURE, REPORT };
static word schedbuf[REPORT];
Scheduler scheduler (schedbuf, REPORT);
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

volatile bool ui_irq;
static bool ui;

#if _DHT
#if DHT_HIGH
DHT dht (DHT_PIN, DHT22);
#else
DHT dht (DHT_PIN, DHT11);
#endif
#endif //_DHT

#if _LCD84x48
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

#endif //_LCD84x48

/* Report variables */

static byte reportCount;    // count up until next report, i.e. packet send

struct {
#if _DHT
	int rhum    :7;  // 0..100 (avg'd)
#if DHT_HIGH
/* DHT22/AM2302: 20% to 100% @ 2% rhum, -40C to 80C @ ~0.5 temp */
	int temp    :10; // -500..+500 (int value of tenths avg)
#else
/* DHT11: 20% to 80% @ 5% rhum, 0C to 50C @ ~2C temp */
	int temp    :10; // -500..+500 (tenths, .5 resolution)
#endif
#endif
	int ctemp   :10; // atmega temperature: -500..+500 (tenths)
#if _MEM
	int memfree :16;
#endif
} payload;

#if _NRF24
#endif // RF24 funcs

/** Generic routines */

#if defined(ARDUINO) && ARDUINO >= 100
#define printByte(args)  write(args);
#else
#define printByte(args)  print(args,BYTE);
#endif


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

void debug(String msg) {
#if DEBUG
	Serial.println(msg);
#endif
}

#if _LCD
#endif //_LCD

#if _RTC
#endif //_RTC

//ISR(INT0_vect) 
void irq0()
{
	ui_irq = true;
	//Sleepy::watchdogInterrupts(0);
}

/* Button detect using PCINT for ADC port */
void buttons_start()
{
	pinMode(A0, INPUT);	   // Pin A0 is input to which a switch is connected
	digitalWrite(A0, LOW);   // Configure internal pull-up resistor
	pinMode(A1, INPUT);	   // Pin A1 is input to which a switch is connected
	digitalWrite(A1, LOW);   // Configure internal pull-up resistor
	pinMode(A2, INPUT);	   // Pin A2 is input to which a switch is connected
	digitalWrite(A2, LOW);   // Configure internal pull-up resistor

	cli();		// switch interrupts off while messing with their settings  
	PCICR = 0x02;          // Enable PCINT1 interrupt
	PCMSK1 = 0b00000111;
	sei();		// turn interrupts back on
}

ISR(PCINT1_vect) {
	// Interrupt service routine. Every single PCINT8..14 (=ADC0..5) change
	// will generate an interrupt: but this will always be the
	// same interrupt routine
	if (digitalRead(A0) == 1)  Serial.println("A0");
	else if (digitalRead(A1) == 1)  Serial.println("A1");
	else if (digitalRead(A2) == 1)  Serial.println("A2");
}


static void doConfig(void)
{
}


void setupLibs()
{
#if _DHT
	dht.begin();
#endif

	buttons_start();
}

static bool doAnnounce()
{
}

static void doMeasure()
{
	byte firstTime = payload.ctemp == 0; // special case to init running avg

	payload.ctemp = smoothedAverage(payload.ctemp, internalTemp(), firstTime);

#if _DHT
	float h = dht.readHumidity();
	float t = dht.readTemperature();
	if (isnan(h)) {
#if SERIAL && DEBUG
		Serial.println(F("Failed to read DHT11 humidity"));
#endif
	} else {
		payload.rhum = smoothedAverage(payload.rhum, round(h), firstTime);
#if SERIAL && DEBUG_MEASURE
		Serial.print(F("DHT RH new/avg "));
		Serial.print(h);
		Serial.print(' ');
		Serial.println(payload.rhum);
#endif
	}
	if (isnan(t)) {
#if SERIAL && DEBUG
		Serial.println(F("Failed to read DHT11 temperature"));
#endif
	} else {
		payload.temp = smoothedAverage(payload.temp, (int)(t * 10), firstTime);
#if SERIAL && DEBUG_MEASURE
		Serial.print(F("DHT T new/avg "));
		Serial.print(t);
		Serial.print(' ');
		Serial.println(payload.temp);
#endif
	}
#endif // _DHT

#if _MEM
	payload.memfree = freeRam();
#if SERIAL && DEBUG_MEASURE
	Serial.print("MEM free ");
	Serial.println(payload.memfree);
#endif
#endif
}

static void doReport(void)
{
#if SERIAL
	Serial.print(node);
	Serial.print(" ");
#if _DHT
	Serial.print((int) payload.rhum);
	Serial.print(' ');
	Serial.print((int) payload.temp);
	Serial.print(' ');
#endif
	Serial.print((int) payload.ctemp);
	Serial.print(' ');
#if _MEM
	Serial.print((int) payload.memfree);
	Serial.print(' ');
#endif
	Serial.println();
#endif//SERIAL
}

#if _LCD84x48
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

void lcd_printDHT()
{
	lcd.setCursor(12, 2);
	lcd.print((float)payload.temp/10, 1);
	lcd.print("\001C ");
	lcd.setCursor(LCD_WIDTH/1.5, 2);
	lcd.print(payload.rhum, 1);
	lcd.print("%");
}
#endif //_LCD84x48

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
		case MEASURE:
			// reschedule these measurements periodically
			debug("MEASURE");
			blink(ledPin, 1, 15);
			scheduler.timer(MEASURE, MEASURE_PERIOD);
			doMeasure();
			lcd_printDHT();
			// every so often, a report needs to be sent out
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

		default:
			Serial.print("0x");
			Serial.print(task, HEX);
			Serial.println(" ?");
			serialFlush();
			break;

	}
}

static void runCommand()
{
}

static void reset(void)
{
	pinMode(ledPin, OUTPUT);
	pinMode(backlightPin, OUTPUT);
	digitalWrite(backlightPin, LOW);
	ui_irq = false;
	tick = 0;
	reportCount = REPORT_EVERY;     // report right away for easy debugging
	scheduler.timer(MEASURE, 0);    // start the measurement loop going
}

/* Main */

void setup(void)
{
	mpeser.begin();
	mpeser.startAnnounce(sketch, version);
	serialFlush();
	delay(500);

	setupLibs();

	reset();

	attachInterrupt(INT0, irq0, RISING);
	ui_irq = true;
}

void loop(void)
{
	//doMeasure();
	//doReport();
	//delay(15000);
	//return;
	if (ui_irq) {
		start_ui();
	}
	debug_ticks();
	char task = scheduler.poll();
	if (0 < task && task < 0xFF) {
		runScheduler(task);
	} else if (ui) {
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
		}
	} else {
		blink(ledPin, 1, 15);
		debug("Sleep");
		serialFlush();
		char task = scheduler.pollWaiting();
		debug("WakeUp");
		if (0 < task && task < 0xFF) {
			runScheduler(task);
		}
	}
}
