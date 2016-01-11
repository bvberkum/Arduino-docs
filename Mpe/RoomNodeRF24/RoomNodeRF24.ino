/*

RoomNodeRF24

 */


/* *** Globals and sketch configuration *** */
#ifndef SERIAL
#define SERIAL          1 /* Enable serial */
#endif
#define DEBUG           1 /* Enable trace statements */

#define _MEM            1   // Report free memory
#define _RFM12B         0
#define _NRF24          1
#define SHT11_PORT      0   // defined if SHT11 is connected to a port
#define PIR_PORT        0   // defined if PIR is connected to a port's DIO pin
#define LDR_PORT        0   // defined if LDR is connected to a port's AIO pin

#define REPORT_EVERY    5   // report every N measurement cycles
#define SMOOTH          5   // smoothing factor used for running averages
#define MEASURE_PERIOD  600 // how often to measure, in tenths of seconds



#include <SPI.h>
#include <JeeLib.h>
#include <RF24.h>
#include <RF24Network.h>
#include <DotmpeLib.h>
#include <mpelib.h>

#include "printf.h"



const String sketch = "RoomNodeRF24";
const int version = 0;

char node[] = "rn24";

String node_id = "rn24-1";



/* IO pins */
static const byte rf24_ce  = 9;
static const byte rf24_csn = 10;
#       define rf24_ce       9  // NRF24
#       define rf24_cs       10 // NRF24
//define _DEBUG_LED 13


MpeSerial mpeser (57600);


/* *** Report variables *** {{{ */


static byte reportCount;    // count up until next report, i.e. packet send

// This defines the structure of the packets which get sent out by wireless:
struct {
	int ctemp       :10; // atmega temperature: -500..+500 (tenths)
#if _MEM
	int memfree     :16;
#endif
} payload;


/* *** /Report variables *** }}} */

/* *** Scheduled tasks *** {{{ */

enum {
	MEASURE,
	REPORT,
	TASK_END
};
// Scheduler.pollWaiting returns -1 or -2
static const char SCHED_WAITING = 0xFF; // -1: waiting to run
static const char SCHED_IDLE = 0xFE; // -2: no tasks running

static word schedbuf[TASK_END];
Scheduler scheduler (schedbuf, TASK_END);

// has to be defined because we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }


/* *** /Scheduled tasks *** }}} */

/* *** EEPROM config *** {{{ */

/* *** /EEPROM config *** }}} */

/* *** Peripheral devices *** {{{ */

#if SHT11_PORT
SHT11 sht11 (SHT11_PORT);
#endif


#if LDR_PORT
Port ldr (LDR_PORT);
#endif

#if _DHT
/* DHT temp/rh sensor
 - AdafruitDHT
*/
#endif // DHT

#if _LCD84x48
/* Nokkia 5110 display */
#endif // LCD84x48

#if _DS
/* Dallas OneWire bus with registration for DS18B20 temperature sensors */



#endif // DS

#if _NRF24
/* nRF24L01+: nordic 2.4Ghz digital radio  */

// Set up nRF24L01 radio on SPI bus plus two extra pins
RF24 radio(rf24_ce, rf24_cs);

// Network uses that radio
RF24Network network(radio);

// nRF24L01 addresses: one for broadcast, one for listening
const uint64_t pipes[2] = {
	0xF0F0F0F0E1LL, /* dest id: central link node */
	0xF0F0F0F0D2LL /* src id: local node */
};

#endif // NRF24


/* *** /Peripheral devices *** }}} */


/* *** Peripheral hardware routines *** {{{ */


#if LDR_PORT
#endif

/* *** PIR support *** {{{ */
#if PIR_PORT
#endif // PIR_PORT
/* *** /PIR support *** }}} */


#if _DHT
/* DHT temp/rh sensor
 - AdafruitDHT
*/

#endif // DHT

#if _LCD84x48


#endif // LCD84x48

#if _DS
/* Dallas DS18B20 thermometer routines */


#endif // DS

#if _RFM12B
/* HopeRF RFM12B 868Mhz digital radio routines */

#endif // RFM12B

#if _NRF24
/* Nordic nRF24L01+ radio routines */

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

#endif // NRF24 funcs

#if _RTC
#endif // RTC

#if _HMC5883L
/* Digital magnetometer I2C routines */


#endif // HMC5883L


/* *** /Peripheral hardware routines }}} *** */


/* *** Initialization routines *** {{{ */

void doConfig(void)
{
}

void initLibs()
{
#if _NRF24
	radio.begin();
#endif // NRF24

#if SERIAL && DEBUG
	printf_begin();
#endif
}

/* *** /Initialization routines *** }}} */


/* *** Run-time handlers *** {{{ */

void doReset(void)
{
	tick = 0;

#if _NRF24
	rf24_init();
#endif //_NRF24

	reportCount = REPORT_EVERY;     // report right away for easy debugging
	scheduler.timer(MEASURE, 0);    // get the measurement loop going
}

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
	return false;
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
			debugline("MEASURE");
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
			debugline("REPORT");
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



/* *** /Run-time handlers *** }}} */

/* *** Main *** {{{ */


void setup(void)
{
#if SERIAL
	mpeser.begin();
	mpeser.startAnnounce(sketch, String(version));
	doAnnounce();
#if DEBUG || _MEM
	Serial.print(F("Free RAM: "));
	Serial.println(freeRam());
#endif
	serialFlush();
#endif

	initLibs();

	doReset();
}

void loop(void)
{
#if _NRF24
	// Pump the network regularly
	network.update();
#endif
	debug_ticks();
	serialFlush();
	char task = scheduler.pollWaiting();
	debugline("Wakeup");
	if (-1 < task && task < SCHED_IDLE) {
		runScheduler(task);
	}
}

/* }}} *** */

