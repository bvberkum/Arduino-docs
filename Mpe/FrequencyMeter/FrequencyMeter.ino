/*
TODO: want a simple freq meter with lcd 

Kerry Wong has a Arduino "version", although it is a bit hacked and does not
work on Arduino 1.0.3

LCD? 8 pins

6 pins for display, one for backlight and one for contrast

*/
#include <PCD8544.h>
//#include <FreqCounter.h>

#include <JeeLib.h>
#include <avr/sleep.h>

// has to be defined because we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

static const byte LCD_WIDTH = 84;
static const byte LCD_HEIGHT = 48;

static PCD8544 lcd(3, 4, 5, 6, 7); /* SCLK, SDIN, DC, RESET, SCE */
unsigned long frq;


void setup()
{
	Serial.begin( 57600 );

	lcd.begin(LCD_WIDTH, LCD_HEIGHT);
//	for (int i = 0; i < 3; i++) {
//		pinMode(BTN_PINS[i], INPUT);
//		digitalWrite(BTN_PINS[i], HIGH);
//	}
	//lcd.setCursor(0, 0);
	//lcd.print("Frequency meter")
	//lcd.setCursor(0, 1);
	//lcd.print(".mpe")
	delay(20);
}

int cntr = 0;

void loop()
{
	Serial.println(".");

//	FreqCounter::f_comp = 106;
//	FreqCounter::f_comp = 10;
//	FreqCounter::start(1000);
//	while (FreqCounter::f_ready == 0)
//		frq = FreqCounter::f_freq;

	lcd.setCursor(0, 0);
	lcd.print(frq, 1);

	lcd.setCursor(0, 4);
	lcd.print(cntr, 1);

	cntr += 1;

	Sleepy::loseSomeTime(2500);
//	byte minutes = 5; 
//	while (minutes-- > 0)
//		Sleepy::loseSomeTime(60000);
}
