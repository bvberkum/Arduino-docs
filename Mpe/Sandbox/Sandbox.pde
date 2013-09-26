#include <JeeLib.h>
#include <avr/sleep.h>


#define DEBUG   1   // set to 1 to display each loop() run and PIR trigger
#define SERIAL  1   // set to 1 to enable serial interface

// has to be defined because we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

static void serialFlush () {
#if ARDUINO >= 100
	Serial.flush();
#endif  
	delay(2); // make sure tx buf is empty before going back to sleep
}

void setup()
{
#if SERIAL || DEBUG
	Serial.begin(57600);
	Serial.println("Sandbox");
	serialFlush();
#endif
}

void loop() {

#if DEBUG
	Serial.print('.');
	serialFlush();
#endif

	Sleepy::loseSomeTime(1000);
}

