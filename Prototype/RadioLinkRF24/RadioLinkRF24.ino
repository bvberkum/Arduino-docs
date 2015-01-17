

/* *** Globals and sketch configuration *** */
#define DEBUG           1 /* Enable trace statements */
#define SERIAL          1 /* Enable serial */
							

#include <SPI.h>
#include <RF24.h>
#include <RF24Network.h>
#include <DotmpeLib.h>
#include <mpelib.h>




const String sketch = "RadioLink24";
const int version = 0;

char node[] = "rl24";


/* IO pins */
#if _NRF24
#       define CE       9
#       define CS       10
#endif
#       define BAUD     57600

// Address of our node
const uint16_t this_node = 0;

// Address of the other node
//const uint16_t other_node = 12;


/* *** Peripheral devices *** {{{ */



/* *** /Peripheral devices }}} *** */


/* *** Peripheral hardware routines *** {{{ */

#if _NRF24
/* nRF24L01+: nordic 2.4Ghz digital radio  */

RF24 radio(CE, CS);

// Network uses that radio
RF24Network/*Debug*/ network(radio);


#endif // NRF24

/* *** Peripheral hardware routines *** {{{ */


#if _NRF24
/* Nordic nRF24L01+ radio routines */

#endif // RF24 funcs



/* *** /Peripheral hardware routines }}} *** */

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
	tick = 0;

	doConfig();

#if _NRF24
	rf24_init();
#endif //_NRF24

	reportCount = REPORT_EVERY;     // report right away for easy debugging
	scheduler.timer(MEASURE, 0);    // start the measurement loop going
}



/* *** /Run-time handlers *** }}} */



/* }}} *** */

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

