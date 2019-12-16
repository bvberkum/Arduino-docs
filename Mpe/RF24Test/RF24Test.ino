/*

 This'll initialize the nRF24L01, set it to listening and then
 dump register values to serial.

 If it is hooked up correctly, it'll show actual values--not all-zero's.

 Compiled with Arduino 1.0.[4-6]

 */



/* *** Globals and sketch configuration *** */
#define SERIAL_EN         1 /* Enable serial */
#define DEBUG             1 /* Enable trace statements */

#define _MEM              1 // Report free memory
#define _NRF24            1
#define MAXLENLINE        79



//#include <EEPROM.h>
//#include <JeeLib.h>
//#include <OneWire.h>
//#include <PCD8544.h>
#include <SPI.h>
//#include "nRF24L01.h"
#include <RF24.h>
//#include <RF24Network.h>
#include <DotmpeLib.h>
#include <mpelib.h>

#include "printf.h"



const String sketch = "RF24Test";
const int version = 0;

static String node = "rf24test";



/* IO pins */
#       define CSN       10  // NRF24
#       define CE        9  // NRF24


MpeSerial mpeser (57600);


/* *** Scheduled tasks *** {{{ */


/* *** /Scheduled tasks *** }}} */

/* *** EEPROM config *** {{{ */



/* *** /EEPROM config *** }}} */

/* *** Peripheral devices *** {{{ */

#if _RFM12B
/* HopeRF RFM12B 868Mhz digital radio */


#endif // RFM12B

#if _NRF24
/* nRF24L01+: nordic 2.4Ghz digital radio  */

// Set up nRF24L01 radio on SPI bus plus two extra pins
RF24 radio(CE, CSN);

// nRF24L01 addresses: one for broadcast, one for listening
const uint64_t pipes[2] = {
  0xF0F0F0F0D2LL, /* dest id: central link node */
  0xF0F0F0F0E1LL /* src id: local node */
};

#endif // NRF24


/* *** /Peripheral devices *** }}} */

/* *** Peripheral hardware routines *** {{{ */


#if LDR_PORT
#endif

/* *** - PIR routines *** {{{ */
#if PIR_PORT
#endif // PIR_PORT
/* *** /- PIR routines *** }}} */

#if _DHT
/* DHT temp/rh sensor routines (AdafruitDHT) */

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
  SPI.begin();
  radio.begin();
}

#endif // NRF24 funcs

#if _RTC
#endif // RTC

#if _HMC5883L
/* Digital magnetometer I2C routines */


#endif // HMC5883L


/* *** /Peripheral hardware routines *** }}} */


/* *** Initialization routines *** {{{ */

void doConfig(void)
{
}

void initLibs(void)
{
#if _NRF24
  rf24_init();
#endif // NRF24

#if SERIAL_EN && DEBUG
  printf_begin();
#endif
}


/* *** /Initialization routines *** }}} */

/* *** Run-time handlers *** {{{ */

void doReset(void)
{
  tick = 0;

#if _NRF24
  radio.openReadingPipe(1,pipes[1]);
  radio.startListening();
#endif //_NRF24
}



/* *** /InputParser handlers *** }}} */

/* *** Main *** {{{ */


void setup(void)
{
#if SERIAL_EN
  mpeser.begin();
  mpeser.startAnnounce(sketch, String(version));
#if DEBUG || _MEM
  Serial.print(F("Free RAM: "));
  Serial.println(freeRam());
#endif
  serialFlush();
#endif

  initLibs();

  Serial.print("Init done, resetting libs..");

  doReset();

  Serial.println("OK\n\r[RF24/examples/GettingStarted/]");

#if _NRF24
  Serial.print("Radio details:");
  radio.printDetails();
  Serial.println();
#endif
}

void loop(void)
{
}

/* *** }}} */

