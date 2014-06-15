#include "MpeSerial.h"


MpeSerial::MpeSerial(word speed)
{
	_speed = speed;
}

void MpeSerial::begin(String sketch, String version)
{
	Serial.begin(_speed);
	Serial.println();
	Serial.print("[");
	Serial.print(sketch);
	Serial.print(".");
	Serial.print(version);
	Serial.print("]");
}

void MpeSerial::read()
{
//    ...Serial.read() stuff...
//          _midiInCallback(message);
}
