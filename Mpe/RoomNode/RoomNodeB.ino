// Mpe

#include <JeeLib.h>
#include <PortsSHT11.h>
#include <avr/sleep.h>
#include <util/atomic.h>
#include <OneWire.h>


#define SERIAL  1   // set to 1 to also report readings on the serial port
#define DEBUG   1   // set to 1 to display each loop() run and PIR trigger

#define LDR_PORT    4   // defined if LDR is connected to a port's AIO pin
#define PIR_PORT    4   // defined if PIR is connected to a port's DIO pin

#define MEASURE_PERIOD  600 // how often to measure, in tenths of seconds
#define RETRY_PERIOD    10  // how soon to retry if ACK didn't come in
#define RETRY_LIMIT     5   // maximum number of times to retry
#define ACK_TIME        10  // number of milliseconds to wait for an ack
#define REPORT_EVERY    5   // report every N measurement cycles
#define SMOOTH          3   // smoothing factor used for running averages

// set the sync mode to 2 if the fuses are still the Arduino default
// mode 3 (full powerdown) can only be used with 258 CK startup fuses
#define RADIO_SYNC_MODE 2


// The scheduler makes it easy to perform various tasks at various times:

enum { MEASURE, REPORT, TASK_END };

static word schedbuf[TASK_END];
Scheduler scheduler (schedbuf, TASK_END);

// Other variables used in various places in the code:

static byte reportCount;    // count up until next report, i.e. packet send
static byte myNodeID;       // node ID used for this unit

// This defines the structure of the packets which get sent out by wireless:

struct {
	byte light;     // light sensor: 0..255
	byte moved :1;  // motion detector: 0..1
	byte humi  :7;  // humidity: 0..100
	int temp   :10; // temperature: -500..+500 (tenths)
	byte lobat :1;  // supply voltage dropped under 3.1V: 0..1
} payload;

// Conditional code, depending on which sensors are connected and how:

Port ldr (LDR_PORT);

OneWire  ds(4);  // Dallas Semiconductor One-Wire on pin 4

byte sensor1[8] = {0x28, 0x82, 0x27, 0xDD, 0x03, 0x00, 0x00, 0x4B};
byte sensor2[8] = {0x28, 0x8A, 0x5B, 0xDD, 0x03, 0x00, 0x00, 0xA6};


#define PIR_HOLD_TIME   30  // hold PIR value this many seconds after change
#define PIR_PULLUP      1   // set to one to pull-up the PIR input pin
#define PIR_FLIP        0   // 0 or 1, to match PIR reporting high or low

class PIR : public Port {
	volatile byte value, changed;
	volatile uint32_t lastOn;
	public:
	PIR (byte portnum)
		: Port (portnum), value (0), changed (0), lastOn (0) {}

	// this code is called from the pin-change interrupt handler
	void poll() {
		// see http://talk.jeelabs.net/topic/811#post-4734 for PIR_FLIP
		byte pin = digiRead() ^ PIR_FLIP;
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
					f = 1 ^ PIR_FLIP;
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

// the PIR signal comes in via a pin-change interrupt
ISR(PCINT2_vect) { pir.poll(); }

// has to be defined because we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

// utility code to perform simple smoothing as a running average
static int smoothedAverage(int prev, int next, byte firstTime =0) {
	if (firstTime)
		return next;
	return ((SMOOTH - 1) * prev + next + SMOOTH / 2) / SMOOTH;
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

	//dsread(sensor1);
	//dsread(sensor2);

	byte firstTime = payload.humi == 0; // special case to init running avg

	payload.lobat = rf12_lowbat();

#if SHT11_PORT
#ifndef __AVR_ATtiny84__
	sht11.measure(SHT11::HUMI, shtDelay);        
	sht11.measure(SHT11::TEMP, shtDelay);
	float h, t;
	sht11.calculate(h, t);
	int humi = h + 0.5, temp = 10 * t + 0.5;
#else
	//XXX TINY!
	int humi = 50, temp = 25;
#endif
	payload.humi = smoothedAverage(payload.humi, humi, firstTime);
	payload.temp = smoothedAverage(payload.temp, temp, firstTime);
#endif
#if LDR_PORT
	ldr.digiWrite2(1);  // enable AIO pull-up
	byte light = ~ ldr.anaRead() >> 2;
	ldr.digiWrite2(0);  // disable pull-up to reduce current draw
	payload.light = smoothedAverage(payload.light, light, firstTime);
#endif
#if PIR_PORT
	payload.moved = pir.state();
#endif
}

static void serialFlush () {
#if ARDUINO >= 100
	Serial.flush();
#endif  
	delay(2); // make sure tx buf is empty before going back to sleep
}

// periodic report, i.e. send out a packet and optionally report on serial port
static void doReport() {
	rf12_sleep(RF12_WAKEUP);
	while (!rf12_canSend())
		rf12_recvDone();
	rf12_sendStart(0, &payload, sizeof payload, RADIO_SYNC_MODE);
	rf12_sleep(RF12_SLEEP);

#if SERIAL
	Serial.print("ROOM ");
	Serial.print((int) payload.light);
	Serial.print(' ');
	Serial.print((int) payload.moved);
	Serial.print(' ');
	Serial.print((int) payload.humi);
	Serial.print(' ');
	Serial.print((int) payload.temp);
	Serial.print(' ');
	Serial.print((int) payload.lobat);
	Serial.println();
	serialFlush();
#endif
}

// send packet and wait for ack when there is a motion trigger
static void doTrigger() {
#if DEBUG
	Serial.print("PIR ");
	Serial.print((int) payload.moved);
	serialFlush();
#endif
	for (byte i = 0; i < RETRY_LIMIT; ++i) {
		rf12_sleep(RF12_WAKEUP);
		while (!rf12_canSend())
			rf12_recvDone();
		rf12_sendStart(RF12_HDR_ACK, &payload, sizeof payload, RADIO_SYNC_MODE);
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

void dsread(byte* addr)
{
	byte i;
	byte present = 0;
	byte data[12];
	byte type_s = 0;
	float celsius, fahrenheit;

	ds.reset();
	ds.select(addr);
	ds.write(0x44,1);         // start conversion, with parasite power on at the end

	delay(1000);     // maybe 750ms is enough, maybe not
	// we might do a ds.depower() here, but the reset will take care of it.

	present = ds.reset();
	ds.select(addr);    
	ds.write(0xBE);         // Read Scratchpad

#if DEBUG
	Serial.print("  Data = ");
	Serial.print(present,HEX);
	Serial.print(" ");
#endif
	for ( i = 0; i < 9; i++) {           // we need 9 bytes
		data[i] = ds.read();
#if DEBUG
		Serial.print(data[i], HEX);
		Serial.print(" ");
#endif
	}
#if DEBUG
	Serial.print(" CRC=");
	Serial.print(OneWire::crc8(data, 8), HEX);
	Serial.println();
#endif

	// convert the data to actual temperature

	unsigned int raw = (data[1] << 8) | data[0];
	if (type_s) {
		raw = raw << 3; // 9 bit resolution default
		if (data[7] == 0x10) {
			// count remain gives full 12 bit resolution
			raw = (raw & 0xFFF0) + 12 - data[6];
		}
	} else {
		byte cfg = (data[4] & 0x60);
		if (cfg == 0x00) raw = raw << 3;  // 9 bit resolution, 93.75 ms
		else if (cfg == 0x20) raw = raw << 2; // 10 bit res, 187.5 ms
		else if (cfg == 0x40) raw = raw << 1; // 11 bit res, 375 ms
		// default is 12 bit resolution, 750 ms conversion time
	}
	celsius = (float)raw / 16.0;
	fahrenheit = celsius * 1.8 + 32.0;
#if DEBUG
	Serial.print("  Temperature = ");
	Serial.print(celsius);
	Serial.print(" Celsius, ");
	Serial.print(fahrenheit);
	Serial.println(" Fahrenheit");
	serialFlush();
#endif
}

void blink (byte pin) {
	for (byte i = 0; i < 6; ++i) {
		delay(100);
		digitalWrite(pin, !digitalRead(pin));
	}
}

void setup () {
#if SERIAL || DEBUG
	Serial.begin(57600);
	Serial.print("\n[roomNode.3]");
	myNodeID = rf12_config();
	serialFlush();
#else
	myNodeID = rf12_config(0); // don't report info on the serial port
#endif

	rf12_sleep(RF12_SLEEP); // power down

	pir.digiWrite(PIR_PULLUP);
#ifdef PCMSK2
	bitSet(PCMSK2, PIR_PORT + 3);
	bitSet(PCICR, PCIE2);
#else
	//XXX TINY!
#endif

	reportCount = REPORT_EVERY;     // report right away for easy debugging
	scheduler.timer(MEASURE, 0);    // start the measurement loop going
}

void loop(void) {

#if DEBUG
	Serial.print('.');
	serialFlush();
#endif

#if PIR_PORT
	if (pir.triggered()) {
		payload.moved = pir.state();
		doTrigger();
	}
#endif

	switch (scheduler.pollWaiting()) {

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

