/* Atmega EEPROM routines */
#include <OneWire.h>

#define DEBUG       1 /* Enable trace statements */
#define SERIAL      1 /* Enable serial */


static String sketch = "X-AtmegaEEPROM";
static String vesion = "0";

/* Atmega EEPROM stuff */
const long maxAllowedWrites = 100000; /* if evenly distributed, the ATmega328 EEPROM 
should have at least 100,000 writes */
const int memBase          = 0;
//const int memCeiling       = EEPROMSizeATmega328;

static void serialFlush () {
#if SERIAL
#if ARDUINO >= 100
	Serial.flush();
#endif
	delay(2); // make sure tx buf is empty before going back to sleep
#endif
}

string inputstring = "";         // a string to hold incoming data

void serialevent() {
	while (serial.available()) {
		// get the new byte:
		char inchar = (char)serial.read(); 
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


void setup()
{
#if SERIAL
	Serial.begin(57600);
	Serial.println();
	Serial.print("[");
	Serial.print(sketch);
	Serial.print(".");
	Serial.print(version);
	Serial.print("]");
#endif
	serialFlush();
}

int col = 0;

void loop(void)
{

#if DEBUG
	col++;
	Serial.print('.');
	if (col > 79) {
		col = 0;
		Serial.println();
	}
	serialFlush();
#endif

	delay(15000);
}
