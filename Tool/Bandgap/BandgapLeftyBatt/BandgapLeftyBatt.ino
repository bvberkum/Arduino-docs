// Function created to obtain chip's actual Vcc voltage value, using internal bandgap reference
// This demonstrates ability to read processors Vcc voltage and the ability to maintain A/D calibration with changing Vcc
// For 328 chip only, mod needed for 1280/2560 chip
// Thanks to "Coding Badly" for direct register control for A/D mux
// 1/9/10 "retrolefty"

// also has support for m168
// see http://forum.arduino.cc/index.php/topic,38119.0.html

#include "Arduino.h"
#include <avr/io.h>
#include <avr/delay.h>
#include <avr/delay.h>
#include <avr/sleep.h>


int battVolts;   // made global for wider avaliblity throughout a sketch if needed, example a low voltage alarm, etc
 
int getBandgap(void) 
{
	const long InternalReferenceVoltage = 1096L;  // Adust this value to your specific internal BG voltage x1000
	// REFS1 REFS0          --> 0 1, AVcc internal ref.
	// MUX3 MUX2 MUX1 MUX0  --> 1110 1.1V (VBG)
	ADMUX = (0<<REFS1) | (1<<REFS0) | (0<<ADLAR) | (1<<MUX3) | (1<<MUX2) | (1<<MUX1) | (0<<MUX0);
	// Start a conversion  
	ADCSRA |= _BV( ADSC );
	// Wait for it to complete
	while( ( (ADCSRA & (1<<ADSC)) != 0 ) );
	// Scale the value
	int results = (((InternalReferenceVoltage * 1024L) / ADC) + 5L) / 10L;
	return results;
}

void setup(void)
{
	Serial.begin(57600);
	Serial.print("volts X 100");
	Serial.println( "\r\n\r\n" );
	delay(100);
}

void loop(void)
{
	for (int i=0; i <= 2; i++) battVolts=getBandgap();  //3 readings seem required for stable value?
	Serial.print("Battery Vcc volts =  ");
	Serial.println(battVolts);
	Serial.print("Analog pin 0 voltage reference of 1.096v = ");
	Serial.println(map(analogRead(0), 0, 1023, 0, battVolts));
	Serial.println();    
	delay(1000);
}
