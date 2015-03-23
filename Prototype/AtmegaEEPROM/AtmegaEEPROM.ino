/* 
Atmega EEPROM routines */


/* *** Globals and sketch configuration *** */
#define SERIAL          1 /* Enable serial */
#define DEBUG           1 /* Enable trace statements */
							
#define MAXLENLINE      79
							
#include <OneWire.h>
#include <DotmpeLib.h>
#include <EEPROM.h>



const String sketch = "X-AtmegaEEPROM";
const int version = 0;

char node[] = "nx";
// determined upon handshake 
char node_id[7];

/* IO pins */
//              RXD      0
//              TXD      1
//              INT0     2
//              MOSI     11
//              MISO     12
//#       define _DBG_LED 13 // SCK


MpeSerial mpeser (57600);

/* *** InputParser *** {{{ */

extern InputParser::Commands cmdTab[] PROGMEM;
byte* buffer = (byte*) malloc(50);
InputParser parser (buffer, 50, cmdTab);


/* *** /InputParser }}} *** */

/* *** Report variables *** {{{ */

/* *** /Report variables }}} *** */

/* *** Scheduled tasks *** {{{ */

/* *** /Scheduled tasks }}} *** */

/* *** EEPROM config *** {{{ */


#define CONFIG_VERSION "nx1"
#define CONFIG_START 0

struct Config {
	char node[4];
	int node_id;
	int version;
	char config_id[4];
} static_config = {
	/* default values */
	{ node[0], node[1], }, 0, version, 
	CONFIG_VERSION
};

Config config;

bool loadConfig(Config &c) 
{
	unsigned int w = sizeof(c);

	if (
			EEPROM.read(CONFIG_START + w - 1) == c.config_id[3] &&
			EEPROM.read(CONFIG_START + w - 2) == c.config_id[2] &&
			EEPROM.read(CONFIG_START + w - 3) == c.config_id[1] &&
			EEPROM.read(CONFIG_START + w - 4) == c.config_id[0]
	) {

		for (unsigned int t=0; t<w; t++)
		{
			*((char*)&c + t) = EEPROM.read(CONFIG_START + t);
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
		
		EEPROM.write(CONFIG_START + t, *((char*)&c + t));

		// verify
		if (EEPROM.read(CONFIG_START + t) != *((char*)&c + t))
		{
			// error writing to EEPROM
#if SERIAL && DEBUG
			Serial.println("Error writing "+ String(t)+" to EEPROM");
#endif
		}
	}
}


/* *** /EEPROM config }}} *** */

/* *** Peripheral devices *** {{{ */

#if LDR_PORT
#endif

#if _DHT
#endif // DHT

#if _LCD84x48
#endif // LCD84x48

#if _DS
#endif // DS

#if _NRF24
/* nRF24L01+: nordic 2.4Ghz digital radio  */

#endif // NRF24

#if _RTC
#endif //_RTC

#if _HMC5883L
/* Digital magnetometer I2C module */

#endif // HMC5883L


/* *** /Peripheral devices }}} *** */

/* *** UI *** {{{ */






/* *** /UI }}} *** */

/* *** Peripheral hardware routines *** {{{ */

#if LDR_PORT
#endif

/* *** PIR support *** {{{ */
#if PIR_PORT
#endif
/* *** /PIR support *** }}} */


#if _DHT
/* DHT temp/rh sensor 
 - AdafruitDHT
*/

#endif // DHT

#if _RFM12B
/* HopeRF RFM12B 868Mhz digital radio */

#endif //_RFM12B

#if _LCD84x48


#endif // LCD84x48

#if _DS
/* Dallas DS18B20 thermometer routines */


#endif // DS

#if _NRF24
/* Nordic nRF24L01+ radio routines */


#endif // NRF24 funcs

#if _RTC
#endif //_RTC

#if _HMC5883L
/* Digital magnetometer I2C routines */


#endif // HMC5883L


/* *** /Peripheral hardware routines }}} *** */

/* *** Initialization routines *** {{{ */

void doConfig(void)
{
	/* load valid config or reset default config */
	if (!loadConfig(static_config)) {
		writeConfig(static_config);
	}
}

void initLibs()
{
}


/* *** /Initialization routines *** }}} */

/* *** Run-time handlers *** {{{ */

void doReset(void)
{
	doConfig();

	tick = 0;

}

bool doAnnounce()
{
/* see CarrierCase */
#if SERIAL && DEBUG
#endif // SERIAL && DEBUG
	return false;
}

// readout all the sensors and other values
void doMeasure()
{
}

void runScheduler(char task)
{
	switch (task) {
	}
}


/* *** /Run-time handlers }}} *** */

/* *** InputParser handlers *** {{{ */

void helpCmd() {
	Serial.println("Help!");
}

void handshakeCmd() {
	int v;
	char buf[7];
	node.toCharArray(buf, 7);
	parser >> v;
	sprintf(node_id, "%s%i", buf, v);
	Serial.print("handshakeCmd:");
	Serial.println(node_id);
	serialFlush();
}

InputParser::Commands cmdTab[] = {
	{ 'h', 0, helpCmd },
	{ 'A', 0, handshakeCmd }
};


/* *** /InputParser handlers }}} *** */

/* *** Main *** {{{ */


void setup(void)
{
#if SERIAL
	mpeser.begin();
	mpeser.startAnnounce(sketch, version);
#if DEBUG || _MEM
	Serial.print("Free RAM: ");
	Serial.println(freeRam());
#endif
	serialFlush();
#endif

	initLibs();

	doReset();
}

void loop(void)
{
	serialFlush();
	//char task = scheduler.pollWaiting();
	//runScheduler(task);
}

/* }}} *** */

