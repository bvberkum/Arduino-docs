/* Serial bus proto? */

#ifndef MpeSerial_h
#define MpeSerial_h

/// @file
/// MpeSerial library definitions.

#if ARDUINO >= 100
#include <Arduino.h> // Arduino 1.0
#else
#include <WProgram.h> // Arduino 0022
#endif

#if ARDUINO >= 100
#define WRITE_RESULT size_t
#else
#define WRITE_RESULT void
#endif

class MpeSerial
{
public:
	MpeSerial(word speed);
	void begin(String sketch, String version);
	void read(void);
	//void setCallback(void (*midiInCallback)(MidiMessage));
private:
protected:
	word _speed;
};


#endif
