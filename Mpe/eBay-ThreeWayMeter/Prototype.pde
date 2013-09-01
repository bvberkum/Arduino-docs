/*
	- Starting based on MoistureNode, should merge back into there.

	- First version: flipping probes on jack A and B, connected to JP3, 4.
	- Planning JP1 and 2 to be shift register control

	- Need to adapt wiring for eBay node. 
		Testing radio.

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

JeeLib RF12 
	- Header byte is CTL, DST, ACK bits and 5bit node ID.
	- Node ID 0 and 31 is special. 30 unique IDs left per group.

	CTL, DST, ACK bits
		- 001 broadcast with ack-request
		- 010 direct msg
		- 011 direct with ack-request
		- 100 control/ack broadcast?
		- 101 control broadcast with ack-request (unused)
		- 110 ack response
		- 111 control for node with ack-request

	

*/
#define _EEPROMEX_DEBUG 1  // Enables logging of maximum of writes and out-of-memory

#include <avr/sleep.h>
#include <util/crc16.h>
#include <avr/eeprom.h>
//include <EEPROM.h>
//include EEPROMEx.h
#include <JeeLib.h>
#include <OneWire.h>

#define DEBUG   0   // set to 1 to display each loop() run and PIR trigger
#define DEBUG_DS   0
#define FIND_DS    0

#define SERIAL  1   // set to 1 to enable serial interface

#define MEASURE_PERIOD  50 // how often to measure, in tenths of seconds
#define REPORT_EVERY    5   // report every N measurement cycles
#define SMOOTH          5   // smoothing factor used for running averages

#define ACK_TIME        10  // number of milliseconds to wait for an ack

#define RADIO_SYNC_MODE 2

#define COLLECT 0x20 // collect mode, i.e. pass incoming without sending acks

const char helpText1[] PROGMEM = 
  "\n"
  "Available commands:" "\n"
  "  <nn> i     - set node ID (standard node ids are 1..30)" "\n"
  "  <n> b      - set MHz band (4 = 433, 8 = 868, 9 = 915)" "\n"
  "  <nnn> g    - set network group (RFM12 only allows 212, 0 = any)" "\n"
  "  <n> c      - set collect mode (advanced, normally 0)" "\n"
  "  t          - broadcast max-size test packet, request ack" "\n"
  "  ...,<nn> a - send data packet to node <nn>, request ack" "\n"
  "  ...,<nn> s - send data packet to node <nn>, no ack" "\n"
  "  <n> l      - turn activity LED on PB1 on or off" "\n"
  "  <n> q      - set quiet mode (1 = don't report bad packets)" "\n"
  "  <n> x      - set reporting format (0 = decimal, 1 = hex)" "\n"
  "  123 z      - total power down, needs a reset to start up again" "\n"
;

// EEPROMEx 
const long maxAllowedWrites = 100000; /* if evenly distributed, the ATmega328 EEPROM 
should have at least 100,000 writes */
const int memBase          = 0;
//const int memCeiling       = EEPROMSizeATmega328;

/* RF12 config (from RF12demo) */

typedef struct {
  byte nodeId;
  byte group;
  char msg[RF12_EEPROM_SIZE-4];
  word crc;
} RF12Config;

static RF12Config config;

static byte quiet;
static byte useHex;


static void showNibble (byte nibble) {
  char c = '0' + (nibble & 0x0F);
  if (c > '9')
    c += 7;
  Serial.print(c);
}

static void showByte (byte value) {
  if (useHex) {
    showNibble(value >> 4);
    showNibble(value);
  } else
    Serial.print((int) value);
}

static void addCh (char* msg, char c) {
  byte n = strlen(msg);
  msg[n] = c;
}

static void addInt (char* msg, word v) {
  if (v >= 10)
    addInt(msg, v / 10);
  addCh(msg, '0' + v % 10);
}

static void saveConfig () {
  // set up a nice config string to be shown on startup
  memset(config.msg, 0, sizeof config.msg);
  strcpy(config.msg, " ");
  
  byte id = config.nodeId & 0x1F;
  addCh(config.msg, '@' + id);
  strcat(config.msg, " i");
  addInt(config.msg, id);
  if (config.nodeId & COLLECT)
    addCh(config.msg, '*');
  
  strcat(config.msg, " g");
  addInt(config.msg, config.group);
  
  strcat(config.msg, " @ ");
  static word bands[4] = { 315, 433, 868, 915 };
  word band = config.nodeId >> 6;
  addInt(config.msg, bands[band]);
  strcat(config.msg, " MHz ");
  
  config.crc = ~0;
  for (byte i = 0; i < sizeof config - 2; ++i)
    config.crc = _crc16_update(config.crc, ((byte*) &config)[i]);

  // save to EEPROM
  for (byte i = 0; i < sizeof config; ++i) {
    byte b = ((byte*) &config)[i];
    eeprom_write_byte(RF12_EEPROM_ADDR + i, b);
  }
  
  if (!rf12_config())
    Serial.println("config save failed");
}

/* Dallas OneWire bus with registration for DS18B20 temperature sensors */
OneWire ds(4);

const int ds_count = 1;
uint8_t ds_addr[ds_count][8] = {
//	{ 0x28, 0xCC, 0x9A, 0xF4, 0x03, 0x00, 0x00, 0x6D },
//	{ 0x28, 0x8A, 0x5B, 0xDD, 0x03, 0x00, 0x00, 0xA6 },
//	{ 0x28, 0x45, 0x94, 0xF4, 0x03, 0x00, 0x00, 0xB3 }
//	{ 0x28, 0x8, 0x76, 0xF4, 0x3, 0x0, 0x0, 0xD5 },
//	{ 0x28, 0x82, 0x27, 0xDD, 0x3, 0x0, 0x0, 0x4B },
};
#if DEBUG_DS
volatile int ds_value[ds_count];
#endif

enum { DS_OK, DS_ERR_CRC };

/* Custom */
String node_id = "";
String inputString = "";         // a string to hold incoming data

// The scheduler makes it easy to perform various tasks at various times:

enum { ANNOUNCE, MEASURE, REPORT, TASK_END };

static word schedbuf[TASK_END];
Scheduler scheduler (schedbuf, TASK_END);

// Other variables used in various places in the code:

static byte reportCount;    // count up until next report, i.e. packet send
static byte myNodeID;       // node ID used for this unit

// This defines the structure of the packets which get sent out by wireless:

struct {
//	int probe1 :10;  // moisture detector: 0..100?
//	int probe2 :10; 
//	int temp1  :10;  // temperature: -500..+500 (tenths)
//	int temp2  :10;  // temperature: -500..+500 (tenths)
	int ctemp  :10;  // atmega temperature: -500..+500 (tenths)
	byte lobat :1;   // supply voltage dropped under 3.1V: 0..1
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

/* 
   Dallas DS18B20 thermometer 
 */
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

	payload.lobat = rf12_lowbat();

//	int p1 = readProbe();
//	payload.probe1 = smoothedAverage(payload.probe1, p1, firstTime); 

//	int t1 = readDS18B20(ds_addr[0]);
//	if (t1 > 0 && t1 < 8500) {
//		payload.temp1 = t1 / 10; // convert to tenths instead of .05
//	}

	serialFlush();
}

// periodic report, i.e. send out a packet and optionally report on serial port
static void doReport() {
	rf12_sleep(RF12_WAKEUP);
	rf12_sendNow(0, &payload, sizeof payload);
	rf12_sendWait(RADIO_SYNC_MODE);
	rf12_sleep(RF12_SLEEP);
	/* no working radio */
#if SERIAL
	Serial.print(node_id);
	Serial.print(' ');
	Serial.print((int) payload.ctemp);
	Serial.print(' ');
//	Serial.print((int) payload.probe1);
//	Serial.print(' ');
//	Serial.print((int) payload.probe2);
//	Serial.print(' ');
//	Serial.print((int) payload.temp1);
//	Serial.print(' ');
//	Serial.print((int) payload.temp2);
//	Serial.print(' ');
	Serial.print((int) payload.lobat);
	Serial.println();
	serialFlush();
#endif
}

static void showString (PGM_P s) {
  for (;;) {
    char c = pgm_read_byte(s++);
    if (c == 0)
      break;
    if (c == '\n')
      Serial.print('\r');
    Serial.print(c);
  }
}

static void showHelp () {
  showString(helpText1);
  Serial.println("Current configuration:");
  rf12_config();
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
	Serial.println("ThreeWayMeter");
	serialFlush();
	byte rf12_show = 0;
#else
	byte rf12_show = 0;
#endif

	if (rf12_config(rf12_show)) {
		config.nodeId = eeprom_read_byte(RF12_EEPROM_ADDR);
		config.group = eeprom_read_byte(RF12_EEPROM_ADDR + 1);
	} else {
		config.nodeId = 0x81; // 868 MHz, node 1
		config.group = 0xD4;  // 212 - default group
		saveConfig();
	}

    rf12_sleep(RF12_SLEEP); // power down

	/* read first config setting from memory, if empty start first-run init */
//	uint8_t nid = EEPROM.read(0);
//	if (nid == 0x0) {
//#if DEBUG
//		Serial.println("");
//#endif
//		doConfig();
//	}

	Serial.println("REG TWM attemp lobat");
	serialFlush();
	node_id = "TWM-1"; // xxx: hardcoded b/c handshake is not reliable?? must be some Py Twisted buffer thing I don't get

	reportCount = REPORT_EVERY;     // report right away for easy debugging
	scheduler.timer(ANNOUNCE, 0);    // start the measurement loop going
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
		
	byte test = 0x3f;

	switch (scheduler.pollWaiting()) {

		case ANNOUNCE:
			rf12_sendNow(
				(config.nodeId & RF12_HDR_MASK) ^ RF12_HDR_ACK ^ RF12_HDR_CTL,
				&test, sizeof test);
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



