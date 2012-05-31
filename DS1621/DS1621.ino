#include <Wire.h>

// Simple DS1621 demo
// -- by Jon McPhalen (www.jonmcphalen.com)
// -- 19 DEC 2007

// SDA pin is Analog4
// SCL pin is Analog5
// DS1621 has A2, A1, and A0 pins connected to GND


#define DEV_ID 0x91 >> 1                    // shift required by wire.h


void setup()
{
	Serial.begin(9600);
	Serial.println("DS1621");

	Wire.begin();
	Wire.beginTransmission(DEV_ID);           // connect to DS1621 (#0)
	Wire.write(0xAC);                          // Access Config
	Wire.write(0x02);                          // set for continuous conversion
	Wire.beginTransmission(DEV_ID);           // restart
	Wire.write(0xEE);                          // start conversions
	Wire.endTransmission();
}


void loop()
{
	int tempC = 0;
	int tempF = 0;

	delay(5000);                              // give time for measurement
	Wire.beginTransmission(DEV_ID);
	Wire.write(0xAA);                          // read temperature
	Wire.endTransmission();
	Wire.requestFrom(DEV_ID, 1);              // request one byte from DS1621
	tempC = Wire.read();                   // get whole degrees reading
	tempF = tempC * 9 / 5 + 32;               // convert to Fahrenheit

	Serial.print(tempC);
	Serial.print(" / ");
	Serial.println(tempF);
}
