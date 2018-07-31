/*
  Lots to do

  - must hook up PWM power

  - PN2907A: EBC (TO-92)


  For now, working on RF24 using LDR and SHT11, see RoomNodeRF24
*/
#define DEBUG_DHT 1
#define _EEPROMEX_DEBUG 1  // Enables logging of maximum of writes and out-of-memory

#include <JeeLib.h>
#include <avr/sleep.h>
//#include <CS_MQ7.h>
#include <OneWire.h>
#include <EEPROM.h>
// include EEPROMEx.h
#include <mpelib.h>

/** Globals and sketch configuration  */
#define DEBUG           1 /* Enable trace statements */
#define SERIAL_EN          1 /* Enable serial */

#define _MEM            1   // Report free memory
#define DHT_PIN     7   // defined if DHTxx data is connected to a DIO pin
//#define LDR_PORT    4   // defined if LDR is connected to a port's AIO pin
#define _DS  0
#define DEBUG_DS        0
#define ATMEGA_CTEMP_ADJ 331

//#define MQ4
#define LED 11
#define _RFM12LOBAT 0
#define _VCC 1

#define MEASURE_PERIOD  600 // how often to measure, in tenths of seconds
#define REPORT_EVERY    5   // report every N measurement cycles
#define SMOOTH          5   // smoothing factor used for running averages


String node_id = "";


const long maxAllowedWrites = 100000; /* if evenly distributed, the ATmega328 EEPROM
should have at least 100,000 writes */
const int memBase          = 0;
//const int memCeiling       = EEPROMSizeATmega328;

#if _DS
/* Dallas bus for DS18S20 temperature */
const String DS_PIN = 8;
OneWire ds(DS_PIN);

uint8_t ds_count = 0;
uint8_t ds_search = 0;
volatile int ds_value[8]; // take on 8 DS sensors in report
enum { DS_OK, DS_ERR_CRC };
#endif // _DS

// MQ series gas sensors
//CS_MQ7  MQ7(12, 13);  // 12 = digital Pin connected to "tog" from sensor board
// 13 = digital Pin connected to LED Power Indicator

int CoSensorPower = 5; // digital power feed/indicator pin
int MthSensorPower = 6;
int CoSensorData = 0; // measure pin (analog) for CO sensor
int MthSensorData = A1;
int VccADC = A2;

const int PREHEAT_PERIOD = 600;
const int HEATING_PERIOD = 900;

// The scheduler makes it easy to perform various tasks at various times:

enum { PREHEAT, HEAT, MEASURE, REPORT, TASK_END };

static word schedbuf[TASK_END];
Scheduler scheduler (schedbuf, TASK_END);

// has to be defined because we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

// Other variables used in various places in the code:


#if LDR_PORT
Port ldr (LDR_PORT);
#endif

#if DHT_PIN
DHTxx dht (DHT_PIN); // JeeLib DHT
#endif

/* Report variables */

static byte reportCount;    // count up until next report, i.e. packet send
//static byte myNodeID;       // node ID used for this unit

// This defines the structure of the packets which get sent out by wireless:

struct {
#if LDR_PORT
	byte light  :8;     // light sensor: 0..255
#endif
	byte moved  :1;  // motion detector: 0..1
	int mq4; // methane
#if DHT_PIN
	byte rhum   :7;  // rhumdity: 0..100
	int temp    :10; // temperature: -500..+500 (tenths)
#endif
	int ctemp   :10; // atmega temperature: -500..+500 (tenths)
#if _MEM
	int memfree :16;
#endif
#if _VCC
	int vcc :10;
#endif
#if _RFM12LOBAT
	byte lobat :1;  // supply voltage dropped under 3.1V: 0..1
#endif
} payload;


/** Generic routines */

void blink(int led, int count, int length) {
	for (int i=0;i<count;i++) {
		digitalWrite (led, HIGH);
		delay(length);
		digitalWrite (led, LOW);
		delay(length);
	}
}

// spend a little time in power down mode while the SHT11 does a measurement
static void lpDelay () {
	Sleepy::loseSomeTime(32); // must wait at least 20 ms
}

#if _DS

/* Dallas DS18B20 thermometer */

static int ds_readdata(uint8_t addr[8], uint8_t data[12]) {
	byte i;
	byte present = 0;

	ds.reset();
	ds.select(addr);
	ds.write(0x44,1);         // start conversion, with parasite power on at the end

	serialFlush();
	Sleepy::loseSomeTime(800);
	//delay(1000);     // maybe 750ms is enough, maybe not
	// we might do a ds.depower() here, but the reset will take care of it.

	present = ds.reset();
	ds.select(addr);
	ds.write(0xBE);         // Read Scratchpad

#if DEBUG_DS
	Serial.print(F("Data="));
	Serial.print(present,HEX);
	Serial.print(" ");
#endif
	for ( i = 0; i < 9; i++) {           // we need 9 bytes
		data[i] = ds.read();
#if DEBUG_DS
		Serial.print(i);
		Serial.print(':');
		Serial.print(data[i], HEX);
		Serial.print(" ");
#endif
	}

	uint8_t crc8 = OneWire::crc8( data, 8);

#if DEBUG_DS
	Serial.print(" CRC=");
	Serial.print( crc8, HEX);
	Serial.println();
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
#if DEBUG || DEBUG_DS
		Serial.println(F("CRC error in ds_readdata"));
#endif
		return 0;
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

static void printDSAddrs() {
	for (int q=0;q<ds_count;q++) {
		Serial.print("Mem Address=");
		int l = 4 + ( q * 8 );
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
}

static void findDS18B20s(void) {
	byte i;
	byte addr[8];

	if (!ds.search(addr)) {
#if DEBUG || DEBUG_DS
		Serial.println("No more addresses.");
#endif
		ds.reset_search();
		if (ds_search != ds_count) {
#if DEBUG || DEBUG_DS
			Serial.print("Found ");
			Serial.print(ds_search );
			Serial.println(" addresses.");
			Serial.print("Previous search found ");
			Serial.print(ds_count);
			Serial.println(" addresses.");
#endif
		}
		updateDSCount(ds_search);
		ds_search = 0;
		return;
	}

	if ( OneWire::crc8( addr, 7) != addr[7]) {
#if DEBUG || DEBUG_DS
		Serial.println("CRC is not valid!");
#endif
		return;
	}

	ds_search++;

#if DEBUG || DEBUG_DS
	Serial.print("New Address=");
	for( i = 0; i < 8; i++) {
		Serial.print(i);
		Serial.print(':');
		Serial.print(addr[i], HEX);
		Serial.print(" ");
	}
	Serial.println();
#endif

	writeDSAddr(addr);
}
#endif //_DS


/* Initialization routines */

void doConfig(void)
{
}


/* Run-time handlers */

bool doAnnounce()
{
	Serial.print("[");
	Serial.print(node_id);
	Serial.print(".0");
	Serial.print("]");

#if LDR_PORT
	Serial.print(" ldr");
#endif
#if DHT_PIN
	Serial.print(" dht11-rhum dht11-temp");
#endif
	Serial.print(" attemp");
	Serial.print(" mq4");
#if _DS
	ds_count = readDSCount();
	for ( int i = 0; i < ds_count; i++) {
		Serial.print(" ds-");
		Serial.print(i+1);
	}
#endif
#if _MEM
	Serial.print(" memfree");
#endif
#if _VCC
	Serial.print(" vcc");
#endif
#if _RFM12LOBAT
	Serial.print(" lobat");
#endif
	Serial.println();
	serialFlush();
	return false;
}


// readout all the sensors and other values
void doMeasure(void)
{
	byte firstTime = payload.ctemp == 0; // special case to init running avg

	payload.ctemp = smoothedAverage(payload.ctemp, internalTemp(), firstTime);

#if _RFM12LOBAT
	payload.lobat = rf12_lowbat();
#endif

#if _VCC
	payload.vcc = analogRead(VccADC);
#endif

	int mq4 = analogRead(MthSensorData);
	payload.mq4 = mq4;//smoothedAverage(payload.mq4, mq4, firstTime);

	//int mq7 = analogRead(CoSensorData);
	//payload.mq7 = smoothedAverage(payload.mq7, mq7, firstTime);

#if PIR_PORT
	payload.moved = pir.state();
#endif

#if LDR_PORT
	ldr.digiWrite2(1);  // enable AIO pull-up
	byte light = ~ ldr.anaRead() >> 2;
	ldr.digiWrite2(0);  // disable pull-up to reduce current draw
	payload.light = light;//smoothedAverage(payload.light, light, firstTime);
#endif

#if DHT_PIN
	int t, h;
	if (dht.reading(t, h)) {
		payload.rhum = smoothedAverage(payload.rhum, h/10, firstTime);
		payload.temp = smoothedAverage(payload.temp, t, firstTime);
	}
#endif

#if _DS
	uint8_t addr[8];
	for ( int i = 0; i < ds_count; i++) {
		readDSAddr(i, addr);
		ds_value[i] = readDS18B20(addr);
	}
#endif
}

// periodic report, i.e. send out a packet and optionally report on serial port
void doReport(void)
{
	/* XXX no working radio */
#if SERIAL_EN
	Serial.print(node_id);
	Serial.print(".0");
	Serial.print(" ");
#if LDR_PORT
	Serial.print((int) payload.light);
	Serial.print(' ');
#endif
	//        Serial.print((int) payload.moved);
	//        Serial.print(' ');
#if DHT_PIN
	Serial.print((int) payload.rhum);
	Serial.print(' ');
	Serial.print((int) payload.temp);
	Serial.print(' ');
#endif
	Serial.print((int) payload.ctemp);
	Serial.print(' ');
	Serial.print((int) payload.mq4);
	Serial.print(' ');
#if _DS
	for (int i=0;i<ds_count;i++) {
		Serial.print((int) ds_value[i]);
		Serial.print(' ');
	}
#endif
#if _MEM
	Serial.print((int) payload.memfree);
	Serial.print(' ');
#endif
#if _VCC
	Serial.print((int) payload.vcc);
	Serial.print(' ');
#endif
#if _RFM12LOBAT
	Serial.print((int) payload.lobat);
#endif
	Serial.println();
	serialFlush();
#endif
}


/* *** Main *** {{{ */


void setup(void)
{
#if SERIAL_EN || DEBUG
	Serial.begin(57600);
	Serial.println("AirQuality+GasDetector MQ4/MQ7");
	serialFlush();
#endif

	node_id = "GAS";

	// Set up PWM, the compare registers work with output compare pins 2A and
	// 2B (OC2A and OC2B), which are Arduino pins 11 and 3 resp.
	//	pinMode(3, OUTPUT);
	//	pinMode(11, OUTPUT);

	// Set some bits... - Timer/Counter Control Register 2A
	//	TCCR2A = (1<<COM2A1) | (1<<COM2B1) | (1<<WGM21) | (1<<WGM20);
	//	// Set 64 prescaler - Timer/Counter Control Register 2B
	//	TCCR2B = (1<<CS22); // 16 MHz / 64 / 256 = 976.5625Hz
	//	// Set Output Compare Registers, which determine the duty cycle
	//	OCR2A = 180;
	//	OCR2B = 50;

	/* warn out of bound if _EEPROMEX_DEBUG */
  //EEPROM.setMemPool(memBase, memCeiling);
  /* warn if _EEPROMEX_DEBUG */
  //EEPROM.setMaxAllowedWrites(maxAllowedWrites);

	/* read first config setting from memory, if empty start first-run init */

//	uint8_t nid = EEPROM.read(0);
//	if (nid == 0x0) {
//#if DEBUG
//		Serial.println("");
//#endif
//		doConfig();
//	}

	doAnnounce();

	reportCount = REPORT_EVERY;     // report right away for easy debugging
	//scheduler.timer(HEAT, 0);    // start the measurement loop going
	scheduler.timer(MEASURE, 0);    // start the measurement loop going

	pinMode(LED, OUTPUT);

	pinMode(CoSensorData, INPUT);
	pinMode(MthSensorData, INPUT);

	pinMode(CoSensorPower, OUTPUT);
	pinMode(MthSensorPower, OUTPUT);
	digitalWrite(MthSensorPower, HIGH);

	pinMode(VccADC, INPUT);
}

void loop(void)
{
	//doMeasure();
	//doReport();
	//delay(200);
	//return;

#if DEBUG
	Serial.print('.');
	serialFlush();
#endif

	switch (scheduler.pollWaiting()) {

		case PREHEAT:
			Serial.println("PREHEAT");
			digitalWrite(CoSensorPower, HIGH);
			analogWrite(LED, HIGH);
			scheduler.timer(HEAT, PREHEAT_PERIOD);
			serialFlush();
			break;

		case HEAT:
			Serial.println("HEAT");
			analogWrite(CoSensorPower, 0x10);
			analogWrite(LED, 0x10);
			//scheduler.timer(MEASURE, HEATING_PERIOD);
			//scheduler.timer(MEASURE, HEATING_PERIOD);
			serialFlush();
			break;

		case MEASURE:
			//Serial.println("MEASURE");
			//digitalWrite(CoSensorPower, LOW);
			//analogWrite(LED, LOW);
			//scheduler.timer(HEAT, MEASURE_PERIOD);
			scheduler.timer(MEASURE, MEASURE_PERIOD);
			doMeasure();
			if (++reportCount >= REPORT_EVERY) {
				reportCount = 0;
				scheduler.timer(REPORT, 0);
			}
			break;

		case REPORT:
			doReport();
			break;
	}
}

/* *** }}} */

