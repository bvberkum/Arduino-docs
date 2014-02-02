/*

MoistureBug

	- Working toward MoistureBug for PipeConnectors
	- Perhaps demux, switching later
	- Display for probe calibration

	- First version: flipping probes on jack A and B, connected to JP3, 4.
	- Planning JP1 and 2 to be shift register control

Hardware
	MoistureBug w/ PipeBoard
		stereo sleeve A - DIO JP3 (D6)
		stereo ring B - AIO JP3 (A2)
		stereo ring A - AIO JP4 (A3)
		stereo sleeve B - DIO JP4 (D7)
	eBay
		SDA (A4) probe left (steel?)
		SCL (A5) tip left (iron?)
		(A6) tip right (iron?)
		(A7) probe right (aluminium)
		(A8) metercoil

*/
#define _EEPROMEX_DEBUG 1  // Enables logging of maximum of writes and out-of-memory

#include <JeeLib.h>
#include <avr/sleep.h>
#include <OneWire.h>
#include <EEPROM.h>
// include EEPROMEx.h

#define DEBUG   0   // set to 1 to display each loop() run and PIR trigger
#define DEBUG_DS   0
#define FIND_DS    0

#define SERIAL  1   // set to 1 to enable serial interface

#define MEASURE_PERIOD  60 // how often to measure, in tenths of seconds
#define REPORT_EVERY    5   // report every N measurement cycles
#define SMOOTH          5   // smoothing factor used for running averages


/* The pin connected to the 100 ohm side */
#define voltageFlipPinA 9
/* The pin connected to the 50k-100k (measuring) side. */
#define voltageFlipPinAMeasure 10
/* The analog pin for measuring */
#define sensorPinA 2
//#define sensorPinAB 3

int flipTimer = 1000;


const long maxAllowedWrites = 100000; /* if evenly distributed, the ATmega328 EEPROM 
should have at least 100,000 writes */
const int memBase          = 0;
//const int memCeiling       = EEPROMSizeATmega328;


OneWire ds(4);

const int ds_count = 1;
uint8_t ds_addr[ds_count][8] = {
//	{ 0x28, 0xCC, 0x9A, 0xF4, 0x03, 0x00, 0x00, 0x6D },
//	{ 0x28, 0x8A, 0x5B, 0xDD, 0x03, 0x00, 0x00, 0xA6 },
//	{ 0x28, 0x45, 0x94, 0xF4, 0x03, 0x00, 0x00, 0xB3 }
//	{ 0x28, 0x8, 0x76, 0xF4, 0x3, 0x0, 0x0, 0xD5 },
	{ 0x28, 0x82, 0x27, 0xDD, 0x3, 0x0, 0x0, 0x4B },
};
#if DEBUG_DS
volatile int ds_value[ds_count];
#endif

enum { DS_OK, DS_ERR_CRC };


String node_id = "";
String inputString = "";         // a string to hold incoming data

// The scheduler makes it easy to perform various tasks at various times:

enum { DISCOVERY, MEASURE, REPORT, TASK_END };

static word schedbuf[TASK_END];
Scheduler scheduler (schedbuf, TASK_END);

// Other variables used in various places in the code:

static byte reportCount;    // count up until next report, i.e. packet send
//static byte myNodeID;       // node ID used for this unit

// This defines the structure of the packets which get sent out by wireless:

struct {
	int probe1 :10;  // moisture detector: 0..100?
	int probe2 :10; 
	int temp1  :10;  // temperature: -500..+500 (tenths)
	int temp2  :10;  // temperature: -500..+500 (tenths)
	int ctemp  :10;  // atmega temperature: -500..+500 (tenths)
} payload;

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
	Serial.print("Data=");
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

static int readDS18B20(uint8_t addr[8]) {
	byte data[12];
	int SignBit;

	int result = ds_readdata(addr, data);	
	
	if (result != 0) {
#if DEBUG || DEBUG_DS
		Serial.println("CRC error in ds_readdata");
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
#if DEBUG || DEBUG_DS
		Serial.println("No more addresses.");
#endif
		ds.reset_search();
		return;
	}

#if DEBUG || DEBUG_DS
	Serial.print("Address=");
	for( i = 0; i < 8; i++) {
		Serial.print(i);
		Serial.print(':');
		Serial.print(addr[i], HEX);
		Serial.print(" ");
	}
#endif

	if ( OneWire::crc8( addr, 7) != addr[7]) {
#if DEBUG || DEBUG_DS
		Serial.println("CRC for address is not valid!");
#endif
		return;
	}

#if DEBUG || DEBUG_DS
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
#if DEBUG || DEBUG_DS
		Serial.println("CRC error in ds_readdata");
#endif
		return;
	}

	int Tc_100 = ds_conv_temp_c(data, SignBit);

	int Whole, Fract;
	Whole = Tc_100 / 100;  // separate off the whole and fractional portions
	Fract = Tc_100 % 100;

	// XXX: add value to payload
#if DEBUG || DEBUG_DS
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

static void doConfig() {
}

void setSensorPolarity(int flip)
{
  if (flip == -1) {
    digitalWrite(voltageFlipPinA, HIGH);
    digitalWrite(voltageFlipPinAMeasure, LOW);
  } 
  else if (flip == 1) {
    digitalWrite(voltageFlipPinA, LOW);
    digitalWrite(voltageFlipPinAMeasure, HIGH);  
  } 
  else {
    digitalWrite(voltageFlipPinA, LOW);
    digitalWrite(voltageFlipPinAMeasure, LOW);
  }
}

int readProbe() {

  setSensorPolarity(-1);
  delay(flipTimer);
  int val1 = analogRead(sensorPinA);
//  Serial.print("v1: ");Serial.println(val1);

  setSensorPolarity(1);
  delay(flipTimer);
  int val2 = 1023 - analogRead(sensorPinA);
//  Serial.print("v2: ");Serial.println(val2);
  
  setSensorPolarity(0);

  return ( val1 + val2 ) / 2;
}

// readout all the sensors and other values
static void doMeasure() {
#if DEBUG_DS
	for ( int i = 0; i < ds_count; i++) {
		ds_value[i] = readDS18B20(ds_addr[i]);
		Serial.print("Debug DS: ");
		Serial.print(i);
		Serial.print('=');
		Serial.println(ds_value[i]);
	}
#endif

	byte firstTime = payload.ctemp == 0; // special case to init running avg

	int ctemp = internalTemp();
	payload.ctemp = smoothedAverage(payload.ctemp, ctemp, firstTime);

	int p1 = readProbe();
	payload.probe1 = smoothedAverage(payload.probe1, p1, firstTime); 

	int t1 = readDS18B20(ds_addr[0]);
	if (t1 > 0 && t1 < 8500) {
		payload.temp1 = t1 / 10; // convert to tenths instead of .05
	}

	serialFlush();
}

// periodic report, i.e. send out a packet and optionally report on serial port
static void doReport() {
	/* no working radio */
#if SERIAL
	Serial.print(node_id);
	Serial.print(" ");
	Serial.print((int) payload.ctemp);
	Serial.print(" ");
	Serial.print((int) payload.probe1);
	Serial.print(" ");
	Serial.print((int) payload.probe2);
	Serial.print(" ");
	Serial.print((int) payload.temp1);
	Serial.print(" ");
	Serial.print((int) payload.temp2);
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
	//Serial.begin(38400);
//	Serial.println("MoistureBug");
	serialFlush();
#endif

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
  pinMode(voltageFlipPinA, OUTPUT);
  pinMode(voltageFlipPinAMeasure, OUTPUT);
  pinMode(sensorPinA, INPUT);

	Serial.println("REG MOIST attemp mp-1 mp-2 ds-1 ds-2");
	serialFlush();
	node_id = "MOIST-1"; // xxx: hardcoded b/c handshake is not reliable?? must be some Py Twisted buffer thing I don't get
	reportCount = REPORT_EVERY;     // report right away for easy debugging
	scheduler.timer(MEASURE, 0);    // start the measurement loop going
}

void loop(){

#if FIND_DS
	printDS18B20s();
	return;
#endif

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
			// report a new node or reinitialize node with central link node
			// fixme: put probe config here?
			//for ( int x=0; x<ds_count; x++) {
			//	Serial.print("SEN ");
			//	Serial.print(node_id);
			//	Serial.println("");
			//}
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


