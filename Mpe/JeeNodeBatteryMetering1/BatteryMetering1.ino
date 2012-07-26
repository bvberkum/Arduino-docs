#include <Ports.h>
#include <RF12.h>
#include <avr/sleep.h>

void setup () {
  	Serial.begin(9600);
  	Serial.println("BatteryMetering1");

	// turn of radio completely
	rf12_initialize(17, RF12_868MHZ);
	rf12_sleep(0);

	// blink led b/c we can
	PORTB |= bit(1);
	DDRB != bit(1);
	for (byte i = 0; i < 6 ; ++i) {
		delay(100);
		PINB |= bit(1);
	}

	// stop responding to interrupts
	cli();

	// disable the ADC, BODS, and everything else we can
	byte prrSave = PRR, adcsraSave = ADCSRA;
	ADCSRA &= ~ bit(ADEN);
	MCUCR |= bit(BODSE) | bit(BODS);
	MCUCR &= ~ bit(BODSE);
	PRR = 0xFF;

	// zzz....
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	sleep_mode();
}

void loop () {
}

