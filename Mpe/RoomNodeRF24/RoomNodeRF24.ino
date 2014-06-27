/* 
 * nRF24L01+ testing
 *
 * Work in progress.
 *
 * - 
 */
#include <DotmpeLib.h>
#include <JeeLib.h>
#include <SPI.h>
#include <RF24.h>

#include "printf.h"


/** Globals and sketch configuration  */
#define DEBUG           1 /* Enable trace statements */
#define SERIAL          1 /* Enable serial */
							
#define MEASURE_PERIOD  600 // how often to measure, in tenths of seconds
#define REPORT_EVERY    5   // report every N measurement cycles
#define SMOOTH          5   // smoothing factor used for running averages
#define MAXLENLINE      79
#define SRAM_SIZE       0x800 // atmega328, for debugging
							
#define SHT11_PORT      0   // defined if SHT11 is connected to a port
#define LDR_PORT        0   // defined if LDR is connected to a port's AIO pin
#define PIR_PORT        0   // defined if PIR is connected to a port's DIO pin
							
#define _MEM            1   // Report free memory 
#define _RFM12B         0
#define _RFM12BLOBAT    0
#define _NRF24          1
							

String sketch = "RF24Test";
String version = "0";

String node_id = "rf24tst-1";

int tick = 0;
int pos = 0;

/* IO pins */
const byte ledPin = 13; // XXX shared with nrf24 SCK
#if _NRF24
const byte rf24_ce = 9;
const byte rf24_csn = 8;
#endif

MpeSerial mpeser (57600);


// The scheduler makes it easy to perform various tasks at various times:
enum { MEASURE, REPORT, STDBY };

// Scheduler.pollWaiting returns -1 or -2
static const char WAITING = 0xFF; // -1: waiting to run
static const char IDLE = 0xFE; // -2: no tasks running

static word schedbuf[STDBY];
Scheduler scheduler (schedbuf, STDBY);

// has to be defined because we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

// Other variables used in various places in the code:
#if SHT11_PORT
SHT11 sht11 (SHT11_PORT);
#endif

#if LDR_PORT
Port ldr (LDR_PORT);
#endif

#if _NRF24
/* nRF24L01+: nordic 2.4Ghz digital radio  */

// Set up nRF24L01 radio on SPI bus plus two extra pins
RF24 radio(rf24_ce, rf24_csn); /* CE, CSN */

// nRF24L01 addresses: one for broadcast, one for listening
const uint64_t pipes[2] = { 
	0xF0F0F0F0E1LL, /* dest id: central link node */
	0xF0F0F0F0D2LL /* src id: local node */
};
#endif //_NRF24


/* Report variables */

static byte reportCount;    // count up until next report, i.e. packet send
//static byte myNodeID;       // node ID used for this unit

// This defines the structure of the packets which get sent out by wireless:

struct {
	int ctemp       :10; // atmega temperature: -500..+500 (tenths)
#if _MEM
	int memfree     :16;
#endif
#if _RFM12BLOBAT
	byte lobat :1;  // supply voltage dropped under 3.1V: 0..1
#endif
} payload;


/** AVR routines */

int freeRam () {
	extern int __heap_start, *__brkval; 
	int v;
	return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

int usedRam () {
	return SRAM_SIZE - freeRam();
}


/** ATmega routines */

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


/** Generic routines */

static void serialFlush () {
#if SERIAL
#if ARDUINO >= 100
	Serial.flush();
#endif
	delay(2); // make sure tx buf is empty before going back to sleep
#endif
}

void blink(int led, int count, int length, int length_off=0) {
	for (int i=0;i<count;i++) {
		digitalWrite (led, HIGH);
		delay(length);
		digitalWrite (led, LOW);
		delay(length);
		(length_off > 0) ? delay(length_off) : delay(length);
	}
}

void debug_ticks(void)
{
#if DEBUG
	tick++;
	// a bit less for non-waiting loops or always..?
	if ((tick % 20) == 0) {
		blink(ledPin, 1, 15);
	}
#if SERIAL
	if ((tick % 20) == 0) {
		Serial.print('.');
		pos++;
	}
	if (pos > MAXLENLINE) {
		pos = 0;
		Serial.println();
	}
#endif
#endif
}

// utility code to perform simple smoothing as a running average
static int smoothedAverage(int prev, int next, byte firstTime =0) {
	if (firstTime)
		return next;
	return ((SMOOTH - 1) * prev + next + SMOOTH / 2) / SMOOTH;
}

void debug(String msg) {
#if DEBUG
	Serial.println(msg);
#endif
}


#if PIR_PORT

#endif // PIR_PORT

#if _RFM12B
/* HopeRF RFM12B 868Mhz digital radio */

#endif //_RFM12B

#if _NRF24
/* Nordic nRF24L01+ routines */

void rf24_init()
{
	/* Setup and configure rf radio */
	radio.setRetries(15,15); /* delay, number */
	radio.setDataRate(RF24_2MBPS);
	/* Start radio */
	radio.openWritingPipe(pipes[0]);
	radio.openReadingPipe(1,pipes[1]);
	radio.startListening();
#if SERIAL && DEBUG
	radio.printDetails();
	serialFlush();
#endif
}

void rf24_run()
{
	if (radio.available()) {
		unsigned long got_time;
		bool done = false;
		while (!done)
		{
			done = radio.read( &got_time, sizeof(unsigned long) );
			if (!done) {
#if SERIAL && DEBUG
				printf("Payload read failed %i...", done);
#endif
				return;
			}

#if SERIAL && DEBUG
			printf("Got payload %lu...", got_time);
#endif
			//delay(10);
			radio.stopListening();
			radio.write( &got_time, sizeof(unsigned long) );
#if SERIAL && DEBUG
			printf("Sent response.\n\r");
#endif
			radio.startListening();
		}
	}
}
#endif //_NRF24


/* Initialization routines */

void doConfig(void)
{
}

void initLibs()
{
#if _NRF24
	radio.begin();
#endif //_NRF24

#if SERIAL && DEBUG
	printf_begin();
#endif
}


/* Run-time handlers */

bool doAnnounce()
{
#if SERIAL
	Serial.print(node_id);
	Serial.print(" ");
	Serial.print(F(" ctemp"));
#endif
#if _MEM
#if SERIAL
	Serial.print(F(" memfree"));
#endif
#endif
}

void doReset(void)
{
	tick = 0;

#if _NRF24
	rf24_init();
#endif //_NRF24

	reportCount = REPORT_EVERY;     // report right away for easy debugging
	scheduler.timer(MEASURE, 0);    // start the measurement loop going
}


// readout all the sensors and other values
void doMeasure()
{
	byte firstTime = payload.ctemp == 0; // special case to init running avg

	int ctemp = internalTemp();
	payload.ctemp = smoothedAverage(payload.ctemp, ctemp, firstTime);
#if SERIAL && DEBUG_MEASURE
	Serial.println();
	Serial.print("AVR T new/avg ");
	Serial.print(ctemp);
	Serial.print(' ');
	Serial.println(payload.ctemp);
#endif

#if _MEM
	payload.memfree = freeRam();
#if SERIAL && DEBUG_MEASURE
	Serial.print("MEM free ");
	Serial.println(payload.memfree);
#endif
#endif

#if _RFM12BLOBAT
	payload.lobat = rf12_lowbat();
#endif
}

// periodic report, i.e. send out a packet and optionally report on serial port
bool doReport(void)
{
	bool ok;

#if _RFM12B
	rf12_sleep(RF12_WAKEUP);
	while (!rf12_canSend())
		rf12_recvDone();
	rf12_sendStart(0, &payload, sizeof payload, RADIO_SYNC_MODE);
	rf12_sleep(RF12_SLEEP);
#endif

#if _NRF24
	//rf24_run();
	ok = radio.write( &payload, sizeof payload );
#endif //_NRF24

#if SERIAL
	/* Report over serial, same fields and order as announced */
	Serial.print(node_id);
	Serial.print(" ");
	Serial.print((int) payload.ctemp);
	Serial.print(' ');
#if _MEM
	Serial.print((int) payload.memfree);
	Serial.print(' ');
#endif
	Serial.println();
#endif // SERIAL || DEBUG

	return ok;
}

void runScheduler(char task)
{
	switch (task) {

		case MEASURE:
			// reschedule these measurements periodically
			debug("MEASURE");
			scheduler.timer(MEASURE, MEASURE_PERIOD);

			doMeasure();

			// every so often, a report needs to be sent out
			if (++reportCount >= REPORT_EVERY) {
				reportCount = 0;
				scheduler.timer(REPORT, 0);
			}
			serialFlush();
			break;

		case REPORT:
			debug("REPORT");
//			payload.msgtype = REPORT_MSG;
			if (doReport()) {
				// XXX report again?
			}
			serialFlush();
			break;

#if DEBUG
		default:
			Serial.print("0x");
			Serial.print(task, HEX);
			Serial.println(" ?");
			serialFlush();
			break;
#endif

	}
}


#if PIR_PORT

// send packet and wait for ack when there is a motion trigger
void doTrigger()
{
}
#endif // PIR_PORT



/* Main */

void setup(void)
{
#if SERIAL
	mpeser.begin();
	mpeser.startAnnounce(sketch, version);
	doAnnounce();
	serialFlush();

#if DEBUG
	Serial.print(F("SRAM used: "));
	Serial.println(usedRam());
#endif
#endif

	initLibs();

	doReset();
}

void loop(void)
{
	debug_ticks();

	char task = scheduler.pollWaiting();
	serialFlush();
	if (task == 0xFF) return; // -1
	if (task == 0xFE) return; // -2
	runScheduler(task);
}
