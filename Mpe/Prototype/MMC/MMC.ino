/* Atmega MMC/SD card routines */


/** Globals and sketch configuration  */
#define DEBUG           1 /* Enable trace statements */
#define SERIAL          1 /* Enable serial */

#define _MEM            1
#define _BAT            1
#define _DHT            1


static String sketch = "X-MMC";
static String vesion = "0";


int freeRam () {
	extern int __heap_start, *__brkval; 
	int v;
	return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
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


String serialBuffer = "";         // a string to hold incoming data

void serialEvent() {
	while (Serial.available()) {
		// get the new byte:
		char inchar = (char)Serial.read(); 
		// add it to the serialBuffer:
		if (inchar == '\n') {
			serialBuffer = "";
		} 
		else if (serialBuffer == "new ") {
			node_id += inchar;
		} 
		else {
			serialBuffer += inchar;
		}
	}
}



/** Generic routines */

static void serialFlush () {
#if SERIAL
#if ARDUINO >= 100
	Serial.flush();
#endif
	delay(2); // make sure tx buf is empty before going back to sleep
#endif
}

static void doConfig(void)
{
}

void setup_peripherals()
{
}

static bool doAnnounce()
{
}

static bool doMeasure()
{
}

static void doReport(void)
{
}

static void runCommand()
{
}

/* Main */

void setup(void)
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
	setup_peripherals();
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
