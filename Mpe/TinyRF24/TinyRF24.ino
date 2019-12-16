/**

Created: 2016-01-23

Got blink working and new makefile set up. This is not tested yet.
Should setup receiver node and see.

**/


/* *** Globals and sketch configuration *** */
#define _MEM            1   // Report free memory
#define _DHT            1
#define DHT_HIGH        0   // enable for DHT22/AM2302, low for DHT11
#define _DS             0
#define _LCD            0
#define _LCD84x48       1
#define _NRF24          1

#define REPORT_EVERY    2   // report every N measurement cycles
#define SMOOTH          5   // smoothing factor used for running averages
#define MEASURE_PERIOD  30  // how often to measure, in tenths of seconds
#define UI_SCHED_IDLE         4000  // tenths of seconds idle time, ...
#define UI_STDBY        8000  // ms
#define BL_INVERTED     0xFF
#define NRF24_CHANNEL   90
#define MAXLENLINE      79
#define ANNOUNCE_START  0




//#include <EEPROM.h>
#include <SPI.h>
#include <RF24.h>
#include <RF24Network.h>
//#include <DotmpeLib.h>
#include <mpelib.h>


/* IO pins */
#       define _DBG_LED 1
#       define CSN      8  // NRF24
#       define CE       9  // NRF24


/* *** Report variables *** {{{ */


static byte reportCount;    // count up until next report, i.e. packet send

/* Data reported by this sketch */
struct {
	int ctemp       :10; // atmega temperature: -500..+500 (tenths)
#if _MEM
	int memfree     :16;
#endif
} payload;


/* *** /Report variables *** }}} */

/* *** Peripheral devices *** {{{ */


#if _NRF24
/* nRF24L01+: nordic 2.4Ghz digital radio  */

// Set up nRF24L01 radio on SPI bus plus two extra pins
RF24 radio(CE, CSN);

// Network uses that radio
RF24Network/*Debug*/ network(radio);

// Address of our node
const uint16_t this_node = 1;

// Address of the other node
const uint16_t rf24_link_node = 0;

#endif // NRF24


/* *** /Peripheral devices *** }}} */

/* *** Initialization routines *** {{{ */

void initLibs()
{
}


/* *** /Initialization routines *** }}} */

/* *** Run-time handlers *** {{{ */

void doReset(void)
{
	//doConfig();

#ifdef _DBG_LED
	pinMode(_DBG_LED, OUTPUT);
#endif
	tick = 0;

	reportCount = REPORT_EVERY;     // report right away for easy debugging
}

bool doAnnounce(void)
{
/* see CarrierCase */
	return false;
}


// readout all the sensors and other values
void doMeasure(void)
{
	byte firstTime = payload.ctemp == 0; // special case to init running avg

	int ctemp = internalTemp();
	payload.ctemp = smoothedAverage(payload.ctemp, ctemp, firstTime);

#if _MEM
	payload.memfree = freeRam();
#endif //_MEM
}


// periodic report, i.e. send out a packet and optionally report on serial port
void doReport(void)
{
#if _RFM12B
#endif //_RFM12B

#if _NRF24
	RF24NetworkHeader header(/*to node*/ rf24_link_node);
	bool ok = network.write(header, &payload, sizeof(payload));
#endif // NRF24
}


/* *** /Run-time handlers *** }}} */

/* *** Main *** {{{ */


void setup(void)
{
	initLibs();

	doReset();
}

void loop(void)
{
#ifdef _DBG_LED
	blink(_DBG_LED, 1, 15);
#endif
#if _NRF24
	// Pump the network regularly
	network.update();
#endif

#ifdef _DBG_LED
		blink(_DBG_LED, 1, 25);
#endif
}

/* *** }}} */

