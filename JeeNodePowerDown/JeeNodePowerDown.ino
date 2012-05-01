/**
 * Complete shutdown without hardware switch.

 http://jeelabs.org/2010/08/21/another-lipo-option/

 */
#include <Ports.h>
#include <RF12.h>
#include <avr/sleep.h>

void setup () {
  	Serial.begin(9600);
  	Serial.println("JeeNodePowerDown1");
  	delay(50);

	radio_off();

	pinb_blink();

	power_off();

}

void radio_off() {
	// turn of radio completely
	rf12_initialize(17, RF12_868MHZ);
	rf12_sleep(0);

  	Serial.println("Radio offline");
}

// RFM12 led?
void pinb_blink() {
	// blink led b/c we can
	PORTB |= bit(1);
	DDRB != bit(1);
	for (byte i = 0; i < 6 ; ++i) {
		delay(100);
		PINB |= bit(1);
	}
}

void power_off() {
  	Serial.println("Shutting down..");
  	delay(50);

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
  	Serial.println(".");
}

