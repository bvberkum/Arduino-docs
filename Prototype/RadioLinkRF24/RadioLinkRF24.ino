/**

TODO: central node, with USB-UART bridge to processing host
(Relay role can be in leaf sketches.)

- Quickest start is probably hardcoding the structs per sketch type.
  And challenging node for config (announce) but using only sketch Id.
  Ie. keeping run-time map of node address to sketch id.

  That or don't bother at all, and parse at host. Matador has some Python code.
  But if it can be parsed here why not.
  Python could exclude embedded devices, ie. an OpenWRT router.

- More elaborated is to a). find a way to write the configs (EEPROM, or SD)
  and b). write routines to parse acc. to c struct. (or can dynamically define
  struct type?)

  And maybe add SD logging, for Relay Nodes (with no or minimal on-board
  sensors).

TODO: store node_id (3+2 bytes + null=6 byte) with adress (uint16_t=2 byte)
on announce.

XXX: Load ids into array on start-up for quick lookup of address.


TODO: map alphachar node_id prefix to sketch payload struct.

*/

/* *** Globals and sketch configuration *** */
#define SERIAL          1 /* Enable serial */
#define DEBUG           1 /* Enable trace statements */

#define _NRF24          1

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
#include <hashmap.h>



// Name Id for this sketch
const String sketch = "RadioLink24";

// Integer version for this sketch
const int version = 0;

// Alpha Id for this sketch
char node[] = "rln";

// Alphanumeric Id suffixed with integer
char node_id[4];

// Network address of current node
const uint16_t link_node = 0;






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

/* *** EEPROM config *** {{{ */



struct Config {
	char node[3]; // Alphanumeric Id (for sketch)
	int node_id; // Id suffix integer (for unique run-time Id)
	int version; // Sketch version
	signed int temp_offset;
	float temp_k;
	uint16_t address; // RF24Network address
	uint16_t nodes; // Node count
} static_config = {
	/* default values */
	{ node[0], node[1], node[2] }, 0, version,
	TEMP_OFFSET, TEMP_K,
	00, 00
};

Config config;

struct Record {
	char node[3];
	int node_id;
	uint16_t address;
};

bool loadConfig(Config &c)
{
	unsigned int w = sizeof(c);

	if (
			EEPROM.read(CONFIG_EEPROM_START + w - 1) == node_id[4] &&
			EEPROM.read(CONFIG_EEPROM_START + w - 2) == node_id[3] &&
			EEPROM.read(CONFIG_EEPROM_START + w - 3) == node_id[2] &&
			EEPROM.read(CONFIG_EEPROM_START + w - 4) == node_id[1] &&
			EEPROM.read(CONFIG_EEPROM_START + w - 5) == node_id[0]
	) {

		for (unsigned int t=0; t<w; t++)
		{
			*((char*)&c + t) = EEPROM.read(CONFIG_EEPROM_START + t);
		}
		return true;

	} else {
#if SERIAL && DEBUG
		Serial.println("No valid data in eeprom");
#endif
		return false;
	}
}

void writeConfig(Config &c)
{
	for (unsigned int t=0; t<sizeof(c); t++) {

		EEPROM.write(CONFIG_EEPROM_START + t, *((char*)&c + t));

		// verify
		if (EEPROM.read(CONFIG_EEPROM_START + t) != *((char*)&c + t))
		{
			// error writing to EEPROM
#if SERIAL && DEBUG
			Serial.println("Error writing "+ String(t)+" to EEPROM");
#endif
		}
	}
}

Record* nodes;

bool nodeConfigured(uint16_t address)
{
	for (Record * r = nodes; ; ++r) {
		if (r->address == address)
			return true;
	}
	return false;
}


/* *** /EEPROM config *** }}} */

/* *** Peripheral devices *** {{{ */

#if _NRF24
/* nRF24L01+: nordic 2.4Ghz digital radio  */

RF24 radio(CE, CS);

#if DEBUG
RF24NetworkDebug network(radio);
#else
RF24Network network(radio);
#endif


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

void initConfig(void)
{
	sprintf(node_id, "%s%i", config.node, config.node_id);
}

void doConfig(void)
{
	/* load valid config or reset default config */
	if (!loadConfig(config)) {
		writeConfig(static_config);
	}
	initConfig();
}

void initLibs()
{
#if _NRF24
	SPI.begin();
	radio.begin();
	network.begin( NRF24_CHANNEL, config.address );
#endif // NRF24
}


/* *** /Initialization routines *** }}} */

/* *** Run-time handlers *** {{{ */

void loadNodeAddresses(void)
{
	Record record;
	unsigned int x;
	unsigned int y = sizeof(config);
	unsigned int w = sizeof(record);
	nodes = (Record*) malloc(config.nodes * w);
	for (unsigned int i=0; i<config.nodes; i++) {
		x = i * w;
		for (unsigned int t=0; t<w; t++)
		{
			*((char*)&record + t) = EEPROM.read(CONFIG_EEPROM_START + y + x + t);
			nodes[i] = record;
		}
	}
}

void doReset(void)
{
	doConfig();

	loadNodeAddresses();

#ifdef _DBG_LED
	pinMode(_DBG_LED, OUTPUT);
#endif
	tick = 0;

	//scheduler.timer(MEASURE, 0);    // get the measurement loop going
}

void nodeReportRead(RF24NetworkHeader header)
{
		header.type;
	network.read(header, &payload, sizeof(payload));
}

void registerNode(RF24NetworkHeader header)
{
	Record record;
	Record announce;
	// XXX: read bytes until announce header
	network.read(header, &announce, sizeof(announce));
	//record.node = announce.node;
	//record.node_id = announce.node_id;
	//record.address = announce.address;
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

	while ( network.available() )
	{
		RF24NetworkHeader header;

		network.peek(header);

#if DEBUG
		printf_P(PSTR("%lu: APP Received #%u type %c from 0%o\n\r"),
				millis(),
				header.id,
				header.type,
				header.from_node);
		//Serial.println();
#endif

		switch (header.type) {
			case 0:
				registerNode(header);
				break;
			case 1:
				if (nodeConfigured(header.from_node)) {
					nodeReportRead(header);
				} else {
		//		Serial.println("Discarded messsage, no node config");
		//		// TODO request node config
				}
				break;
		}

	}

	serialFlush();
}

/* *** }}} */

