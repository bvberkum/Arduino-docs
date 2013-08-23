/*
- Starting based on GasDetector.

- Working toward MoistureBug for PipeConnectors
- Perhaps demux, switching later

- First version: flipping probes on jack A and B, connected to JP3, 4.
- Planning JP1 and 2 to be shift register control

*/
#define _EEPROMEX_DEBUG 1  // Enables logging of maximum of writes and out-of-memory

#include <JeeLib.h>
#include <avr/sleep.h>
// include <OneWire.h>
#include <EEPROM.h>
// include EEPROMEx.h

#define DEBUG   0   // set to 1 to display each loop() run and PIR trigger

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
	int probe1 :10;  // moisture detector: 0..100
	byte probe2 :7;  // moisture detector: 0..100
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
	byte firstTime = payload.probe1 == 0; // special case to init running avg

	int p1 = readProbe();
//	Serial.println(p1);
	payload.probe1 = p1;

	serialFlush();

	payload.ctemp = internalTemp();

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

	Serial.println("REG MOIST attemp mp-1 mp-2");
	serialFlush();
	node_id = "MOIST-1"; // xxx: hardcoded b/c handshake is not reliable?? must be some Py Twisted buffer thing I don't get
	reportCount = REPORT_EVERY;     // report right away for easy debugging
	scheduler.timer(MEASURE, 0);    // start the measurement loop going
}

void loop(){

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


