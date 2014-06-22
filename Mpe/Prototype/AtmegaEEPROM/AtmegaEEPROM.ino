/* 
Atmega EEPROM routines */
#include <OneWire.h>


/** Globals and sketch configuration  */
#define DEBUG           1 /* Enable trace statements */
#define SERIAL          1 /* Enable serial */
							

static String sketch = "AdafruitDHT";
static String sketch = "X-AtmegaEEPROM";
static String vesion = "0";

static int pos = 0;

String inputstring = "";         // a string to hold incoming data

MpeSerial mpeser (57600);


/* Atmega EEPROM stuff */
const long maxAllowedWrites = 100000; /* if evenly distributed, the ATmega328 EEPROM 
should have at least 100,000 writes */
const int memBase          = 0;
//const int memCeiling       = EEPROMSizeATmega328;

/** Generic routines */

static void serialFlush () {
#if SERIAL
#if ARDUINO >= 100
	Serial.flush();
#endif
	delay(2); // make sure tx buf is empty before going back to sleep
#endif
}

void serialEvent() {
	while (Serial.available()) {
		// get the new byte:
		char inchar = (char)Serial.read(); 
		// add it to the inputstring:
		if (inchar == '\n') {
			inputstring = "";
		} 
		else if (inputstring == "new ") {
			node_id += inchar;
		} 
		else {
			inputstring += inchar;
		}
	}
}



/* Main */

void setup(void)
{
	mpeser.begin();
	mpeser.startAnnounce(sketch, version);
	serialFlush();
}

void loop(void)
{
#if SERIAL && DEBUG
	pos++;
	Serial.print('.');
	if (pos > MAXLENLINE) {
		pos = 0;
		Serial.println();
	}
	serialFlush();
#endif

	delay(15000);
}
