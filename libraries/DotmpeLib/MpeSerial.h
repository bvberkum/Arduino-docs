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

struct SerMsg {
	byte type;
	byte value;
};

class MpeSerial
{
public:
	MpeSerial(word speed);
	void begin(void);
	void startAnnounce(String sketch, String version);
	void read(void);
	void readHandshake(SerMsg &msg);
	void readHandshakeWaiting(SerMsg &msg, int wait);
	//void setCallback(void (*midiInCallback)(MidiMessage));
private:
protected:
	word _speed;
};


#endif
