/*
TODO: want a simple freq meter with lcd 

- http://interface.khm.de/index.php/lab/experiments/arduino-frequency-counter-library/
  FreqCounter is explained here, as wel as some conditioning tips given.

Kerry Wong has a Arduino "version", although it is a bit hacked and does not
work on Arduino 1.0.3


LCD? 8 pins

6 pins for display, one for backlight and one for contrast

*/
#define SERIAL 1
#define LCD 0
#define DEMUX 0
#define REPORT 1

#define SMOOTH          3   // smoothing factor used for running averages

#if LCD
#include <PCD8544.h>
#endif

#include <FreqCounter.h>
#include <JeeLib.h>
#include <avr/int.h>
#include <avr/sleep.h>

// has to be defined because we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

unsigned long new_frequency;
unsigned long frequency;

int report = 0;

#if LCD
static const byte LCD_WIDTH = 84;
static const byte LCD_HEIGHT = 48;

static PCD8544 lcd(3, 4, 9, 6, 7); /* SCLK, SDIN, DC, RESET, SCE */
#endif

#if DEMUX
//Pin connected to ST_CP of 74HC595
int latchPin = 8;
//Pin connected to SH_CP of 74HC595
int clockPin = 12;
//Pin connected to DS of 74HC595
int dataPin = 11;
// Testing on 3 pins
int maxPins = 3;
int pinNumber = 0;
#endif

#if DEMUX
void setOne(int q)
{
	digitalWrite(latchPin, LOW);
	shiftOut(dataPin, clockPin, MSBFIRST, 1 << q);
	digitalWrite(latchPin, HIGH);
}
#endif

int smoothedAverage(int prev, int next) {
//		return (prev + next) / 2;
	return ((SMOOTH - 1) * prev + next + SMOOTH / 2) / SMOOTH;
}

void setup()
{
#if SERIAL
	Serial.begin( 57600 );
	Serial.println( "FrequencyMeter" );
	delay(2);
#endif

#if DEMUX
    pinMode(latchPin, OUTPUT);
    pinMode(clockPin, OUTPUT);
    pinMode(dataPin, OUTPUT);
#endif

#if LCD
	lcd.begin(LCD_WIDTH, LCD_HEIGHT);
#endif
}

void loop()
{
#if LCD
	lcd.clear();
#endif

#if LCD
	lcd.setCursor(0, 0);
	lcd.print("FrequencyMeter");
#endif

#if DEMUX
	setOne(pinNumber++);
#endif

#if DEMUX
#if LCD
	lcd.setCursor(0, 2);
	lcd.print(pinNumber, 1);
#endif
#if SERIAL
	Serial.print(pinNumber);
	Serial.print(" ");
#endif
#endif

	delay(10);

	FreqCounter::f_comp = 10;
//	FreqCounter::f_comp = 1;
	FreqCounter::start(100);
	while (FreqCounter::f_ready == 0)
		new_frequency = FreqCounter::f_freq;

	frequency = smoothedAverage(frequency, new_frequency);
	
#if LCD
	lcd.setCursor(0, 1);
	lcd.print(frequency, 1);
//	lcd.print(" cycles");
#endif
#if SERIAL
	report++;
	if (report == REPORT) {
		Serial.print(new_frequency, DEC);
		Serial.print(" cycles ");
		Serial.print(frequency, DEC);
		Serial.println(" avg");
		report = 0;
	}
#endif

#if DEMUX
	if (pinNumber == maxPins) {
		pinNumber = 0;
	}
#endif

//delay(500);

//	Sleepy::loseSomeTime(500);
//	byte minutes = 5; 
//	while (minutes-- > 0)
//		Sleepy::loseSomeTime(60000);
}
