#include <DHT.h>
#include <JeeLib.h>
//include <OneWire.h>


// JeeLib DHT
#define DEBUG_DHT 1
#define DEBUG   0   // set to 1 to display each loop() run and PIR trigger

// DHT lib
#define DHTTYPE DHT11   // DHT 11 
//#define DHTTYPE DHT22   // DHT 22  (AM2302)

#define SERIAL  1   // set to 1 to enable serial interface
#define DHT_PIN     7   // defined if DHTxx data is connected to a DIO pin

#define SMOOTH          5   // smoothing factor used for running averages


#if DHT_PIN
DHTxx dht (DHT_PIN);
#endif

DHT dht2(DHT_PIN, DHTTYPE);

// has to be defined because we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

static void serialFlush () {
#if ARDUINO >= 100
	Serial.flush();
#endif  
	delay(2); // make sure tx buf is empty before going back to sleep
}

// utility code to perform simple smoothing as a running average
static int smoothedAverage(int prev, int next, byte firstTime =0) {
	if (firstTime)
		return next;
	return ((SMOOTH - 1) * prev + next + SMOOTH / 2) / SMOOTH;
}


struct {
	byte light ;     // light sensor: 0..255
	byte moved :1;  // motion detector: 0..1
	int rhum  :7;  // rhumdity: 0..100
	int temp   :10; // temperature: -500..+500 (tenths)
	int ctemp  :10; // atmega temperature: -500..+500 (tenths)
	byte lobat :1;  // supply voltage dropped under 3.1V: 0..1
} payload;

static void doMeasure() {
	byte firstTime = payload.rhum == 0; // special case to init running avg

#if DHT_PIN
	//int t, h;
	float h = dht2.readHumidity();
	float t = dht2.readTemperature();
	//if (dht.reading(t, h)) {
	//	Serial.print(h);
	//	Serial.print(" ");
	//	Serial.println(t);
		payload.rhum = smoothedAverage(payload.rhum, h * 10, firstTime);
		payload.temp = smoothedAverage(payload.temp, t * 10, firstTime);
	//}
#endif
}

void setup() {
#if SERIAL || DEBUG
	Serial.begin(57600);
	//Serial.begin(38400);
	Serial.println("DHT11Test");
	serialFlush();
#endif

  dht2.begin();
}

void loop() {
	doMeasure();

	float h = dht2.readHumidity();
	float t = dht2.readTemperature();
	// check if returns are valid, if they are NaN (not a number) then something went wrong!
	if (isnan(t) || isnan(h)) {
		Serial.println("Failed to read from DHT");
	} else {
		Serial.print("Humidity: "); 
		Serial.print(h);
		Serial.print(" %\t");
		Serial.print("Temperature: "); 
		Serial.print(t);
		Serial.println(" *C");
	}

	serialFlush();
	Sleepy::loseSomeTime(500);
}
