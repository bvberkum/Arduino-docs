/**
* I2C PIR
* cannot do announce on PIR trigger? perhaps do other serial proto

TODO: add/wire up power jack (5v, no vreg) to box
TODO: wire PIR and left data-jack
TODO: test DS bus, serial autodetect?
TODO: wire up other jacks, figure out some link/through/detect scheme (no radio
	so needs to mate with device that does. A RelayBox probably.)
*/

#include <Wire.h>

#define DHT_PIN     7   // defined if DHTxx data is connected to a DIO pin
#define LDR_PORT    4   // defined if LDR is connected to a port's AIO pin
#define MEMREPORT   1


// TODO state payload config in propery type-bytes
#if LDR_PORT
c += 'l'
#endif
#if DHT_PIN
c += 'h'
#endif
#if MEMREPORT
c += 'm'
#endif


struct {
#if LDR_PORT
	byte light  :8;     // light sensor: 0..255
#endif
	byte moved  :1;  // motion detector: 0..1
#if DHT_PIN
	int rhum    :7;   // rhumdity: 0..100 (4 or 5% resolution?)
	int temp    :10; // temperature: -500..+500 (tenths, .5 resolution)
#endif
	int attemp  :10; // atmega temperature: -500..+500 (tenths)
#if MEMREPORT
	int memfree :16;
#endif
	byte lobat  :1;  // supply voltage dropped under 3.1V: 0..1
} payload;

static void serialFlush () {
#if ARDUINO >= 100
	Serial.flush();
#endif  
	delay(2); // make sure tx buf is empty before going back to sleep
}

double internalTemp(void)
{
	unsigned int wADC;
	double t;

	// The internal temperature has to be used
	// with the internal reference of 1.1V.
	// Channel 8 can not be selected with
	// the analogRead function yet.

	// Set the internal reference and mux.
	ADMUX = (_BV(REFS1) | _BV(REFS0) | _BV(MUX3));
	ADCSRA |= _BV(ADEN);  // enable the ADC

	delay(20);            // wait for voltages to become stable.

	ADCSRA |= _BV(ADSC);  // Start the ADC

	// Detect end-of-conversion
	while (bit_is_set(ADCSRA,ADSC));

	// Reading register "ADCW" takes care of how to read ADCL and ADCH.
	wADC = ADCW;

	// The offset of 324.31 could be wrong. It is just an indication.
	t = (wADC - 311 ) / 1.22;

	// The returned temperature is in degrees Celcius.
	return (t);
}

int freeRam () {
	extern int __heap_start, *__brkval; 
	int v; 
	return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

void setup()
{
#if SERIAL || DEBUG
	Serial.begin(57600);
	Serial.println(F("\n[I2CPIR]"));
#if DEBUG
	Serial.print(F("Free RAM: "));
	Serial.println(freeRam());
#endif
	serialFlush();
#endif
	Wire.begin(9);
	Wire.onRequest(requestEvent);
	Wire.onReceive(receiveEvent);
}

void loop()
{
}

void requestEvent()
{
}

void receiveEvent()
{
	while (1 < Wire.available())
	{
		char c = Wire.read();
	}
	//int x = Wire.read();
}
