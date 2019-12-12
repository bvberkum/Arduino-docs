/*
Dallas OneWire Temperature Bus with autodetect and eeprom config
 */



/* *** Globals and sketch configuration *** */
#define SERIAL_EN       1 /* Enable serial */
#define DEBUG           1 /* Enable trace statements */
#define DEBUG_MEASURE   1

#define _MEM            1   // Report free memory
#define _DHT            0
#define DHT_HIGH        1   // enable for DHT22/AM2302, low for DHT11
#define _DS             1
#define DEBUG_DS        0
#define _LCD84x48       0
#define _NRF24          0

#define SMOOTH          5   // smoothing factor used for running averages

#define MAXLENLINE      79


#include <EEPROM.h>
#if defined(__AVR_ATmega328__) || defined(__AVR_ATmega328P__)
#define I2C_SLAVE       4
#include <Wire.h>
#else
#define I2C_SLAVE       0
#endif

#include <OneWire.h>
#include <DotmpeLib.h>
#include <mpelib.h>

void receiveEvent(int howMany);



const String sketch = "X-DallasTempBus";
const int version = 0;


/* IO pins */
//              RXD      0
//              TXD      1
//              INT0     2
#if _DS
static const byte DS_PIN = 8;
#endif

MpeSerial mpeser (57600);

/* *** Report variables *** {{{ */


static byte reportCount;    // count up until next report, i.e. packet send

struct {
} payload;


/* *** /Report variables *** }}} */

/* *** Scheduled tasks *** {{{ */


/* *** /Scheduled tasks *** }}} */

/* *** EEPROM config *** {{{ */



/* *** /EEPROM config *** }}} */

/* *** Peripheral devices *** {{{ */

#if LDR_PORT
Port ldr (LDR_PORT);
#endif

#if _DHT
#endif // DHT

#if _LCD84x48
/* Nokkia 5110 display */
#endif //_LCD84x48

#if _DS
/* Dallas OneWire bus with registration for DS18B20 temperature sensors */

OneWire ds(DS_PIN);

volatile int ds_value[0];

enum { DS_OK, DS_ERR_CRC };

#endif // DS

#if _RFM12B
/* HopeRF RFM12B 868Mhz digital radio */


#endif //_RFM12B

#if _NRF24
/* nRF24L01+: nordic 2.4Ghz digital radio  */

#endif // NRF24

#if _RTC
/* DS1307: Real Time Clock over I2C */
#endif //_RTC

#if _HMC5883L
/* Digital magnetometer I2C module */


#endif // HMC5883L


/* *** /Peripheral devices *** }}} */

/* *** Peripheral hardware routines *** {{{ */


#if LDR_PORT
#endif

/* *** PIR support *** {{{ */
#if PIR_PORT
#endif //PIR_PORT
/* *** /PIR support *** }}} */

#if _DHT
/* DHT temp/rh sensor routines (AdafruitDHT) */

#endif // DHT

#if _LCD
#endif //_LCD

#if _LCD84x48


#endif // LCD84x48

#if _DS
/* Dallas DS18B20 thermometer routines */

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

#if SERIAL_EN && DEBUG_DS
	Serial.print(F("Data="));
	Serial.print(present, HEX);
	Serial.print(" ");
#endif
	for ( i = 0; i < 9; i++) {           // we need 9 bytes
		data[i] = ds.read();
#if SERIAL_EN && DEBUG_DS
		Serial.print(i);
		Serial.print(':');
		Serial.print(data[i], HEX);
		Serial.print(" ");
#endif
	}

	uint8_t crc8 = OneWire::crc8( data, 8);

#if SERIAL_EN && DEBUG_DS
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

#if SERIAL_EN && ( DEBUG || DEBUG_DS )
	Serial.print("Reading Address=");
	for (int i=0;i<8;i++) {
		Serial.print(i);
		Serial.print(':');
		Serial.print(addr[i], HEX);
		Serial.print(' ');
	}
	Serial.println();
#endif

	int result = ds_readdata(addr, data);

	if (result != DS_OK) {
#if SERIAL_EN
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

static int readDSCount() {
	int ds_count = EEPROM.read(DS_EEPROM_OFFSET);
	if (ds_count == 0xFF) return 0;
	return ds_count;
}

static void updateDSCount(int new_count) {
	int ds_count = EEPROM.read(DS_EEPROM_OFFSET);
	if (new_count != ds_count) {
		EEPROM.write(DS_EEPROM_OFFSET, new_count);
	}
}

static void writeDSAddr(int a, byte addr[8]) {
	int l = DS_EEPROM_OFFSET + 1 + ( a * 8 );
	for (int i=0;i<8;i++) {
		EEPROM.write(l+i, addr[i]);
	}
}

static void readDSAddr(int a, byte addr[8]) {
	int l = DS_EEPROM_OFFSET + 1 + ( a * 8 );
	for (int i=0;i<8;i++) {
		addr[i] = EEPROM.read(l+i);
	}
}

static void printDSAddrs(int ds_count) {
#if SERIAL_EN && DEBUG_DS
	Serial.print("Reading ");
	Serial.print(ds_count);
	Serial.println(" DS18B20 sensors");
	for (int q=0; q<ds_count;q++) {
		Serial.print("Mem Address=");
		int l = DS_EEPROM_OFFSET + 1 + ( q * 8 );
		int r[8];
		for (int i=0;i<8;i++) {
			r[i] = EEPROM.read(l+i);
			Serial.print(i);
			Serial.print(':');
			Serial.print(r[i], HEX);
			Serial.print(' ');
		}
		Serial.println();
	}
#endif
}

static int findDS18B20s(int &ds_search) {
	byte i;
	byte addr[8];

	if (!ds.search(addr)) {
		ds.reset_search();
#if SERIAL_EN && DEBUG_DS
		Serial.println("No more addresses.");
		Serial.print("Found ");
		Serial.print(ds_search);
		Serial.println(" addresses.");
		Serial.print("Previous search found ");
		Serial.print(readDSCount());
		Serial.println(" addresses.");
#endif
		return 0;
	}

	if ( OneWire::crc8( addr, 7 ) != addr[7]) {
#if SERIAL_EN
		Serial.println("CRC is not valid!");
#endif
		return 2;
	}

#if SERIAL_EN && ( DEBUG || DEBUG_DS )
	Serial.print("New Address=");
	for( i = 0; i < 8; i++) {
		Serial.print(i);
		Serial.print(':');
		Serial.print(addr[i], HEX);
		Serial.print(" ");
	}
	Serial.println();
#endif

	ds_search++;
	Serial.println(ds_search);

	writeDSAddr(( ds_search-1 ), addr);

	return 1;
}


#endif //_DS

#if _RFM12B
/* HopeRF RFM12B 868Mhz digital radio routines */


#endif //_RFM12B

#if _NRF24
/* Nordic nRF24L01+ radio routines */

void radio_init()
{
	SPI.begin();
	radio.begin();
	config.rf24id = 3;
	network.begin( NRF24_CHANNEL, config.rf24id );
}

void rf24_start()
{
#if DEBUG
	radio.printDetails();
#endif
}


#endif // NRF24 funcs

#if _RTC
#endif //_RTC

#if _HMC5883L
/* Digital magnetometer I2C routines */


#endif // HMC5883L


/* *** /Peripheral hardware routines *** }}} */

/* *** UI *** {{{ */

/* *** /UI *** }}} */

/* *** UART commands *** {{{ */


/* *** UART commands *** }}} */

/* *** Initialization routines *** {{{ */

void doConfig(void)
{
}

void initLibs()
{
#if I2C_SLAVE
	Wire.begin(I2C_SLAVE);
	Wire.onReceive(receiveEvent);
#endif
}


/* *** /Initialization routines *** }}} */

/* *** Run-time handlers *** {{{ */

void doReset(void)
{
	doConfig();

#if _DS
	int ds_search = 0;
	while (findDS18B20s(ds_search) == 1) ;
	updateDSCount(ds_search);
#endif

	tick = 0;
}

bool doAnnounce(void)
{
}


// readout all the sensors and other values
void doMeasure(void)
{
}


// periodic report, i.e. send out a packet and optionally report on serial port
void doReport(void)
{
}


void runScheduler(char task)
{
}


/* *** /Run-time handlers *** }}} */

#if I2C_SLAVE
void receiveEvent(int howMany)
{
	  while(1 < Wire.available()) // loop through all but the last
	  	    {
	  	    	    char c = Wire.read(); // receive byte as a character
	  	    	        Serial.print(c);         // print the character
	  	    	          }
	    int x = Wire.read();    // receive byte as an integer
	      Serial.println(x);         // print the integer
}
#endif

/* *** Main *** {{{ */


void setup(void)
{
#if SERIAL_EN
	mpeser.begin();
	mpeser.startAnnounce(sketch, String(version));
#if DEBUG || _MEM
	Serial.print(F("Free RAM: "));
	Serial.println(freeRam());
#endif
	serialFlush();
#endif

	initLibs();

	doReset();
}

void loop(void)
{
#ifdef _DBG_LED
	blink(_DBG_LED, 1, 15);
#endif

#if _NRF24
	// Pump the network regularly
	network.update();
#endif

#if _DS
	bool ds_reset = digitalRead(7);
	if (ds_reset) {
		if (ds_reset) {
			Serial.println("Reset triggered");
		}
		int ds_search = 0;
		while (findDS18B20s(ds_search) == 1) ;
		updateDSCount(ds_search);
		return;
	}
#endif

	doMeasure();
	doReport();

	debug_ticks();

	delay(15);
	//delay(15000);
}

/* *** }}} */

