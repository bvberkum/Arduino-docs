#include <Ports.h>
#include <RF12.h>
#include <avr/sleep.h>


char* p(char *fmt, ... ){
	char tmp[128]; // resulting string limited to 128 chars
	va_list args;
	va_start (args, fmt );
	vsnprintf(tmp, 128, fmt, args);
	va_end (args);
	return tmp;
}


int getBandgap(void) // Returns actual value of Vcc (x 100)
{
	// For 168/328 boards
	const long InternalReferenceVoltage = 1100L; // Adjust this value to your boards specific internal BG voltage x1000
	// REFS1 REFS0 --> 0 1, AVcc internal ref. -Selects AVcc external reference
	// MUX3 MUX2 MUX1 MUX0 --> 1110 1.1V (VBG) -Selects channel 14, bandgap voltage, to measure
	ADMUX = (0<<REFS1) | (1<<REFS0) | (0<<ADLAR) | (1<<MUX3) | (1<<MUX2) | (1<<MUX1) | (0<<MUX0);
	delay(50); // Let mux settle a little to get a more stable A/D conversion
	// Start a conversion
	ADCSRA |= _BV( ADSC );
	// Wait for it to complete
	while( ( (ADCSRA & (1<<ADSC)) != 0 ) );
	// Scale the value
	int results = (((InternalReferenceVoltage * 1023L) / ADC) + 5L) / 10L; // calculates for straight line value
	return results;
}


void setup () {
  	Serial.begin(9600);
  	Serial.println("JeeNodeTest1");
  	delay(50);
  	analogReference(INTERNAL); // pin 21 (AREF) read 1.10v by DMM
}

void loop () {
	Serial.println(p("BG: %4i  D: %4i %4i %4i %4i %4i %4i    A: %4i %4i %4i %4i %4i %4i", 
		getBandgap(),
		digitalRead(0),
		digitalRead(1),
		digitalRead(2),
		digitalRead(3),
		digitalRead(4),
		digitalRead(5),
		analogRead(0),
		analogRead(1),
		analogRead(2),
		analogRead(3),
		analogRead(4),
		analogRead(5)));
  	delay(500);
}
