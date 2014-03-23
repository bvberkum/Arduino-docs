/**/
#include <OneWire.h>

#define DEBUG   1
#define DEBUG_DS   1
#define FIND_DS    1

#define SERIAL 1

/* Dallas OneWire bus with registration for DS18B20 temperature sensors */
OneWire ds(10);

// TODO; autoconf and make usable for CarrierCase, RoomNode etc.
//const int ds_count = 1;
//uint8_t ds_addr[ds_count][8] = {
//	{ 0x28, 0xCC, 0x9A, 0xF4, 0x03, 0x00, 0x00, 0x6D }, // In Atmega8TempGaurd
//	{ 0x28, 0x8A, 0x5B, 0xDD, 0x03, 0x00, 0x00, 0xA6 },
//	{ 0x28, 0x45, 0x94, 0xF4, 0x03, 0x00, 0x00, 0xB3 },
//	{ 0x28, 0x08, 0x76, 0xF4, 0x03, 0x00, 0x00, 0xD5 },
//	{ 0x28, 0x82, 0x27, 0xDD, 0x03, 0x00, 0x00, 0x4B },
//};
#if DEBUG_DS
//volatile int ds_value[ds_count];
#endif

enum { DS_OK, DS_ERR_CRC };

static void serialFlush () {
#if SERIAL || DEBUG
#if ARDUINO >= 100
	Serial.flush();
#endif  
	delay(2); // make sure tx buf is empty before going back to sleep
#endif
}

/* Dallas DS18B20 thermometer */
static int ds_readdata(uint8_t addr[8], uint8_t data[12]) {
	byte i;
	byte present = 0;

	ds.reset();
	ds.select(addr);
	ds.write(0x44,1);         // start conversion, with parasite power on at the end

	delay(1000);     // maybe 750ms is enough, maybe not
	// we might do a ds.depower() here, but the reset will take care of it.

	present = ds.reset();
	ds.select(addr);    
	ds.write(0xBE);         // Read Scratchpad

#if SERIAL && DEBUG_DS
	Serial.print("Data=");
	Serial.print(present,HEX);
	Serial.print(" ");
#endif
	for ( i = 0; i < 9; i++) {           // we need 9 bytes
		data[i] = ds.read();
#if SERIAL && DEBUG_DS
		Serial.print(i);
		Serial.print(':');
		Serial.print(data[i], HEX);
		Serial.print(" ");
#endif
	}

	uint8_t crc8 = OneWire::crc8( data, 8);

#if SERIAL && DEBUG_DS
	Serial.print(" CRC=");
	Serial.print( crc8, HEX);
	Serial.println();
	serialFlush();
#endif

	if (crc8 != data[8]) {
		return DS_ERR_CRC; 
	} else { 
		return DS_OK; 
	}
}

static int ds_conv_temp_c(uint8_t data[8], int SignBit) {
	int HighByte, LowByte, TReading, Tc_100;
	LowByte = data[0];
	HighByte = data[1];
	TReading = (HighByte << 8) + LowByte;
	SignBit = TReading & 0x8000;  // test most sig bit
	if (SignBit) // negative
	{
		TReading = (TReading ^ 0xffff) + 1; // 2's comp
	}
	Tc_100 = (6 * TReading) + TReading / 4;    // multiply by (100 * 0.0625) or 6.25
	return Tc_100;
}

static int readDS18B20(uint8_t addr[8]) {
	byte data[12];
	int SignBit;

	int result = ds_readdata(addr, data);	
	
	if (result != 0) {
#if SERIAL
		Serial.println("CRC error in ds_readdata");
		serialFlush();
#endif
		return 32767;
	}

	int Tc_100 = ds_conv_temp_c(data, SignBit);

	if (SignBit) {
		return 0 - Tc_100;
	} else {
		return Tc_100;
	}
}

static void printDS18B20s(void) {
	byte i;
	byte data[8];
	byte addr[8];
	int SignBit;

	if ( !ds.search(addr)) {
#if SERIAL && DEBUG_DS
		Serial.println("No more addresses.");
#endif
		ds.reset_search();
		return;
	}

#if SERIAL && DEBUG_DS
	Serial.print("Address=");
	for( i = 0; i < 8; i++) {
		Serial.print(i);
		Serial.print(':');
		Serial.print(addr[i], HEX);
		Serial.print(" ");
	}
#endif

	if ( OneWire::crc8( addr, 7) != addr[7]) {
#if SERIAL && DEBUG_DS
		Serial.println("CRC for address is not valid!");
#endif
		return;
	}

#if SERIAL && DEBUG_DS
	if ( addr[0] == 0x10) {
		Serial.println("Device is a DS18S20 family device.");
	}
	else if ( addr[0] == 0x28) {
		Serial.println("Device is a DS18B20 family device.");
	}
	else {
		Serial.print("Device family is not recognized: 0x");
		Serial.println(addr[0],HEX);
		return;
	}
#endif

	int result = ds_readdata(addr, data);	
	
	if (result != 0) {
#if SERIAL && DEBUG_DS
		Serial.println("CRC error in ds_readdata");
#endif
		return;
	}

	int Tc_100 = ds_conv_temp_c(data, SignBit);

	int Whole, Fract;
	Whole = Tc_100 / 100;  // separate off the whole and fractional portions
	Fract = Tc_100 % 100;

	// XXX: add value to payload
#if SERIAL && DEBUG_DS
	if (SignBit) // If its negative
	{
		Serial.print("-");
	}
	Serial.print(Whole);
	Serial.print(".");
	if (Fract < 10)
	{
		Serial.print("0");
	}
	Serial.print(Fract);

	Serial.println("");

	serialFlush();
#endif
}

void setup()
{
#if SERIAL
	Serial.begin(57600);
	Serial.println("DallasTempBus");
	serialFlush();
#endif
#if FIND_DS
	printDS18B20s();
#endif
}

void loop() {
}
