

/* *** Globals and sketch configuration *** */
#define SERIAL          1 /* Enable serial */
#define DEBUG           1 /* Enable trace statements */

#define _NRF24          1

#define CONFIG_VERSION "ln2"
#define CONFIG_EEPROM_START 0
#define NRF24_CHANNEL   90



#include <EEPROM.h>
#include <JeeLib.h>
#include <SPI.h>
#include <RF24.h>
#include <RF24Network.h>
#include <DotmpeLib.h>
#include <mpelib.h>
#include "printf.h"



const String sketch = "RadioLink24";
const int version = 0;

char node[] = "rl24";

// Address of our node
const uint16_t this_node = 0;




/* IO pins */
//              RXD      0
//              TXD      1
//              INT0     2     UI IRQ
#if _NRF24
#       define CE       9  // NRF24
#       define CS       10 // NRF24
#endif




MpeSerial mpeser (57600);


/* *** InputParser *** {{{ */

Stream& cmdIo = Serial;
extern InputParser::Commands cmdTab[];
InputParser parser (50, cmdTab);


/* *** /InputParser }}} *** */

/* *** Report variables *** {{{ */


static byte reportCount;    // count up until next report, i.e. packet send

/* Data reported by this sketch */
struct {
	int rhum    :7;  // 0..100 (avg'd)
	int temp    :10; // -500..+500 (int value of tenths avg)
	int ctemp       :10; // atmega temperature: -500..+500 (tenths)
} payload;


/* *** /Report variables }}} *** */

/* *** Peripheral devices *** {{{ */

#if _NRF24
/* nRF24L01+: nordic 2.4Ghz digital radio  */

RF24 radio(CE, CS);

// Network uses that radio
RF24NetworkDebug network(radio);


#endif // NRF24


/* *** /Peripheral devices }}} *** */

/* *** Peripheral hardware routines *** {{{ */


#if _RFM12B
/* HopeRF RFM12B 868Mhz digital radio */

#endif //_RFM12B


#if _NRF24
/* Nordic nRF24L01+ radio routines */

void rf24_init()
{
	SPI.begin();
	radio.begin();
	network.begin( NRF24_CHANNEL, 0 );
}



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
	network.begin( NRF24_CHANNEL, 0 );
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
	//rf24_init();
#endif //_NRF24

	//scheduler.timer(MEASURE, 0);    // get the measurement loop going
}


/* *** /Run-time handlers }}} *** */

/* *** InputParser handlers *** {{{ */

#if SERIAL

InputParser::Commands cmdTab[] = {
	{ 0 }
};

#endif // SERIAL


/* *** /InputParser handlers *** }}} */

/* *** Main *** {{{ */


void setup(void)
{
#if SERIAL
	mpeser.begin();
	mpeser.startAnnounce(sketch, String(version));
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
	//debug_ticks();

	// Is there anything ready for us?
	while ( network.available() )
	{
    RF24NetworkHeader header;
    //network.peek(header);
    //printf_P(PSTR("%lu: APP Received #%u type %c from 0%o\n\r"),millis(),header.id,header.type,header.from_node);
    //Serial.println();

		// If so, grab it and print it out
		network.read(header, &payload, sizeof(payload));
		Serial.print("Received ");
		Serial.print(header.id);
		Serial.print(" ");
		Serial.print(header.from_node);
		Serial.print(" atmega temp: ");
		Serial.print(payload.ctemp);
		Serial.print(" rhum: ");
		Serial.println(payload.rhum);

	}
}

/* *** }}} */

