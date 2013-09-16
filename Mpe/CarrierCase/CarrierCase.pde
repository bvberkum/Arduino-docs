/*
CarrierCase
*/
#define DEBUG_DHT 1
#define _EEPROMEX_DEBUG 1  // Enables logging of maximum of writes and out-of-memory

#include <stdlib.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>

#include <JeeLib.h>
#include <OneWire.h>
#include <DHT.h>
#include <EEPROM.h>
// include EEPROMEx.h
#include "EmBencode.h"

#define DEBUG   0   // set to 1 to display each loop() run and PIR trigger

#define MEMREPORT 1
#define ATMEGA_TEMP 1

#define SERIAL  1   // set to 1 to enable serial interface
#define DHT_PIN     7   // defined if DHTxx data is connected to a DIO pin
#define LDR_PORT    4   // defined if LDR is connected to a port's AIO pin

#define REPORT_EVERY    5   // report every N measurement cycles
#define SMOOTH          5   // smoothing factor used for running averages
#define MEASURE_PERIOD  50 // how often to measure, in tenths of seconds

#define RETRY_PERIOD    10  // how soon to retry if ACK didn't come in
#define RETRY_LIMIT     5   // maximum number of times to retry
#define ACK_TIME        10  // number of milliseconds to wait for an ack

#define RADIO_SYNC_MODE 2


EmBencode encoder;
uint8_t* embencBuff;
int embencBuffLen = 0;

void EmBencode::PushChar(char ch) {
	Serial.write(ch);
//	embencBuffLen += 1;
//	uint8_t* np = (uint8_t *) realloc( embencBuff, sizeof(uint8_t) * embencBuffLen);
//	if( np != NULL ) { 
//		embencBuff = np; 
//		embencBuff[embencBuffLen] = ch;
//	} else {
//		Serial.println(F("Out of Memory"));
//	}
}

enum { ANNOUNCE_MSG, REPORT_MSG,  };

/* Atmega EEPROM */
const long maxAllowedWrites = 100000; /* if evenly distributed, the ATmega328 EEPROM 
should have at least 100,000 writes */
const int memBase          = 0;
//const int memCeiling       = EEPROMSizeATmega328;

/* Dallas bus for DS18S20 temperature */
OneWire ds(A5);  // on pin 10

String node_id = "";
static byte myNodeID;       // node ID used for this unit
String inputString = "";         // a string to hold incoming data

const int ds_count = 3;
uint8_t ds_addr[ds_count][8] = {
	{ 0x28, 0xCC, 0x9A, 0xF4, 0x03, 0x00, 0x00, 0x6D },
	{ 0x28, 0x8A, 0x5B, 0xDD, 0x03, 0x00, 0x00, 0xA6 },
	{ 0x28, 0x45, 0x94, 0xF4, 0x03, 0x00, 0x00, 0xB3 }
};
volatile int ds_value[ds_count];


// The scheduler makes it easy to perform various tasks at various times:

enum { ANNOUNCE, DISCOVERY, MEASURE, REPORT, TASK_END };

static word schedbuf[TASK_END];
Scheduler scheduler (schedbuf, TASK_END);

// Other variables used in various places in the code:

static byte reportCount;    // count up until next report, i.e. packet send
//static byte myNodeID;       // node ID used for this unit

// This defines the structure of the packets which get sent out by wireless:

struct {
#if LDR_PORT
	byte light  :8;     // light sensor: 0..255
#endif
//	byte moved :1;  // motion detector: 0..1
#if DHT_PIN
	int rhum   :7;   // rhumdity: 0..100 (4 or 5% resolution?)
	int temp    :10; // temperature: -500..+500 (tenths, .5 resolution)
#endif
#if ATMEGA_TEMP
	int ctemp   :10; // atmega temperature: -500..+500 (tenths)
#endif
#if MEMREPORT
	int memfree :16;
#endif
	byte lobat  :1;  // supply voltage dropped under 3.1V: 0..1
} payload;

#if LDR_PORT
Port ldr (LDR_PORT);
#endif

#if DHT_PIN

//DHTxx dht (DHT_PIN); // JeeLib DHT

#define DHTTYPE DHT11   // DHT 11 
//#define DHTTYPE DHT22   // DHT 22  (AM2302)
DHT dht (DHT_PIN, DHTTYPE); // DHT lib
#endif

// has to be defined because we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

static void serialFlush () {
#if ARDUINO >= 100
	Serial.flush();
#endif  
	delay(2); // make sure tx buf is empty before going back to sleep
}

void blink(int led, int count, int length) {
  for (int i=0;i<count;i++) {
    digitalWrite (led, HIGH);
    delay(length);
    digitalWrite (led, LOW);
    delay(length);
  }
}

// utility code to perform simple smoothing as a running average
static int smoothedAverage(int prev, int next, byte firstTime =0) {
	if (firstTime)
		return next;
	return ((SMOOTH - 1) * prev + next + SMOOTH / 2) / SMOOTH;
}

// spend a little time in power down mode while the SHT11 does a measurement
static void lpDelay () {
	Sleepy::loseSomeTime(32); // must wait at least 20 ms
}

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

int freeRam () {
	extern int __heap_start, *__brkval; 
	int v; 
	return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

static void sendSomeData () {
  EmBencode encoder;
  // send a simple string
  encoder.push("abcde");
  // send a number of bytes, could be binary
  encoder.push("123", 3);
  // send an integer
  encoder.push(12345);
  // send a list with an int, a nested list, and an int
  encoder.startList();
    encoder.push(987);
    encoder.startList();
      encoder.push(654);
    encoder.endList();
    encoder.push(321);
  encoder.endList();
  // send a large integer
  encoder.push(999999999);
  // send a dictionary with two entries
  encoder.startDict();
    encoder.push("one");
    encoder.push(11);
    encoder.push("two");
    encoder.push(22);
  encoder.endDict();
  // send one last string
  encoder.push("bye!");
}

enum { DS_OK, DS_ERR_CRC };

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

#if DEBUG
	F("Data=");
	Serial.print(present,HEX);
	Serial.print(" ");
#endif
	for ( i = 0; i < 9; i++) {           // we need 9 bytes
		data[i] = ds.read();
#if DEBUG
		Serial.print(i);
		Serial.print(':');
		Serial.print(data[i], HEX);
		Serial.print(" ");
#endif
	}

	uint8_t crc8 = OneWire::crc8( data, 8);

#if DEBUG
	F(" CRC=");
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

static int readDS18B20(uint8_t addr[8]) {
	byte data[12];
	int SignBit;

	int result = ds_readdata(addr, data);	
	
	if (result != 0) {
		F("CRC error in ds_readdata");
		return 0;
	}

	int Tc_100 = ds_conv_temp_c(data, SignBit);

	if (SignBit) {
		return 0 - Tc_100;
	} else {
		return Tc_100;
	}
}
/*

static byte* findDS(bool do_read=false) {
	byte i;
	byte data[8];
	byte addr[8];
	int SignBit;

	if ( !ds.search(addr)) {
#if DEBUG
		Serial.println("No more addresses.");
#endif
		ds.reset_search();
		return;
	}

#if DEBUG
	Serial.print("Address=");
	for( i = 0; i < 8; i++) {
		Serial.print(i);
		Serial.print(':');
		Serial.print(addr[i], HEX);
		Serial.print(" ");
	}
#endif

	if ( OneWire::crc8( addr, 7) != addr[7]) {
#if DEBUG
		Serial.println("CRC is not valid!");
#endif
		return;
	}

#if DEBUG
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

//	ds_count += 1;
//	ds_addr[ds_count] = addr;

	if (!do_read)
		return;

	int result = ds_readdata(addr, data);	

	if (do_read)
		return;// result;
	
	if (result != 0) {
		Serial.println("CRC error in ds_readdata");
		return;
	}
}
*/

/*
void printDS18B20(bool do_read=false) {
	int Tc_100 = ds_conv_temp_c(data, SignBit);

	int Whole, Fract;
	Whole = Tc_100 / 100;  // separate off the whole and fractional portions
	Fract = Tc_100 % 100;

	// XXX: add value to payload
#if DEBUG
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
*/

static void doConfig() {
}

/**
 l
 */
static void encodePayloadConfig() {
}

static byte waitForAck() {
    MilliTimer ackTimer;
    while (!ackTimer.poll(ACK_TIME)) {
        if (rf12_recvDone() && rf12_crc == 0 &&
                // see http://talk.jeelabs.net/topic/811#post-4712
                rf12_hdr == (RF12_HDR_DST | RF12_HDR_CTL | myNodeID))
            return 1;
        set_sleep_mode(SLEEP_MODE_IDLE);
        sleep_mode();
    }
    return 0;
}

// readout all the sensors and other values
static void doMeasure() 
{
	byte firstTime = payload.ctemp == 0; // special case to init running avg

	payload.ctemp = smoothedAverage(payload.ctemp, internalTemp(), firstTime);

	payload.lobat = rf12_lowbat();

#if DHT_PIN
	float h = dht.readHumidity();
	float t = dht.readTemperature();
	if (isnan(h)) {
#if SERIAL | DEBUG
		Serial.println(F("Failed to read DHT11 humidity"));
#endif
	} else {
		payload.rhum = smoothedAverage(payload.rhum, (int)h, firstTime);
	}
	if (isnan(t)) {
#if SERIAL | DEBUG
		Serial.println(F("Failed to read DHT11 temperature"));
#endif
	} else {
		payload.temp = smoothedAverage(payload.temp, (int)t*10, firstTime);
	}
#endif

#if LDR_PORT
	ldr.digiWrite2(1);  // enable AIO pull-up
	byte light = ~ ldr.anaRead() >> 2;
	ldr.digiWrite2(0);  // disable pull-up to reduce current draw
	payload.light = smoothedAverage(payload.light, light, firstTime);
#endif

#if MEMREPORT
	payload.memfree = freeRam();
#endif

	serialFlush();
}

// periodic report, i.e. send out a packet and optionally report on serial port
static void doReport() {
	rf12_sleep(RF12_WAKEUP);
	rf12_sendNow(0, &payload, sizeof payload);
	rf12_sendWait(RADIO_SYNC_MODE);
	rf12_sleep(RF12_SLEEP);
#if SERIAL
	Serial.print(node_id);
	Serial.print(" ");
#if LDR_PORT
	Serial.print((int) payload.light);
	Serial.print(' ');
#endif
//  Serial.print((int) payload.moved);
//  Serial.print(' ');
#if DHT_PIN
	Serial.print((int) payload.rhum);
	Serial.print(' ');
	Serial.print((int) payload.temp);
	Serial.print(' ');
#endif
#if ATMEGA_TEMP
	Serial.print((int) payload.ctemp);
	Serial.print(' ');
#endif
//	Serial.print((int) ds_value[0]);
//	Serial.print(' ');
//	Serial.print((int) ds_value[1]);
//	Serial.print(' ');
//	Serial.print((int) ds_value[2]);
//	Serial.print(' ');
#if MEMREPORT
	Serial.print((int) payload.memfree);
	Serial.print(' ');
#endif
	Serial.print((int) payload.lobat);
	Serial.println();
	serialFlush();
#endif
}

void serialEvent() {
	while (Serial.available()) {
		// get the new byte:
		char inChar = (char)Serial.read(); 
		// add it to the inputString:
		if (inChar == '\n') {
			inputString = "";
		} 
		else if (inputString == "NEW ") {
			node_id += inChar;
		} 
		else {
			inputString += inChar;
		}
	}
}

void setup()
{
#if SERIAL || DEBUG
	Serial.begin(57600);
	Serial.println(F("\nCarrierCase"));
	Serial.print(F("Free RAM: "));
	Serial.println(freeRam());
	serialFlush();
	byte rf12_show = 1;
#else
	byte rf12_show = 0;
#endif
	myNodeID = 23;
	rf12_initialize(myNodeID, RF12_868MHZ, 5);
	//rf12_config(rf12_show);
    
	rf12_sleep(RF12_SLEEP); // power down

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
  
#if DHT_PIN
 	dht.begin();
#endif

	Serial.print(F("REG ROOM"));
#if LDR_PORT
	Serial.print(F(" ldr"));
#endif
#if DHT_PIN
	Serial.print(F(" dht11-rhum dht11-temp"));
#endif
#if ATMEGA_TEMP
	Serial.print(F(" attemp"));
#endif
#if MEMREPORT
	Serial.print(F(" memfree"));
#endif
	Serial.println(F(" lobat"));
	serialFlush();
	node_id = "ROOM-5-23";
	reportCount = REPORT_EVERY;     // report right away for easy debugging
	scheduler.timer(MEASURE, 0);    // start the measurement loop going
}

void loop(void)
{
#if DEBUG
	Serial.print('.');
	serialFlush();
#endif
	//while (node_id == "" || inputString != "") { // xxx get better way to wait for entire id
	//}
	// XXX: get node id or perhaps get sketch type number from central node
	//scheduler.timer(DISCOVERY, 0);    // start by completing node discovery

	switch (scheduler.pollWaiting()) {

		case DISCOVERY:
//			while (findDS()) {
#if DEBUG
//				printDS18B20(ds_addr[ds_count]);
#endif
//			}
			break;

		case ANNOUNCE:

			encodePayloadConfig();

//			rf12_sleep(RF12_WAKEUP);
//			rf12_sendNow(0, &payload_config, sizeof payload_config);
//			rf12_sendWait(RADIO_SYNC_MODE);
//			rf12_sleep(RF12_SLEEP);
//
			// report a new node or reinitialize node with central link node
//			for ( int x=0; x<ds_count; x++) {
//				Serial.print("SEN ");
//				Serial.print(node_id);
//				Serial.print(" ds-");
//				Serial.print(x);
//				for ( int y= 0; y<9; y++) {           // we need 9 bytes
//					Serial.print(" ");
//					Serial.print(ds_addr[x][y], HEX);
//				}
//				Serial.println("");
//			}
			serialFlush();
			break;

		case MEASURE:
			// reschedule these measurements periodically
			scheduler.timer(MEASURE, MEASURE_PERIOD);
			doMeasure();
			// every so often, a report needs to be sent out
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


