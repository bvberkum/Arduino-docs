/*
TODO: want a simple freq meter with lcd 

Kerry Wong has a Arduino "version", although it is a bit hacked and does not
work on Arduino 1.0.3

LCD? 8 pins

6 pins for display, one for backlight and one for contrast

*/
#include <PCD8544.h>
#include <FreqCounter.h>
#include <JeeLib.h>
#include <avr/sleep.h>

// has to be defined because we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

static const byte LCD_WIDTH = 84;
static const byte LCD_HEIGHT = 48;

unsigned long frequency;

int cntr = 0;

static PCD8544 lcd(3, 4, 9, 6, 7); /* SCLK, SDIN, DC, RESET, SCE */

//Pin connected to ST_CP of 74HC595
int latchPin = 8;
//Pin connected to SH_CP of 74HC595
int clockPin = 12;
//Pin connected to DS of 74HC595
int dataPin = 11;
// Testing on 3 pins
int maxPins = 3;
int pinNumber = 0;

void setOne(int q)
{
	digitalWrite(latchPin, LOW);
	shiftOut(dataPin, clockPin, MSBFIRST, 1 << q);
	digitalWrite(latchPin, HIGH);
}

void setup()
{
	Serial.begin( 57600 );

    pinMode(latchPin, OUTPUT);
    pinMode(clockPin, OUTPUT);
    pinMode(dataPin, OUTPUT);

	lcd.begin(LCD_WIDTH, LCD_HEIGHT);
}

void loop()
{
	lcd.clear();
	Serial.println(".");

	lcd.setCursor(0, 0);
	lcd.print("FrequencyMeter");

	setOne(pinNumber++);

	lcd.setCursor(0, 2);
	lcd.print(pinNumber, 1);

	delay(100);

//	FreqCounter::f_comp = 106;
	FreqCounter::f_comp = 100;
	FreqCounter::start(100);
	while (FreqCounter::f_ready == 0)
		frequency = FreqCounter::f_freq;
	
	lcd.setCursor(0, 1);
	lcd.print(frequency, 1);
//	lcd.print(" cycles");

	if (pinNumber == maxPins) {
		pinNumber = 0;
	}

	Sleepy::loseSomeTime(200);
//	byte minutes = 5; 
//	while (minutes-- > 0)
//		Sleepy::loseSomeTime(60000);
}
