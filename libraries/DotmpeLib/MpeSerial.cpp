#include "MpeSerial.h"


MpeSerial::MpeSerial(word speed)
{
	_speed = speed;
}

void MpeSerial::begin(void)
{
	Serial.begin(_speed);
	Serial.println();
}

void MpeSerial::startAnnounce(
		String sketch, String version)
{
	Serial.print("\n[");
	Serial.print(sketch);
	Serial.print(".");
	Serial.print(version);
	Serial.print("] ");
}

void MpeSerial::readHandshake(SerMsg &msg)
{
	msg.type = Serial.read();
	msg.value = Serial.read();
}

void MpeSerial::readHandshakeWaiting(SerMsg &msg, int wait)
{
	while (wait && Serial.available() <= 0)
	{
		delay(10);
		wait--;
	}
	MpeSerial::readHandshake(msg);
}

void MpeSerial::read()
{
//    ...Serial.read() stuff...
//          _midiInCallback(message);
}
