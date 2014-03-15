/**

*/
#include <PCD8544.h>
#include <FreqCounter.h>
#include <JeeLib.h>
#include <avr/sleep.h>

// has to be defined because we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

static const byte LCD_WIDTH = 84;
static const byte LCD_HEIGHT = 48;

#if LCD_94x48
static PCD8544 lcd(3, 4, 9, 6, 7); /* SCLK, SDIN, DC, RESET, SCE */
#endif

//Pin connected to ST_CP of 74HC595
int latchPin = 5;
//Pin connected to SH_CP of 74HC595
int clockPin = 12;
//Pin connected to DS of 74HC595
int dataPin = A1;
// Testing on 5 pins (MSB, so starting at 
int maxPins = 5;

void setOne(int q)
{
	digitalWrite(latchPin, LOW);
	shiftOut(dataPin, clockPin, MSBFIRST, 1 << q);
	digitalWrite(latchPin, HIGH);
}

void setup() 
{
    Serial.begin( 57600 );
#if LCD_94x48
    lcd.begin(LCD_WIDTH, LCD_HEIGHT);
    lcd.setCursor(0, 0);
    lcd.print("OutputExpander");
#endif
    //set pins to output so you can control the shift register
    pinMode(latchPin, OUTPUT);
    pinMode(clockPin, OUTPUT);
    pinMode(dataPin, OUTPUT);
}

void loop() {
	for (int pinNumber = 0; pinNumber < maxPins; pinNumber++) {
		setOne(pinNumber);
#if LCD_94x48
		lcd.setCursor(0, 1);
		lcd.print(pinNumber);
#endif
		Serial.println(pinNumber);
		//Sleepy::loseSomeTime(300);
		delay(200);
	}
}

