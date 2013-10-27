#include <JeeLib.h>
#include <avr/sleep.h>


#define DEBUG   0   // set to 1 to display each loop() run and PIR trigger
#define SERIAL  1   // set to 1 to enable serial interface

// has to be defined because we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }


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

int f1 = A0;

void setup()
{
#if SERIAL || DEBUG
	Serial.begin(57600);
	Serial.println("Sandbox");
	serialFlush();
#endif

	pinMode(f1, INPUT);
}

void loop() {

#if DEBUG
	Serial.print('.');
	serialFlush();
#endif

	int v1 = analogRead(f1);
	Serial.println(v1);
	serialFlush();

	Sleepy::loseSomeTime(100);
}

