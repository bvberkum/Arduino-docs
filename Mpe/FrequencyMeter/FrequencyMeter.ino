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


void setup()
{
	Serial.begin( 57600 );

	lcd.begin(LCD_WIDTH, LCD_HEIGHT);

	lcd.setCursor(0, 0);
	lcd.print("FrequencyMeter");
}

void loop()
{
	Serial.println(".");

	FreqCounter::f_comp = 106;
	FreqCounter::f_comp = 10;
	FreqCounter::start(1000);
	while (FreqCounter::f_ready == 0)
		frequency = FreqCounter::f_freq;
	
	lcd.clear();
	lcd.setCursor(0, 1);
	lcd.print(frequency, 1);
	lcd.print(" cycles");

//	Sleepy::loseSomeTime(500);
//	byte minutes = 5; 
//	while (minutes-- > 0)
//		Sleepy::loseSomeTime(60000);
}
