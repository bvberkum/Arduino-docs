

/* *** Globals and sketch configuration *** */
#define SERIAL          1 /* Enable serial */
#define DEBUG           1 /* Enable trace statements */
							

#include <SPI.h>
#include <RF24.h>
#include <RF24Network.h>
#include <DotmpeLib.h>
#include <mpelib.h>



const String sketch = "RadioLink24";
const int version = 0;

char node[] = "rl24";


/* IO pins */
//              RXD      0
//              TXD      1
//              INT0     2     UI IRQ
#if _NRF24
#       define CE       9  // NRF24
#       define CS       10 // NRF24
#endif

// Address of our node
const uint16_t this_node = 0;



/* *** Peripheral devices *** {{{ */

#if _NRF24
/* nRF24L01+: nordic 2.4Ghz digital radio  */

RF24 radio(CE, CS);

// Network uses that radio
RF24Network/*Debug*/ network(radio);


#endif // NRF24


/* *** /Peripheral devices }}} *** */

/* *** Peripheral hardware routines *** {{{ */


#if _RFM12B
/* HopeRF RFM12B 868Mhz digital radio */

#endif //_RFM12B


#if _NRF24
/* Nordic nRF24L01+ radio routines */


#endif // NRF24 funcs



/* *** /Peripheral hardware routines }}} *** */

/* *** UI *** {{{ */

/* *** /UI }}} *** */

/* *** Initialization routines *** {{{ */

void doConfig(void)
{
}

void initLibs()
{
#if _NRF24
	SPI.begin();
	radio.begin();
	network.begin(/*channel*/ 90, /*node address*/ this_node);
#endif // NRF24
}


/* *** /Initialization routines *** }}} */

/* *** Run-time handlers *** {{{ */

void doReset(void)
{
	doConfig();

#ifdef _DBG_LED
	pinMode(_DBG_LED, OUTPUT);
#endif
	tick = 0;

#if _NRF24
	rf24_init();
#endif //_NRF24

	reportCount = REPORT_EVERY;     // report right away for easy debugging
	scheduler.timer(MEASURE, 0);    // get the measurement loop going
}


/* *** /Run-time handlers }}} *** */

/* *** Main *** {{{ */


void setup(void)
{
#if SERIAL
	mpeser.begin();
	mpeser.startAnnounce(sketch, version);
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
#ifdef _DBG_LED
	blink(_DBG_LED, 1, 15);
#endif
#if _NRF24
	// Pump the network regularly
	network.update();
#endif
	debug_ticks();

	// Is there anything ready for us?
	while ( network.available() )
	{
		// If so, grab it and print it out
		RF24NetworkHeader header;
		payload_t payload;
		network.read(header, &payload, sizeof(payload));
		Serial.print("Received ");
		Serial.print(header.id);
		Serial.print(" ");
		Serial.print(header.from_node);
		Serial.print(" atmega temp: ");
		Serial.print(payload.ctemp);
		Serial.print(" memfree: ");
		Serial.println(payload.memfree);
	}
}

/* }}} *** */

