/*
CarrierCase

- Generally based on roomNode, probably can be made compatible with other environmental sensor nodes.
- Two moisture probes.
- Later to have shift register and analog switchting for mre inputs, perhaps.
  Make SDA/SCL free for I2C or other bus?

Current pin assignments:

Ard Atm Jee  Sign Mapping
=== === ==== ==== ===========
D0  PD0      TXD  SER/DEBUG  
D1  PD1      RXD  SER/DEBUG  
D2  PD2      INT0 JeePort IRQ
D3  PD3      INT1 RFM12B IRQ 
D4  PD4 DIO1      PIR        
D5  PD5 DIO2                 
D6  PD6 DIO3                 
D7  PD7 DIO4      DHT11      
D8  PB0                      
D9  PB1                      
D10 PB2      SS   RFM12B SEL 
D11 PB3      MOSI RFM12B SDI 
D12 PB4      MISO RFM12B DSO 
D13 PB5      SCK  RFM12B SCK 
A0  PC1 AIO1                 
A1  PC1 AIO2                 
A2  PC2 AIO3                 
A1  PC3 AIO4      LDR        
A4  PC4      SDA             
A5  PC5      SCL             
    PC6      RST  Reset      
=== === ==== ==== ===========


Todo:

Ard Atm Jee  Sign Mapping
=== === ==== ==== ===========
D4  PD4 DIO1      MP-1 RIGHT 
D5  PD5 DIO2      MP-1 LEFT  
D6  PD6 DIO3      MP-2 RIGHT 
D7  PD7 DIO4      MP-2 LEFT  
D8  PB0           MP-1 LED   
D9  PB1           MP-2 LED   
A0  PC1 AIO1      MP1 measure
A1  PC1 AIO2      MP2 measure
A2  PC2 AIO3                 
A1  PC3 AIO4      LDR        
A4  PC4      SDA  Dallas bus 
A5  PC5      SCL  DHT out    
=== === ==== ==== ===========

ToDo
  - Announce payload config

  	spec:
		detect:
			PIR
				motion
			LDR
				
  - 
*/

//#ifdef MYCMDLINE

#define DEBUG_DHT 1
#define _EEPROMEX_DEBUG 1  // Enables logging of maximum of writes and out-of-memory

#include <stdlib.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <util/atomic.h>

#include <JeeLib.h>
#include <OneWire.h>
#include <DHT.h>
//#include <EEPROM.h>
// include EEPROMEx.h
#include "EmBencode.h"

#define DEBUG   1   // set to 1 to display each loop() run and PIR trigger

#define SERIAL  1   // set to 1 to enable serial interface

#define DHT_PIN     7   // defined if DHTxx data is connected to a DIO pin
#define LDR_PORT    4   // defined if LDR is connected to a port's AIO pin
#define PIR_PORT    1
#define DS_PIN      A5
#define MEMREPORT 1
#define ATMEGA_TEMP 1

#define REPORT_EVERY    5   // report every N measurement cycles
#define SMOOTH          5   // smoothing factor used for running averages
#define MEASURE_PERIOD  50  // how often to measure, in tenths of seconds

#define RETRY_PERIOD    10   // how soon to retry if ACK didn't come in
#define RETRY_LIMIT     2   // maximum number of times to retry
#define ACK_TIME        10  // number of milliseconds to wait for an ack

#define RADIO_SYNC_MODE 2


static String node_id = "";

/* embenc for storage and xfer proto */

// unused?
uint8_t* embencBuff;
int embencBuffLen = 0;

// dynamic allocation for embencBuff
void embenc_push(char ch) {
	embencBuffLen += 1;
	uint8_t* np = (uint8_t *) realloc( embencBuff, sizeof(uint8_t) * embencBuffLen);
	if( np != NULL ) { 
		embencBuff = np; 
		embencBuff[embencBuffLen] = ch;
	} else {
		Serial.println(F("Out of Memory"));
	}
}

//void EmBencode::PushChar(char ch) {
//	Serial.write(ch);
//	embenc_push(ch);
//}

void freeEmbencBuff() {
	embencBuff = (uint8_t *) realloc(embencBuff, 0); // free??
	embencBuffLen = 0;
}


/* Atmega EEPROM stuff */
const long maxAllowedWrites = 100000; /* if evenly distributed, the ATmega328 EEPROM 
should have at least 100,000 writes */
const int memBase          = 0;
//const int memCeiling       = EEPROMSizeATmega328;

/* RF12 message types */
enum { ANNOUNCE_MSG, REPORT_MSG };


#if DS_PIN
/* Dallas bus for DS18S20 temperature */
OneWire ds(DS_PIN);  // on pin 10

const int ds_count = 3;
uint8_t ds_addr[ds_count][8] = {
	{ 0x28, 0xCC, 0x9A, 0xF4, 0x03, 0x00, 0x00, 0x6D },
	{ 0x28, 0x8A, 0x5B, 0xDD, 0x03, 0x00, 0x00, 0xA6 },
	{ 0x28, 0x45, 0x94, 0xF4, 0x03, 0x00, 0x00, 0xB3 }
};
volatile int ds_value[ds_count];
#endif

// The scheduler makes it easy to perform various tasks at various times:

enum { ANNOUNCE, DISCOVERY, MEASURE, REPORT, TASK_END };

static word schedbuf[TASK_END];
Scheduler scheduler (schedbuf, TASK_END);

// Other variables used in various places in the code:

static byte reportCount;    // count up until next report, i.e. packet send
static byte rf12_id;       // node ID used for this unit

// This defines the structure of the packets which get sent out by wireless:

struct {
	int msgtype :8; // XXX: incorporated, but not the right place..
#if LDR_PORT
	byte light  :8;     // light sensor: 0..255
#endif
#if PIR_PORT
	byte moved  :1;  // motion detector: 0..1
#endif
#if DHT_PIN
	int rhum    :7;   // rhumdity: 0..100 (4 or 5% resolution?)
	int temp    :10; // temperature: -500..+500 (tenths, .5 resolution)
#endif
#if ATMEGA_TEMP
	int attemp  :10; // atmega temperature: -500..+500 (tenths)
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

/**
 l
 */
static bool waitForAck() {
    MilliTimer ackTimer;
    while (!ackTimer.poll(ACK_TIME)) {
        if (rf12_recvDone() && rf12_crc == 0 &&
                // see http://talk.jeelabs.net/topic/811#post-4712
                rf12_hdr == (RF12_HDR_DST | RF12_HDR_CTL | rf12_id))
            return 1;
        set_sleep_mode(SLEEP_MODE_IDLE);
        sleep_mode();
    }
    return 0;
}

static void serialFlush () {
#if ARDUINO >= 100
	Serial.flush();
#endif  
	delay(2); // make sure tx buf is empty before going back to sleep
}


#if PIR_PORT

#define PIR_INVERTED    0   // 0 or 1, to match PIR reporting high or low
#define PIR_HOLD_TIME   30  // hold PIR value this many seconds after change
/// Interface to a Passive Infrared motion sensor.
    class PIR : public Port {
        volatile byte value, changed;
    public:
        volatile uint32_t lastOn;
        PIR (byte portnum)
            : Port (portnum), value (0), changed (0), lastOn (0) {}

        // this code is called from the pin-change interrupt handler
        void poll() {
            // see http://talk.jeelabs.net/topic/811#post-4734 for PIR_INVERTED
            byte pin = digiRead() ^ PIR_INVERTED;
            // if the pin just went on, then set the changed flag to report it
            if (pin) {
                if (!state())
                    changed = 1;
                lastOn = millis();
            }
            value = pin;
        }

        // state is true if curr value is still on or if it was on recently
        byte state() const {
            byte f = value;
            if (lastOn > 0)
                ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
                    if (millis() - lastOn < 1000 * PIR_HOLD_TIME)
                        f = 1;
                }
            return f;
        }

        // return true if there is new motion to report
        byte triggered() {
            byte f = changed;
            changed = 0;
            return f;
        }
    };


PIR pir (PIR_PORT);

ISR(PCINT2_vect) { pir.poll(); }

// has to be defined because we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

// send packet and wait for ack when there is a motion trigger
static void doTrigger() {
    #if DEBUG
        Serial.print("PIR ");
        Serial.print((int) payload.moved);
		Serial.print(' ');
        Serial.print((int) pir.lastOn);
		Serial.println(' ');
        serialFlush();
    #endif

    for (byte i = 0; i < RETRY_LIMIT; ++i) {
        rf12_sleep(RF12_WAKEUP);
        rf12_sendNow(RF12_HDR_ACK, &payload, sizeof payload);
        rf12_sendWait(RADIO_SYNC_MODE);
        byte acked = waitForAck();
        rf12_sleep(RF12_SLEEP);

        if (acked) {
            #if DEBUG
                Serial.print(" ack ");
                Serial.println((int) i);
                serialFlush();
            #endif
            // reset scheduling to start a fresh measurement cycle
            scheduler.timer(MEASURE, MEASURE_PERIOD);
            return;
        }
        
        delay(RETRY_PERIOD * 100);
    }
    scheduler.timer(MEASURE, MEASURE_PERIOD);
    #if DEBUG
        Serial.println(" no ack!");
        serialFlush();
    #endif
}
#endif

void blink(int led, int count, int length) {
  for (int i=0;i<count;i++) {
    digitalWrite (led, HIGH);
    delay(length);
    digitalWrite (led, LOW);
    delay(length);
  }
}

static void showNibble (byte nibble) {
	char c = '0' + (nibble & 0x0F);
	if (c > '9')
		c += 7;
	Serial.print(c);
}

bool useHex = 0;

static void showByte (byte value) {
	if (useHex) {
		showNibble(value >> 4);
		showNibble(value);
	} else {
		Serial.print((int) value);
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

// XXX: just an example, not actually used 
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

static void writeConfig() {
	eeprom_write_byte(RF12_EEPROM_ADDR, 4);
	eeprom_write_byte(RF12_EEPROM_ADDR+1, 5);
//	eeprom_write_byte(RF12_EEPROM_ADDR+2, );
	// JeeLib RF12_EEPROM_ADDR is at 20
    int nodeId = eeprom_read_byte(RF12_EEPROM_ADDR);
    int group = eeprom_read_byte(RF12_EEPROM_ADDR + 1);

    Serial.print("nodeId: ");
    Serial.println(nodeId);
    Serial.print("group: ");
    Serial.println(group);
    serialFlush();
}

#if DS_PIN
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
	Serial.print(F("Data="));
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
	Serial.print(F(" CRC="));
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
		Serial.println(F("CRC error in ds_readdata"));
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
#endif

static void doConfig() {
	// JeeLib RF12_EEPROM_ADDR is at 20
    int nodeId = eeprom_read_byte(RF12_EEPROM_ADDR);
    int group = eeprom_read_byte(RF12_EEPROM_ADDR + 1);

	// Read embencoded from eeprom
	int specByteCnt= eeprom_read_byte(RF12_EEPROM_ADDR + 2);
	uint8_t specRaw [specByteCnt]; 
	eeprom_read_block( 
			(void*)specRaw, 
			(const void*)RF12_EEPROM_ADDR + 3,
			specByteCnt 
		);

	char embuf [200];
	EmBdecode decoder (embuf, sizeof embuf);
	uint8_t bytes;
	int i = 0;
	// XXX: fix this, catch longer than scenario, check for remaining raw spec
	while (!bytes) {
		bytes = decoder.process(specRaw[i]);
		i += 1;
	}

	if (!bytes) {
		Serial.println("Unable to decode");
	} else {
		Serial.print(" => ");
		Serial.print((int) bytes);
		Serial.println(" bytes");
		for (;;) {
			uint8_t token = decoder.nextToken();
			if (token == EmBdecode::T_END)
				break;
			switch (token) {
				case EmBdecode::T_STRING:
					Serial.print(" string: ");
					Serial.println(decoder.asString());
					break;
				case EmBdecode::T_NUMBER:
					Serial.print(" number: ");
					Serial.println(decoder.asNumber());
					break;
				case EmBdecode::T_DICT:
					Serial.println(" > dict");
					break;
				case EmBdecode::T_LIST:
					Serial.println(" > list");
					break;
				case EmBdecode::T_POP:
					Serial.println(" < pop");
					break;
			}
		}
		decoder.reset();
	}
}

static bool doAnnounce() {
#if SERIAL
	Serial.print(node_id);
	Serial.print(' ');
	Serial.print(rf12_id);
	Serial.print(' ');
#endif

	embenc_push(ANNOUNCE_MSG);

	EmBencode payload_spec;

	payload_spec.startList();
#if LDR_PORT
	payload_spec.startList();
	// name, may be unique but can be part number and as specific or generic as possible
	payload_spec.push("ldr2250"); 
	// bits in payload
	payload_spec.push(8);
	// XXX decoder params, optional?
	payload_spec.push("int(0-255)");
	payload_spec.endList();
#if SERIAL
	Serial.print(F(" ldr"));
#endif
#endif
#if PIR_PORT
	payload_spec.startList();
	payload_spec.push("moved");
	payload_spec.push(1);
	payload_spec.endList();
#if SERIAL
	Serial.print("moved ");
#endif
#endif
#if DHT_PIN
	payload_spec.startList();
	payload_spec.push("dht11-rhum");
	payload_spec.push(7);
	payload_spec.endList();
#if SERIAL
	Serial.print(F(" dht11-rhum dht11-temp"));
#endif
#endif
#if ATMEGA_TEMP
	payload_spec.startList();
	payload_spec.push("attemp");
	payload_spec.push(10);
	payload_spec.endList();
#if SERIAL
	Serial.print(F(" attemp"));
#endif
#endif
#if MEMREPORT
	payload_spec.startList();
	payload_spec.push("memfree");
	payload_spec.push(16);
	payload_spec.endList();
#if SERIAL
	Serial.print(F(" memfree"));
#endif
#endif
	payload_spec.startList();
	payload_spec.push("lobat");
	payload_spec.endList();
#if SERIAL
	Serial.println(F(" lobat"));
	serialFlush();
#endif
	payload_spec.endList();

	Serial.println();
	for (int i=0; i<embencBuffLen; i++) {
		//showByte(embencBuff[i]);
		if (i > 0) Serial.print(' ');
		Serial.print((int)embencBuff[i]);
	}
	Serial.println();
	Serial.print(F("Sending"));
	Serial.print((int)embencBuffLen);
	Serial.println(F("bytes"));
	serialFlush();

	if (embencBuffLen > 66) {
	// FIXME: need to send in chunks
	}

	rf12_sleep(RF12_WAKEUP);
	rf12_sendNow(
		(rf12_id & RF12_HDR_MASK) | RF12_HDR_ACK,
		&embencBuff, 
		sizeof embencBuff);
	rf12_sendWait(RADIO_SYNC_MODE);
	bool acked = waitForAck();
	
	acked = 1;

	freeEmbencBuff();

	if (acked) {
//		Serial.println("ACK");
//	} else {
//		Serial.println("NACK");
	}
	Serial.print(F("Free RAM: "));
	Serial.println(freeRam());
	serialFlush();

	rf12_sleep(RF12_SLEEP);
	return acked;

#if DS_PIN
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
#endif
}

// readout all the sensors and other values
static void doMeasure() 
{
	byte firstTime = payload.attemp == 0; // special case to init running avg

	payload.attemp = smoothedAverage(payload.attemp, internalTemp(), firstTime);

	payload.lobat = rf12_lowbat();

#if PIR_PORT
	payload.moved = pir.state();
#endif

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
#endif //DHT

#if LDR_PORT
	//ldr.digiWrite2(1);  // enable AIO pull-up
	byte light = ~ ldr.anaRead() >> 2;
	ldr.digiWrite2(0);  // disable pull-up to reduce current draw
	payload.light = smoothedAverage(payload.light, light, firstTime);
#endif //LDR

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
#if PIR_PORT
	Serial.print((int) payload.moved);
	Serial.print(' ');
#endif
#if DHT_PIN
	Serial.print((int) payload.rhum);
	Serial.print(' ');
	Serial.print((int) payload.temp);
	Serial.print(' ');
#endif
#if ATMEGA_TEMP
	Serial.print((int) payload.attemp);
	Serial.print(' ');
#endif
#if DS_PIN
//	Serial.print((int) ds_value[0]);
//	Serial.print(' ');
//	Serial.print((int) ds_value[1]);
//	Serial.print(' ');
//	Serial.print((int) ds_value[2]);
//	Serial.print(' ');
#endif
#if MEMREPORT
	Serial.print((int) payload.memfree);
	Serial.print(' ');
#endif
	Serial.print((int) payload.lobat);
	Serial.println();
	serialFlush();
#endif//SERIAL
}


String inputString = "";         // a string to hold incoming data

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

/*
  *
  * Main
  *
   */

void setup()
{
#if SERIAL || DEBUG
	Serial.begin(57600);
	Serial.println(F("\n[CarrierCase]"));
#if DEBUG
	Serial.print(F("Free RAM: "));
	Serial.println(freeRam());
	byte rf12_show = 1;
#else
	byte rf12_show = 0;
#endif
	serialFlush();
#endif

	writeConfig();
	//doConfig();
	return;


	// Reported on serial?
	node_id = "CarrierCase.0-2-RF12-5-23";
	//<sketch>.0-<count>-RF12....
	rf12_id = 23;
	rf12_initialize(rf12_id, RF12_868MHZ, 5);
	//rf12_config(rf12_show);
    
	rf12_sleep(RF12_SLEEP); // power down

	// PIR pull down and interrupt 
	pir.digiWrite(0);
	// PCMSK2 = PCIE2 = PCINT16-23 = D0-D7
	bitSet(PCMSK2,  3 + PIR_PORT); // DIO1
	bitSet(PCICR, PCIE2); // enable PCMSK2 for PCINT at DIO1-4 (D4-7)

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
		doConfig();
//	}
  
#if DHT_PIN
 	dht.begin();
#endif

	reportCount = REPORT_EVERY;     // report right away for easy debugging
	scheduler.timer(ANNOUNCE, 0);
}

int line = 0;
void loop(void)
{
	return;
#if DEBUG
	line++;
	Serial.print('.');
	if (line > 79) {
		line = 0;
		Serial.println();
	}
	serialFlush();
#endif

#if PIR_PORT
	if (pir.triggered()) {
		payload.moved = pir.state();
		doTrigger();
	}
#endif

	switch (scheduler.pollWaiting()) {

		case ANNOUNCE:
			if (doAnnounce()) {
				scheduler.timer(MEASURE, 0);
			} else {
				scheduler.timer(ANNOUNCE, 100);
			}
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
			payload.msgtype = REPORT_MSG;
			doReport();
			break;

		case DISCOVERY:
			Serial.print("discovery? ");
			break;

		case TASK_END:
			Serial.print("task? ");
			break;
	}
}

//#endif
