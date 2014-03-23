/**
 * atmega8 with direct-hookup double 7Seg, and DS18B20 temperature display (1C
 * resolution).
 */

#include <OneWire.h>

#define DEBUG   1
#define DEBUG_DS   0

#define SERIAL 1

byte seven_seg_digits[12][7] = { 
/*	  b c d e f a g */
	{ 1,1,1,1,1,1,0 },  // = 0
	{ 0,1,1,0,0,0,0 },  // = 1
	{ 1,1,0,1,1,0,1 },  // = 2
	{ 1,1,1,1,0,0,1 },  // = 3
	{ 0,1,1,0,0,1,1 },  // = 4
	{ 1,0,1,1,0,1,1 },  // = 5
	{ 1,0,1,1,1,1,1 },  // = 6
	{ 1,1,1,0,0,0,0 },  // = 7
	{ 1,1,1,1,1,1,1 },  // = 8
	{ 1,1,1,1,0,1,1 },  // = 9
	{ 1,0,0,1,1,1,1 },  // = E for Error
	{ 0,0,0,0,1,0,1 },  // = r for Error
};
int dp = 9;
int ca1 = 12;
int ca2 = 11;
int _7segRefresh = 6;

/* Dallas OneWire bus with registration for DS18B20 temperature sensors */
OneWire ds(10);

uint8_t dsProbe [8] = { 
	0x28, 0xCC, 0x9A, 0xF4, 0x03, 0x00, 0x00, 0x6D }; // ds18b20 at probe pin

enum { DS_OK, DS_ERR_CRC };

int atmegaTemp;
int probeTemp;

static void serialFlush () {
#if SERIAL
#if ARDUINO >= 100
	Serial.flush();
#endif  
	delay(2); // make sure tx buf is empty before going back to sleep
#endif
}

/* ATmega328 internal temperature */
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

/* 7Seg:: Segment Led numeric display */
void writeDot(byte dot) {
	digitalWrite(dp, dot);
}

void digitOne() {
//	pinMode(ca1, OUTPUT);
	digitalWrite(ca1, HIGH);
//	pinMode(ca2, INPUT);
	digitalWrite(ca2, LOW);
}
void digitTwo() {
//	pinMode(ca1, INPUT);
	digitalWrite(ca1, LOW);
//	pinMode(ca2, OUTPUT);
	digitalWrite(ca2, HIGH);
}

void noDigit() {
	pinMode(ca1, OUTPUT);
	digitalWrite(ca1, HIGH);
	pinMode(ca2, OUTPUT);
	digitalWrite(ca2, HIGH);
}

void sevenSegWrite(byte digit) {
	byte pin = 2;
	for (byte segCount = 0; segCount < 7; ++segCount) {
		// write for common-anode (CA)
		digitalWrite(pin, ! seven_seg_digits[digit][segCount]);
		++pin;
	}
}

void sevenSegClear() {
	byte pin = 2;
	while (pin < 9) {
		digitalWrite(pin, HIGH);
		++pin;
	}
}

/* delays.. perhaps write no-delay version */
void activeDemuxPrintDigits(byte d1, byte d2) {
	for (int x=0; x<150; x++) {
		digitTwo();
		sevenSegWrite(d2);
		delay(_7segRefresh);
		digitOne();
		sevenSegWrite(d1);
		delay(_7segRefresh);
	}
}

void displayDoubleDigit(int count) {
	activeDemuxPrintDigits(count / 10, count % 10);
}

void setup() {
#if SERIAL
	Serial.begin(57600);
	Serial.println("Atmega8TempGuard");
	serialFlush();
#endif
//	DDRD = 0b11111111; // pin 0-7 to output mode
	pinMode(0, OUTPUT); 
	pinMode(2, OUTPUT); // B
	pinMode(3, OUTPUT); // C
	pinMode(4, OUTPUT); // D
	pinMode(5, OUTPUT); // E
	pinMode(6, OUTPUT); // F
	pinMode(7, OUTPUT); // A
	pinMode(8, OUTPUT); // G
	pinMode(dp, OUTPUT); // DP
	pinMode(ca1, OUTPUT); // CA1
	pinMode(ca2, OUTPUT); // CA2
	digitTwo();
	writeDot(1);
	digitOne();
	sevenSegClear();
	sevenSegWrite(3);
}

/**
	The delayed implementation blinks during sensor read.
  */
void loop() {
	sevenSegClear();
	noDigit();
	int temp = readDS18B20(dsProbe);
	if (temp == 32767) {
		activeDemuxPrintDigits(10, 11);
	} else {
		Serial.print(temp/100);
		Serial.print('.');
		Serial.print(temp%100);
		Serial.println('C');
		displayDoubleDigit(temp/100);
	}
//	delay(400);
}
