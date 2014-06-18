
/*
 */

// Definition of interrupt names
#include < avr/io.h >
// ISR interrupt service routine
#include < avr/interrupt.h >

#include <DHT.h> // Adafruit DHT
#include <DotmpeLib.h>
#include <JeeLib.h>
#include <OneWire.h>
#include <PCD8544.h>


/** Globals and sketch configuration  */
#define DEBUG           1 /* Enable trace statements */
#define SERIAL          1 /* Enable serial */
							
#define REPORT_EVERY    5   // report every N measurement cycles
#define MEASURE_PERIOD  100  // how often to measure, in tenths of seconds
							
#define _MEM            1
#define _DHT            1


static String sketch = "X-ThermoLog84x48";
static String node = "thermolog-1";
static String version = "0";

static const byte backlightPin = 9;
static const byte sensorPin = A3;
static const byte ledPin = 13;


MpeSerial mpeser (57600);

/* Scheduled tasks */
enum { USER, USER_POLL, USER_IDLE, USER_STDBY, MEASURE, REPORT, TASK_END };
static word schedbuf[TASK_END];
Scheduler scheduler (schedbuf, TASK_END);
// has to be defined because we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

#if _DHT
static DHT dht(sensorPin, DHT11);
#endif

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
#if _DHT
	int rhum        :7;  // rhumdity: 0..100 (4 or 5% resolution?)
	int temp        :10; // temperature: -500..+500 (tenths, .5 resolution)
#endif
	int ctemp       :10; // atmega temperature: -500..+500 (tenths)
#if _MEM
	int memfree     :16;
#endif
} payload;

/** Generic routines */

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

static void doMeasure()
{
	byte firstTime = payload.ctemp == 0; // special case to init running avg
	payload.ctemp = internalTemp();
#if _DHT
	float h = dht.readHumidity();
	float t = dht.readTemperature();
	if (isnan(h)) {
#if SERIAL | DEBUG
		Serial.println(F("Failed to read DHT11 humidity"));
#endif
	} else {
		payload.rhum = (int) h;
#if SERIAL && DEBUG_MEASURE
		Serial.print(F("DHT RH new/avg "));
		Serial.print(rh);
		Serial.print(' ');
		Serial.println(payload.rhum);
#endif
	}
	if (isnan(t)) {
#if SERIAL | DEBUG
		Serial.println(F("Failed to read DHT11 temperature"));
#endif
	} else {
		payload.temp = (int) t*10;
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
#endif
	serialFlush();
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
	serialFlush();
#endif//SERIAL
}


volatile bool ui_irq;

//ISR(INT0_vect) 
void irq0()
{
	ui_irq = true;
	Sleepy::watchdogInterrupts(0);
}


/* Main */

void setup(void)
{
	mpeser.begin(sketch, version);
	serialFlush();
	attachInterrupt(INT0, irq0, RISING);
	scheduler.timer(MEASURE, 0);

	lcd.begin(LCD_WIDTH, LCD_HEIGHT);

	// Register the custom symbol...
	lcd.createChar(DEGREES_CHAR, degrees_glyph);

	lcd.send( LOW, 0x21 );  // LCD Extended Commands on
	lcd.send( LOW, 0x07 );  // Set Temp coefficent. //0x04--0x07
//    lcd.send( LOW, 0x14 );  // LCD bias mode 1:48. //0x13
//    lcd.send( LOW, 0x0C );  // LCD in normal mode.
	lcd.send( LOW, 0x20 );  // LCD Extended Commands toggle off

	pinMode(ledPin, OUTPUT);
	pinMode(sensorPin, INPUT);

	dht.begin();

	pinMode(backlightPin, OUTPUT);
	analogWrite(backlightPin, 0xaf);
}

void loop(void)
{
	switch (scheduler.pollWaiting()) {
		case MEASURE:
			blink(ledPin, 1, 15);
			scheduler.timer(MEASURE, MEASURE_PERIOD);
			doMeasure();
			lcd.setCursor(12, 0);
			lcd.print((float)payload.temp/10, 1);
			lcd.print("\001C ");
			lcd.setCursor(LCD_WIDTH/1.5, 0);
			lcd.print(payload.rhum, 1);
			lcd.print("%");
			//if (++reportCount >= REPORT_EVERY) {
			//	reportCount = 0;
			//	scheduler.timer(REPORT, 0);
			//}
			serialFlush();
			break;
		case REPORT:
			break;
		default:
			//Sleepy::loseSomeTime(MEASURE_PERIOD);
			break;
	}
}
