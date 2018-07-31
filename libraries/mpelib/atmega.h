#ifndef mpelib_atmega_h
#define mpelib_atmega_h

// Internal temperature for Atmel mega/tiny

// Calibration: http://www.atmel.com/images/doc8108.pdf

#ifndef TEMP_OFFSET
#define TEMP_OFFSET 0
#endif

#ifndef TEMP_K
//#define TEMP_K 1.0
#define TEMP_K 10
#endif


static double internalTemp(int offset, float k)
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

	delay(10);            // wait for voltages to become stable.

	ADCSRA |= _BV(ADSC);  // Start the ADC

	// Detect end-of-conversion
	while (bit_is_set(ADCSRA,ADSC));

	// Reading register "ADCW" takes care of how to read ADCL and ADCH.
	wADC = ADCW;

	// One or two point calibration should be used to correct the output, see AVR122.
	// With my current setup and atmega328P chips, offsets of -50 or -60 are usual.
	t = ( wADC - 273 + offset ) * k;

	// The returned temperature is in degrees Celcius.
	return (t);
}

#endif
