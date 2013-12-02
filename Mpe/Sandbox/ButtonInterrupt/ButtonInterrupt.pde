#include <JeeLib.h>
#include <avr/sleep.h>


#define DEBUG   1   // set to 1 to display each loop() run and PIR trigger
#define SERIAL  1   // set to 1 to enable serial interface

// has to be defined because we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

bool set = false;
ISR(PCINT2_vect) { set = true; }


static void serialFlush () {
#if ARDUINO >= 100
	Serial.flush();
#endif  
	delay(2); // make sure tx buf is empty before going back to sleep
}

void blink(int led, int count, int length) {
  for (int i=0;i<count;i++) {
    digitalWrite (led, HIGH);
    delay(length);
    digitalWrite (led, LOW);
    delay(length);
  }
}

void setup()
{
#if SERIAL || DEBUG
	Serial.begin(57600);
	Serial.println("Sandbox");
	serialFlush();
#endif

	pinMode(4, OUTPUT);
	pinMode(5, OUTPUT);
	pinMode(7, OUTPUT);
	digitalWrite(4, LOW);

	// PCMSK2 = PCIE2 = PCINT16-23 = D0-D7
	bitSet(PCMSK2,  PCINT19); // DIO1

	// interrupt sense control
	//MCUCR = (1<<ISC00) | (1<<ISC01);
	MCUCR = (0<<ISC00) | (0<<ISC01);

	bitSet(PCICR, PCIE2); // enable PCMSK2 for PCINT at DIO1-4 (D4-7)
}

void loop() {

#if DEBUG
	Serial.print('.');
	serialFlush();
#endif

	if (set) {
		blink(5, 3, 50);
		blink(7, 3, 50);
		set = false;
	}

	Sleepy::loseSomeTime(1000);
}


