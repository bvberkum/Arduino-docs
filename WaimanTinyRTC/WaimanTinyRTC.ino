//
// Maurice Ribble
// 4-17-2008
// http://www.glacialwanderer.com/hobbyrobotics

// This code tests the DS1307 Real Time clock on the Arduino board.
// The ds1307 works in binary coded decimal or BCD.  You can look up
// bcd in google if you aren't familior with it.  There can output
// a square wave, but I don't expose that in this code.  See the
// ds1307 for it's full capabilities.

#include "Wire.h"
#define DS1307_I2C_ADDRESS 0x68



void i2c_eeprom_write_byte( int deviceaddress, unsigned int eeaddress, byte data ) {
	int rdata = data;
	Wire.beginTransmission(deviceaddress);
	Wire.write((int)(eeaddress >> 8)); // MSB
	Wire.write((int)(eeaddress & 0xFF)); // LSB
	Wire.write(rdata);
	Wire.endTransmission();
}

// WARNING: address is a page address, 6-bit end will wrap around
// also, data can be maximum of about 30 bytes, because the Wire library has a buffer of 32 bytes
void i2c_eeprom_write_page( int deviceaddress, unsigned int eeaddresspage, byte* data, byte length ) {
	Wire.beginTransmission(deviceaddress);
	Wire.write((int)(eeaddresspage >> 8)); // MSB
	Wire.write((int)(eeaddresspage & 0xFF)); // LSB
	byte c;
	for ( c = 0; c < length; c++)
		Wire.write(data[c]);
	Wire.endTransmission();
}

byte i2c_eeprom_read_byte( int deviceaddress, unsigned int eeaddress ) {
	byte rdata = 0xFF;
	Wire.beginTransmission(deviceaddress);
	Wire.write((int)(eeaddress >> 8)); // MSB
	Wire.write((int)(eeaddress & 0xFF)); // LSB
	Wire.endTransmission();
	Wire.requestFrom(deviceaddress,1);
	if (Wire.available()) rdata = Wire.read();
	return rdata;
}

// maybe let's not read more than 30 or 32 bytes at a time!
void i2c_eeprom_read_buffer( int deviceaddress, unsigned int eeaddress, byte *buffer, int length ) {
	Wire.beginTransmission(deviceaddress);
	Wire.write((int)(eeaddress >> 8)); // MSB
	Wire.write((int)(eeaddress & 0xFF)); // LSB
	Wire.endTransmission();
	Wire.requestFrom(deviceaddress,length);
	int c = 0;
	for ( c = 0; c < length; c++ )
		if (Wire.available()) buffer[c] = Wire.read();
}


// Convert normal decimal numbers to binary coded decimal
byte decToBcd(byte val)
{
	return ( (val/10*16) + (val%10) );
}

// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val)
{
	return ( (val/16*10) + (val%16) );
}

// Stops the DS1307, but it has the side effect of setting seconds to 0
// Probably only want to use this for testing
/*void stopDs1307()
  {
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  Wire.send(0);
  Wire.send(0x80);
  Wire.endTransmission();
  }*/

// 1) Sets the date and time on the ds1307
// 2) Starts the clock
// 3) Sets hour mode to 24 hour clock
// Assumes you're passing in valid numbers
void setDateDs1307(
		byte second,        // 0-59
		byte minute,        // 0-59
		byte hour,          // 1-23
		byte dayOfWeek,     // 1-7
		byte dayOfMonth,    // 1-28/29/30/31
		byte month,         // 1-12
		byte year)          // 0-99
{
	Wire.beginTransmission(DS1307_I2C_ADDRESS);
	Wire.write(0.0);
	Wire.write(decToBcd(second));    // 0 to bit 7 starts the clock
	Wire.write(decToBcd(minute));
	Wire.write(decToBcd(hour));      // If you want 12 hour am/pm you need to set
	// bit 6 (also need to change readDateDs1307)
	Wire.write(decToBcd(dayOfWeek));
	Wire.write(decToBcd(dayOfMonth));
	Wire.write(decToBcd(month));
	Wire.write(decToBcd(year));
	Wire.endTransmission();
}

// Gets the date and time from the ds1307
void getDateDs1307(
		byte *second,
		byte *minute,
		byte *hour,
		byte *dayOfWeek,
		byte *dayOfMonth,
		byte *month,
		byte *year)
{
	// Reset the register pointer
	Wire.beginTransmission(DS1307_I2C_ADDRESS);
	Wire.write(0.0);
	Wire.endTransmission();

	Wire.requestFrom(DS1307_I2C_ADDRESS, 7);

	// A few of these need masks because certain bits are control bits
	*second     = bcdToDec(Wire.read() & 0x7f);
	*minute     = bcdToDec(Wire.read());
	*hour       = bcdToDec(Wire.read() & 0x3f);  // Need to change this if 12 hour am/pm
	*dayOfWeek  = bcdToDec(Wire.read());
	*dayOfMonth = bcdToDec(Wire.read());
	*month      = bcdToDec(Wire.read());
	*year       = bcdToDec(Wire.read());
}

void setup()
{
	byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
	Wire.begin();
	Serial.begin(9600);

	Serial.println("WaimanTinyRTC");

	getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
	if (second == 0 && minute == 0 && hour == 0 && month == 0 && year == 0) {
		Serial.println("Warning: time reset");
		second = 0;
		minute = 31;
		hour = 5;
		dayOfWeek = 4;
		dayOfMonth = 18;
		month = 5;
		year = 12;
		setDateDs1307(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
	}
}

void readEEPROM()
{
	int addr=0; //first address
	byte b = i2c_eeprom_read_byte(0x50, 0); // access the first address from the memory

	while (b!=0) // keep on reading while there is data
	{
		Serial.print((char)b); //print content to serial port
		addr++; //increase address
		b = i2c_eeprom_read_byte(0x50, addr); //access an address from the memory
	}
	Serial.println();
}

void loop()
{
	byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
	getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
	Serial.print(hour, DEC);
	Serial.print(":");
	Serial.print(minute, DEC);
	Serial.print(":");
	Serial.print(second, DEC);
	Serial.print("  ");
	Serial.print(month, DEC);
	Serial.print("/");
	Serial.print(dayOfMonth, DEC);
	Serial.print("/");
	Serial.print(year, DEC);
	Serial.print("  Day_of_week:");
	Serial.println(dayOfWeek, DEC);

	readEEPROM();
	delay(1250);
	Serial.println('.');
	delay(1250);
	Serial.println('.');
	delay(1250);
	Serial.println('.');
	delay(1250);
}
