/* 
Dallas OneWire Temperature Bus with autodetect and eeprom config 
*/
#include <DotmpeLib.h>
#include <OneWire.h>

/** Globals and sketch configuration  */
#define DEBUG           1 /* Enable trace statements */
#define SERIAL          1 /* Enable serial */
							
#define _MEM            1   // Report free memory 
#define _DS             1
#define DS_PIN          8
#define DEBUG_DS        1
//#define FIND_DS    1
							
#define SMOOTH          5   // smoothing factor used for running averages
							
#define MAXLENLINE      79
							

String sketch = "X-DallasTempBus";
String version = "0";

int tick = 0;
int pos = 0;


MpeSerial mpeser (57600);


#if _DS
/* Dallas OneWire bus with registration for DS18B20 temperature sensors */

OneWire ds(DS_PIN);

uint8_t ds_count = 0;
uint8_t ds_search = 0;

//uint8_t ds_addr[ds_count][8] = {
//	{ 0x28, 0xCC, 0x9A, 0xF4, 0x03, 0x00, 0x00, 0x6D }, // In Atmega8TempGaurd
//	{ 0x28, 0x8A, 0x5B, 0xDD, 0x03, 0x00, 0x00, 0xA6 },
//	{ 0x28, 0x45, 0x94, 0xF4, 0x03, 0x00, 0x00, 0xB3 },
//	{ 0x28, 0x08, 0x76, 0xF4, 0x03, 0x00, 0x00, 0xD5 },
//	{ 0x28, 0x82, 0x27, 0xDD, 0x03, 0x00, 0x00, 0x4B },
//};
enum { DS_OK, DS_ERR_CRC };
#endif // _DS


/* *** AVR routines *** {{{ */


/* }}} *** */

/* *** ATmega routines *** {{{ */



/* }}} *** */

/* *** Generic routines *** {{{ */

static void serialFlush () {
#if SERIAL
#if ARDUINO >= 100
	Serial.flush();
#endif
	delay(2); // make sure tx buf is empty before going back to sleep
#endif
}

void debug_ticks(void)
{
#if SERIAL && DEBUG
	tick++;
	if ((tick % 20) == 0) {
		Serial.print('.');
		pos++;
	}
	if (pos > MAXLENLINE) {
		pos = 0;
		Serial.println();
	}
	serialFlush();
#endif
}


// utility code to perform simple smoothing as a running average
static int smoothedAverage(int prev, int next, byte firstTime =0) {
	if (firstTime)
		return next;
	return ((SMOOTH - 1) * prev + next + SMOOTH / 2) / SMOOTH;
}

void debugline(String msg) {
#if DEBUG
	Serial.println(msg);
#endif
}


/* }}} *** */

/* *** Peripheral hardware routines *** {{{ */

#if _DS
/* Dallas DS18B20 thermometer routines */

static int ds_readdata(uint8_t addr[8], uint8_t data[12]) {
	byte i;
	byte present = 0;

	ds.doReset();
	ds.select(addr);
	ds.write(0x44,1);         // start conversion, with parasite power on at the end

	delay(1000);     // maybe 750ms is enough, maybe not
	// we might do a ds.depower() here, but the doReset will take care of it.

	present = ds.doReset();
	ds.select(addr);
	ds.write(0xBE);         // Read Scratchpad

#if SERIAL && DEBUG_DS
	Serial.print(F("Data="));
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
	Serial.print(F(" CRC="));
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

// FIXME: returns 8500 value at times, drained parasitic power?
static int readDS18B20(uint8_t addr[8]) {
	byte data[12];
	int SignBit;

	int result = ds_readdata(addr, data);	
	
	if (result != DS_OK) {
#if SERIAL
		Serial.println(F("CRC error in ds_readdata"));
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

static uint8_t readDSCount() {
	uint8_t ds_count = EEPROM.read(3);
	if (ds_count == 0xFF) return 0;
	return ds_count;
}

static void updateDSCount(uint8_t new_count) {
	if (new_count != ds_count) {
		EEPROM.write(3, new_count);
		ds_count = new_count;
	}
}

static void writeDSAddr(uint8_t addr[8]) {
	int l = 4 + ( ( ds_search-1 ) * 8 );
	for (int i=0;i<8;i++) {
		EEPROM.write(l+i, addr[i]);
	}
}

static void readDSAddr(int a, uint8_t addr[8]) {
	int l = 4 + ( a * 8 );
	for (int i=0;i<8;i++) {
		addr[i] = EEPROM.read(l+i);
	}
}


static void printDS18B20s(void) {
	byte i;
	byte data[8];
	byte addr[8];
	int SignBit;

	if (!ds.search(addr)) {
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
#endif //_DS



/* }}} *** */

/* *** Initialization routines *** {{{ */

void doConfig(void)
{
}

void initLibs()
{
}


/* }}} *** */

/* *** Run-time handlers *** {{{ */

void doReset(void)
{
	doConfig();
}

bool doAnnounce()
{
}

void doMeasure()
{
}

// periodic report, i.e. send out a packet and optionally report on serial port
void doReport(void)
{
}

void runScheduler(char task)
{
}


/* }}} *** */

/* *** Main *** {{{ */

void setup(void)
{
	mpeser.begin();
	mpeser.startAnnounce(sketch, version);
#if FIND_DS
	printDS18B20s();
#endif
#if DS 
	ds_count = readDSCount();
	for ( int i = 0; i < ds_count; i++) {
		Serial.print(" ds-");
		Serial.print(i+1);
	}
#endif
	serialFlush();

	initLibs();

	doReset();
}

void loop(void)
{
#if _DS
	bool ds_reset = digitalRead(7);
	if (ds_search || ds_reset) {
		if (ds_reset) {
			Serial.println("Reset triggered");
		}
		findDS18B20s();
		return;
	}
#endif

	doMeasure();
	doReport();

	debug_ticks();

	delay(15000);
}

/* }}} *** */

