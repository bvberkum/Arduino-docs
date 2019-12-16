/*
  Base sketch SensorNode.

  boilerplate for ATmega node with Serial interface
  uses JeeLib scheduler like JeeLab's roomNode.
  XXX: No radio?  Using std nrf24.

  TODO: test sketch, serial.
    Simple protocol for control, data retrieval sessions?

 */



/* *** Globals and sketch configuration *** */
#define SERIAL_EN         1 /* Enable serial */
#define DEBUG             1 /* Enable trace statements */
#define DEBUG_MEASURE     1

#define _MEM              1 // Report free memory
#define LDR_PORT          0 // defined if LDR is connected to a port's AIO pin
#define _DHT              0
#define DHT_HIGH          0 // enable for DHT22/AM2302, low for DHT11
#define _DS               0
#define DEBUG_DS          0
#define _AM2321           1 // AOSONG AM2321 temp./rhum
#define _BMP085           1 // Bosch BMP085/BMP180 pressure/temp.
#define _LCD              0
#define _LCD84x48         0
#define _RTC              0
#define _RFM12B           0
#define _RFM12LOBAT       0 // Use JeeNode lowbat measurement
#define _NRF24            0

#define REPORT_EVERY      1 // report every N measurement cycles
#define SMOOTH            5 // smoothing factor used for running averages
#define MEASURE_PERIOD    30 // how often to measure, in tenths of seconds
//#define TEMP_OFFSET       -57
//#define TEMP_K            1.0
#define ANNOUNCE_START    0
#define CONFIG_VERSION    "sn 0"
#define CONFIG_EEPROM_START 0
#define NRF24_CHANNEL       90



//#include <SoftwareSerial.h>
#include <EEPROM.h>
#include <JeeLib.h>
#include <OneWire.h>
#include <SPI.h>
#if _DHT
#include <Adafruit_Sensor.h>
#include <DHT.h> // Adafruit DHT
#endif
#include <DotmpeLib.h>
#include <mpelib.h>
//#include "printf.h"




const String sketch = "X-SensorNode";
const int version = 0;

char node[] = "sn";

// determined upon handshake
char node_id[7];


/* IO pins */
//              RXD      0
//              TXD      1
//              INT0     2
#if _DHT
static const byte DHT_PIN = 7;
#endif
#if _DS
static const byte DS_PIN = 8;
#endif
//              MOSI     11
//              MISO     12
//             SCK      13    NRF24
#       define _DBG_LED 13 // SCK


MpeSerial mpeser (57600);


/* *** InputParser *** {{{ */

Stream& cmdIo = Serial;
extern const InputParser::Commands cmdTab[] PROGMEM;
byte* buffer = (byte*) malloc(50);
const InputParser parser (buffer, 50, cmdTab);


/* *** /InputParser *** }}} */

/* *** Report variables *** {{{ */


static byte reportCount;    // count up until next report, i.e. packet send

/* Data reported by this sketch */
struct {
#if LDR_PORT
  byte light      :8; // light sensor: 0..255
#endif
#if PIR_PORT
  byte moved      :1; // motion detector: 0..1
#endif

#if _DHT
  int rhum        :7;  // rel-humdity: 0..100 (4 or 5% resolution?)
// XXX : dht temp
#if DHT_HIGH
/* DHT22/AM2302: 20% to 100% @ 2% rhum, -40C to 80C @ ~0.5 temp */
  int temp        :10; // -500..+500 (int value of tenths avg)
#else
/* DHT11: 20% to 80% @ 5% rhum, 0C to 50C @ ~2C temp */
  int temp        :10; // -500..+500 (tenths, .5 resolution)
#endif // DHT_HIGH
#endif //_DHT

  int ctemp       :10; // atmega temperature: -500..+500 (tenths)
#if _AM2321
  unsigned int am2321temp        :9;   // Range: -10--41.2, 512 value
  unsigned int am2321rhum        :7;  // Range: 0-100
#endif //_AM2321
#if _BMP085
  unsigned int bmp085pressure    :12;  // Weather Range: 990--1030.96, 4096 values
  unsigned int bmp085temp        :9;   // Weather Range: -10--41.2, 512 values
#endif//_BMP085
#ifdef _HMC5883L
  int magnx;
  int magny;
  int magnz;
#endif //_HMC5883L
#if _MEM
  int memfree     :16;
#endif
#if _RFM12LOBAT
  byte lobat      :1; // supply voltage dropped under 3.1V: 0..1
#endif
} payload;

// 2**6  64
// 2**9  512
// 2**10 1024
// 2**11 2048
// 2**12 4096
// 2**13 8192

// nRF24L01 max length: 32 bytes? RF24Network header? - iow. 256 bits


/* *** /Report variables *** }}} */

/* *** Scheduled tasks *** {{{ */

enum {
  ANNOUNCE,
  MEASURE,
  REPORT,
  TASK_END
};
// Scheduler.pollWaiting returns -1 or -2
static const char SCHED_WAITING = 0xFF; // -1: waiting to run
static const char SCHED_IDLE = 0xFE; // -2: no tasks running

static word schedbuf[TASK_END];
Scheduler scheduler (schedbuf, TASK_END);

/* *** /Scheduled tasks *** }}} */

/* *** EEPROM config *** {{{ */


struct Config {
  char node[3]; // Alphanumeric Id (for sketch)
  int node_id; // Id suffix integer (for unique run-time Id)
  int version;
  signed int temp_offset;
  int temp_k;
} static_config = {
  /* default values */
  { node[0], node[1], node[2], }, 0, version,
  TEMP_OFFSET, TEMP_K
};

Config config;

bool loadConfig(Config &c)
{
  unsigned int w = sizeof(c);

  if (
      EEPROM.read(CONFIG_EEPROM_START + 3 ) == CONFIG_VERSION[3] &&
      EEPROM.read(CONFIG_EEPROM_START + 2 ) == CONFIG_VERSION[2] &&
      EEPROM.read(CONFIG_EEPROM_START + 1 ) == CONFIG_VERSION[1] &&
      EEPROM.read(CONFIG_EEPROM_START) == CONFIG_VERSION[0]
  ) {

    for (unsigned int t=0; t<w; t++)
    {
      *((char*)&c + t) = EEPROM.read(CONFIG_EEPROM_START + t);
    }

    return true;
  } else {
#if SERIAL_EN && DEBUG
    cmdIo.println("No valid data in eeprom");
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
#if SERIAL_EN && DEBUG
      cmdIo.println("Error writing "+ String(t)+" to EEPROM");
#endif
    }
  }
#if _RFM12B
#endif //_RFM12B
}



/* *** /EEPROM config *** }}} */

/* *** Scheduler routines *** {{{ */

// has to be defined because we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

/* *** /Scheduler routines *** }}} */

/* *** Peripheral devices *** {{{ */

#if SHT11_PORT
#endif

#if LDR_PORT
Port ldr (LDR_PORT);
#endif

/* *** PIR *** {{{ */
#if PIR_PORT
#endif
/* *** /PIR *** }}} */

#if _DHT
/* DHTxx: Digital temperature/humidity (Adafruit) */
#if DHT_HIGH
DHT dht (DHT_PIN, DHT21);
//DHT dht (DHT_PIN, DHT22);
// AM2301
// DHT21, AM2302? AM2301?
#else
DHT dht(DHT_PIN, DHT11);
#endif
#endif // DHT

#if _AM2321
#include <Wire.h>
#include <AM2321.h>

AM2321 ac;
#endif

#if _BMP085
#include <Adafruit_BMP085_U.h>

Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);

#endif

#if _LCD84x48
/* Nokkia 5110 display */
#endif // LCD84x48

#if _DS
/* Dallas OneWire bus with registration for DS18B20 temperature sensors */

OneWire ds(DS_PIN);

uint8_t ds_count = 0;
uint8_t ds_search = 0;
volatile int ds_value[0];

//uint8_t ds_addr[ds_count][8] = {
//  { 0x28, 0xCC, 0x9A, 0xF4, 0x03, 0x00, 0x00, 0x6D }, // In Atmega8TempGaurd
//  { 0x28, 0x8A, 0x5B, 0xDD, 0x03, 0x00, 0x00, 0xA6 },
//  { 0x28, 0x45, 0x94, 0xF4, 0x03, 0x00, 0x00, 0xB3 },
//  { 0x28, 0x08, 0x76, 0xF4, 0x03, 0x00, 0x00, 0xD5 },
//  { 0x28, 0x82, 0x27, 0xDD, 0x03, 0x00, 0x00, 0x4B },
//};
enum { DS_OK, DS_ERR_CRC };

#endif // DS

#if _RFM12B
/* HopeRF RFM12B 868Mhz digital radio */


#endif // RFM12B

#if _NRF24
/* nRF24L01+: nordic 2.4Ghz digital radio */

// Set up nRF24L01 radio on SPI bus plus two extra pins
RF24 radio(CE, CSN);

// Network uses that radio
#if DEBUG
RF24NetworkDebug network(radio);
#else
RF24Network network(radio);
#endif

// Address of the other node
const uint16_t rf24_link_node = 0;


#endif // NRF24

#if _RTC
/* DS1307: Real Time Clock over I2C */
#endif // RTC

#if _HMC5883L
/* Digital magnetometer I2C module */

#endif // HMC5883L


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

#if _LCD
#endif //_LCD

#if _LCD84x48


#endif // LCD84x48

#if _DS
/* Dallas DS18B20 thermometer routines */

static int ds_readdata(uint8_t addr[8], uint8_t data[12]) {
  byte i;
  byte present = 0;

  ds.reset();
  ds.select(addr);
  ds.write(0x44,1);         // start conversion, with parasite power on at the end

  //Sleepy::loseSomeTime(800); //delay(800); // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.

  present = ds.reset();
  ds.select(addr);
  ds.write(0xBE);         // Read Scratchpad

#if SERIAL_EN && DEBUG_DS
  cmdIo.print(F("Data="));
  cmdIo.print(present, HEX);
  cmdIo.print(" ");
#endif
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
#if SERIAL_EN && DEBUG_DS
    cmdIo.print(i);
    cmdIo.print(':');
    cmdIo.print(data[i], HEX);
    cmdIo.print(" ");
#endif
  }

  uint8_t crc8 = OneWire::crc8( data, 8);

#if SERIAL_EN && DEBUG_DS
  cmdIo.print(F(" CRC="));
  cmdIo.print( crc8, HEX);
  cmdIo.println();
  serialFlush();
#endif

  if (crc8 != data[8]) {
    return DS_ERR_CRC;
  } else {
    return DS_OK;
  }
}

static int ds_conv_temp_c(uint8_t data[8], int SignBit) {
  int HighByte, LowByte, TReading, Tc_100;
  LowByte = data[0];
  HighByte = data[1];
  TReading = (HighByte << 8) + LowByte;
  SignBit = TReading & 0x8000;  // test most sig bit
  if (SignBit) // negative
  {
    TReading = (TReading ^ 0xffff) + 1; // 2's comp
  }
  Tc_100 = (6 * TReading) + TReading / 4;    // multiply by (100 * 0.0625) or 6.25
  return Tc_100;
}

static int readDS18B20(byte addr[8]) {
  byte data[12];
  int SignBit;

#if SERIAL_EN && ( DEBUG || DEBUG_DS )
  cmdIo.print("Reading Address=");
  for (int i=0;i<8;i++) {
    cmdIo.print(i);
    cmdIo.print(':');
    cmdIo.print(addr[i], HEX);
    cmdIo.print(' ');
  }
  cmdIo.println();
#endif

  int result = ds_readdata(addr, data);

  if (result != DS_OK) {
#if SERIAL_EN
    cmdIo.println(F("CRC error in ds_readdata"));
    serialFlush();
#endif
    return 32767;
  }

  int Tc_100 = ds_conv_temp_c(data, SignBit);

  if (SignBit) {
    return 0 - Tc_100;
  } else {
    return Tc_100;
  }
}

static int readDSCount() {
  int ds_count = EEPROM.read(DS_EEPROM_OFFSET);
  if (ds_count == 0xFF) return 0;
  return ds_count;
}

static void updateDSCount(int new_count) {
  int ds_count = EEPROM.read(DS_EEPROM_OFFSET);
  if (new_count != ds_count) {
    EEPROM.write(DS_EEPROM_OFFSET, new_count);
  }
}

static void writeDSAddr(int a, byte addr[8]) {
  int l = DS_EEPROM_OFFSET + 1 + ( a * 8 );
  for (int i=0;i<8;i++) {
    EEPROM.write(l+i, addr[i]);
  }
}

static void readDSAddr(int a, byte addr[8]) {
  int l = DS_EEPROM_OFFSET + 1 + ( a * 8 );
  for (int i=0;i<8;i++) {
    addr[i] = EEPROM.read(l+i);
  }
}

static void printDSAddrs(int ds_count) {
#if SERIAL_EN && DEBUG_DS
  cmdIo.print("Reading ");
  cmdIo.print(ds_count);
  cmdIo.println(" DS18B20 sensors");
  for (int q=0; q<ds_count;q++) {
    cmdIo.print("Mem Address=");
    int l = DS_EEPROM_OFFSET + 1 + ( q * 8 );
    int r[8];
    for (int i=0;i<8;i++) {
      r[i] = EEPROM.read(l+i);
      cmdIo.print(i);
      cmdIo.print(':');
      cmdIo.print(r[i], HEX);
      cmdIo.print(' ');
    }
    cmdIo.println();
  }
#endif
}

static int findDS18B20s(int &ds_search) {
  byte i;
  byte addr[8];

  if (!ds.search(addr)) {
    ds.reset_search();
#if SERIAL_EN && DEBUG_DS
    cmdIo.println("No more addresses.");
    cmdIo.print("Found ");
    cmdIo.print(ds_search);
    cmdIo.println(" addresses.");
    cmdIo.print("Previous search found ");
    cmdIo.print(readDSCount());
    cmdIo.println(" addresses.");
#endif
    return 0;
  }

  if ( OneWire::crc8( addr, 7 ) != addr[7]) {
#if SERIAL_EN
    cmdIo.println("CRC is not valid!");
#endif
    return 2;
  }

#if SERIAL_EN && ( DEBUG || DEBUG_DS )
  cmdIo.print("New Address=");
  for( i = 0; i < 8; i++) {
    cmdIo.print(i);
    cmdIo.print(':');
    cmdIo.print(addr[i], HEX);
    cmdIo.print(" ");
  }
  cmdIo.println();
#endif

  ds_search++;
  cmdIo.println(ds_search);

  writeDSAddr(( ds_search-1 ), addr);

  return 1;
}


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
  config.rf24id = 3;
  network.begin( NRF24_CHANNEL, config.rf24id );
}

void rf24_start()
{
#if DEBUG
  radio.printDetails();
#endif
}


#endif // NRF24 funcs

#if _RTC
#endif // RTC

#if _HMC5883L
/* Digital magnetometer I2C routines */


#endif // HMC5883L


/* *** /Peripheral hardware routines *** }}} */

/* *** UI *** {{{ */



/* *** /UI *** }}} */

/* *** UART commands *** {{{ */

#if SERIAL_EN

void help_sercmd(void) {
  cmdIo.println("[Prototype/SensorNode:0] Help");
  cmdIo.println("c: print node, -id, version and config-id");
  cmdIo.println("t: print internal temperature");
  cmdIo.println("<O> T: set internal temperature offset (integer)");
  // TODO: cmdIo.println("<O> K: set internal temperature coefficient (float)");
  cmdIo.println("o: print temp. cfg.");
  cmdIo.println("r: report now");
  cmdIo.println("M: measure now");
  cmdIo.println("<TYPE> N: set Node (3 byte char)");
  cmdIo.println("<ID> C: set Node ID (1 byte int)");
  cmdIo.println("1/0 W: write or load config to/from EEPROM");
  // TODO: cmdIo.println("R: reset config to factory defaults and write to EEPROM");
  cmdIo.println("E: erase EEPROM!");
  cmdIo.println("n: node cfg.");
  cmdIo.println("v: node ver.");
  cmdIo.println("x: soft reset");
  cmdIo.println("?/h: this help");
}

// forward declarations
void initConfig(void);
void doReset(void);

void config_sercmd(void) {
  cmdIo.print("c ");
  cmdIo.print(config.node);
  cmdIo.print(" ");
  cmdIo.print(config.node_id);
  cmdIo.print(" ");
  cmdIo.print(config.version);
  cmdIo.println();
}

static void configNodeCmd(void) {
  cmdIo.print("n ");
  cmdIo.print(node_id);
}

static void configVersionCmd(void) {
  cmdIo.print("v ");
  cmdIo.println(config.version);
}

static void configSetNodeCmd() {
  const char *node;
  parser >> node;
  config.node[0] = node[0];
  config.node[1] = node[1];
  config.node[2] = node[2];
  initConfig();
  cmdIo.print("N ");
  cmdIo.println(config.node);
}

static void configNodeIDCmd() {
  parser >> config.node_id;
  initConfig();
  cmdIo.print("C ");
  cmdIo.println(node_id);
  serialFlush();
}

void ctempconfig_sercmd(void) {
  cmdIo.print("Offset: ");
  cmdIo.println(config.temp_offset);
  cmdIo.print("K: ");
  cmdIo.println(config.temp_k/10);
  cmdIo.print("Raw: ");
  float temp_k = (float) config.temp_k / 10;
  int ctemp = internalTemp(config.temp_offset, temp_k);
  cmdIo.println(ctemp);
}

void ctempoffset_sercmd(void) {
  char c;
  parser >> c;
  int v = c;
  if( v > 127 ) {
    v -= 256;
  }
  config.temp_offset = v;
  cmdIo.print("New offset: ");
  cmdIo.println(config.temp_offset);
  writeConfig(config);
}

void ctemp_sercmd(void) {
  float temp_k = (float) config.temp_k / 10;
  double ctemp = internalTemp(config.temp_offset, temp_k);
  cmdIo.println(ctemp);
}

void configEEPROMCmd(void) {
  int write;
  parser >> write;
  if (write) {
    writeConfig(config);
  } else {
    loadConfig(config);
    initConfig();
  }
  cmdIo.print("W ");
  cmdIo.println(write);
}

static void eraseEEPROM() {
  cmdIo.print("! Erasing EEPROM..");
  for (int i = 0; i<EEPROM_SIZE; i++) {
    char b = EEPROM.read(i);
    if (b != 0x00) {
      EEPROM.write(i, 0);
      cmdIo.print('.');
    }
  }
  cmdIo.println(' ');
  cmdIo.print("E ");
  cmdIo.println(EEPROM_SIZE);
}

#endif


/* *** UART commands *** }}} */

/* *** Initialization routines *** {{{ */

void initConfig(void)
{
  sprintf(node_id, "%s%i", config.node, config.node_id);
}

void doConfig(void)
{
  /* load valid config or reset default config */
  if (!loadConfig(config)) {
    writeConfig(config);
  }
  initConfig();
}

void initLibs()
{
#if _NRF24
  rf24_init();
#endif // NRF24

#if _RTC
  rtc_init();
#endif
#if _DHT
  dht.begin();
#if DEBUG
  cmdIo.println("Initialized DHT");
  float t = dht.readTemperature();
  cmdIo.println(t);
  float rh = dht.readHumidity();
  cmdIo.println(rh);
#endif
#endif

#if _HMC5883L
  /* Initialise the Wire library */
  Wire.begin();

  /* Initialise the module */
  Init_HMC5803L();
#endif //_HMC5883L

//#if SERIAL_EN && DEBUG
//  printf_begin();
//#endif
}


/* *** /Initialization routines *** }}} */

/* *** Run-time handlers *** {{{ */

void doReset(void)
{
  doConfig();

#ifdef _DBG_LED
  pinMode(_DBG_LED, OUTPUT);
#endif

#if _DS
  int ds_search = 0;
  while (findDS18B20s(ds_search) == 1) ;
  updateDSCount(ds_search);
#endif
  tick = 0;

#if _NRF24
  rf24_init();
#endif //_NRF24

  reportCount = REPORT_EVERY; // report right away for easy debugging
  scheduler.timer(ANNOUNCE, ANNOUNCE_START); // get the measurement loop going
#if SERIAL_EN && DEBUG
  cmdIo.println("Reinitialized");
#endif
}

bool doAnnounce(void)
{
/* see CarrierCase */
#if SERIAL_EN && DEBUG
  cmdIo.print("\n[");
  cmdIo.print(sketch);
  cmdIo.print(".");
  cmdIo.print(version);
  cmdIo.println("]");
#endif // SERIAL_EN && DEBUG
  cmdIo.println(node_id);
#if LDR_PORT
  cmdIo.print("light:8 ");
#endif
#if PIR
  cmdIo.print("moved:1 ");
#endif
#if _DHT
  cmdIo.print(F("rhum:7 "));
  cmdIo.print(F("temp:10 "));
#endif
  cmdIo.print(F("ctemp:10 "));
#if _MEM
  cmdIo.print(F("memfree:16 "));
#endif
#if _RFM12LOBAT
  cmdIo.print(F("lobat:1 "));
#endif //_RFM12LOBAT
  return false;
}


// readout all the sensors and other values
void doMeasure(void)
{
  byte firstTime = payload.ctemp == 0; // special case to init running avg

  int ctemp = internalTemp(config.temp_offset, (float)config.temp_k/10);
  payload.ctemp = smoothedAverage(payload.ctemp, ctemp, firstTime);
#if SERIAL_EN && DEBUG_MEASURE
  cmdIo.print("Payload size: ");
  cmdIo.println(sizeof(payload));
  cmdIo.print("AVR T new/avg ");
  cmdIo.print(ctemp);
  cmdIo.print('/');
  cmdIo.println(payload.ctemp);
#endif

#if _RFM12LOBAT
  payload.lobat = rf12_lowbat();
#if SERIAL_EN && DEBUG_MEASURE
  if (payload.lobat) {
    cmdIo.println("Low battery");
  }
#endif
#endif //_RFM12LOBAT

#if _DHT
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (isnan(h)) {
#if SERIAL_EN | DEBUG
    cmdIo.println(F("Failed to read DHT11 humidity"));
#endif
  } else {
    int rh = h * 10;
    payload.rhum = smoothedAverage(payload.rhum, rh, firstTime);
#if SERIAL_EN && DEBUG_MEASURE
    cmdIo.print(F("DHT RH new/avg "));
    cmdIo.print(rh);
    cmdIo.print(' ');
    cmdIo.println(payload.rhum);
#endif
  }
  if (isnan(t)) {
#if SERIAL_EN | DEBUG
    cmdIo.println(F("Failed to read DHT11 temperature"));
#endif
  } else {
    payload.temp = smoothedAverage(payload.temp, (int)(t * 10), firstTime);
#if SERIAL_EN && DEBUG_MEASURE
    cmdIo.print(F("DHT T new/avg "));
    cmdIo.print(t);
    cmdIo.print(' ');
    cmdIo.println(payload.temp);
#endif
  }
#endif //_DHT

#if LDR_PORT
  ldr.digiWrite2(1);  // enable AIO pull-up
  byte light = ~ ldr.anaRead() >> 2;
  ldr.digiWrite2(0);  // disable pull-up to reduce current draw
  payload.light = smoothedAverage(payload.light, light, firstTime);
#if SERIAL_EN && DEBUG_MEASURE
  cmdIo.print(F("LDR new/avg "));
  cmdIo.print(light);
  cmdIo.print(' ');
  cmdIo.println(payload.light);
#endif
#endif //_LDR_PORT

#if _DS
  int ds_count = readDSCount();
  byte addr[8];
  for ( int i = 0; i < ds_count; i++) {
    readDSAddr(i, addr);
    ds_value[i] = readDS18B20(addr);
#if SERIAL_EN && DEBUG_MEASURE
    cmdIo.print(F("DS18B20 #"));
    cmdIo.print(i+1);
    cmdIo.print('/');
    cmdIo.print(ds_count);
    cmdIo.print(' ');
    cmdIo.println( (float) ds_value[i] / 100 );
#endif
  }
#endif //_DS

#if _HMC5883L
  payload.magnx = smoothedAverage(payload.magnx, HMC5803L_Read(X), firstTime);
  payload.magny = smoothedAverage(payload.magny, HMC5803L_Read(Y), firstTime);
  payload.magnz = smoothedAverage(payload.magnz, HMC5803L_Read(Z), firstTime);
#endif //_HMC5883L

#if _MEM
  payload.memfree = freeRam();
#if SERIAL_EN && DEBUG_MEASURE
  cmdIo.print("MEM free ");
  cmdIo.println(payload.memfree);
#endif
#endif //_MEM

#if _RFM12LOBAT
  payload.lobat = rf12_lowbat();
#if SERIAL_EN && DEBUG_MEASURE
  if (payload.lobat) {
    cmdIo.println("Low battery");
  }
#endif
#endif //_RFM12LOBAT
}


// periodic report, i.e. send out a packet and optionally report on serial port
bool doReport(void)
{
  bool ok;

#if _RFM12B
  serialFlush();
  rf12_sleep(RF12_WAKEUP);
  rf12_sendNow(0, &payload, sizeof payload);
  rf12_sendWait(RADIO_SYNC_MODE);
  rf12_sleep(RF12_SLEEP);
#endif //_RFM12B

#if _NRF24
  //rf24_run();
  RF24NetworkHeader header(/*to node*/ rf24_link_node);
  ok = network.write(header, &payload, sizeof(payload));
#endif //_NRF24

#if SERIAL_EN
  /* Report over serial, same fields and order as announced */
  cmdIo.print(node_id);
#if LDR_PORT
  cmdIo.print(' ');
  cmdIo.print((int) payload.light);
#endif
  cmdIo.print((int) payload.ctemp);
#if _DS
  int ds_count = readDSCount();
  for (int i=0;i<ds_count;i++) {
    cmdIo.print((int) ds_value[i]);
    cmdIo.print(' ');
  }
#endif
#if _MEM
  cmdIo.print(' ');
  cmdIo.print((int) payload.memfree);
#endif //_MEM
#if _RFM12LOBAT
  cmdIo.print(' ');
  cmdIo.print((int) payload.lobat);
#endif //_RFM12LOBAT

  cmdIo.println();
#endif //SERIAL_EN

  return ok;
}

void runScheduler(char task)
{
  switch (task) {

    case ANNOUNCE:
      debugline("ANNOUNCE");
      cmdIo.print(strlen(node_id));
      cmdIo.println();
      if (strlen(node_id) > 0) {
        cmdIo.print("Node: ");
        cmdIo.println(node_id);
        //scheduler.timer(MEASURE, 0); schedule next step
      } else {
        doAnnounce();
        scheduler.timer(MEASURE, 0); //schedule next step
      }
#if SERIAL_EN
      serialFlush();
#endif
      break;

    case MEASURE:
      // reschedule these measurements periodically
      scheduler.timer(MEASURE, MEASURE_PERIOD);
      doMeasure();

      // every so often, a report needs to be sent out
      if (++reportCount >= REPORT_EVERY) {
        reportCount = 0;
        scheduler.timer(REPORT, 0);
      }
#if SERIAL_EN
      serialFlush();
#endif
#ifdef _DBG_LED
      blink(_DBG_LED, 2, 25);
#endif
      break;

    case REPORT:
      doReport();
      serialFlush();
      break;

#if DEBUG && SERIAL_EN
    case SCHED_WAITING:
    case SCHED_IDLE:
      cmdIo.print("!");
      serialFlush();
      break;

    default:
      cmdIo.print("0x");
      cmdIo.print(task, HEX);
      cmdIo.println(" ?");
      serialFlush();
      break;
#endif

  }
}


/* *** /Run-time handlers *** }}} */

/* *** InputParser handlers *** {{{ */

#if SERIAL_EN

const InputParser::Commands cmdTab[] = {
  { '?', 0, help_sercmd },
  { 'h', 0, help_sercmd },
  { 'c', 0, config_sercmd },
  { 'n', 0, configNodeCmd },
  { 'v', 0, configVersionCmd },
  { 'N', 3, configSetNodeCmd },
  { 'C', 1, configNodeIDCmd },
  { 'W', 1, configEEPROMCmd },
  { 'o', 0, ctempconfig_sercmd },
  { 'T', 1, ctempoffset_sercmd },
  { 't', 0, ctemp_sercmd },
  { 'r', 0, doReport },
  { 'M', 0, doMeasure },
  { 'E', 0, eraseEEPROM },
  { 'x', 0, doReset },
  { 0 }
};

#endif // SERIAL_EN


/* *** /InputParser handlers *** }}} */

/* *** Main *** {{{ */


void setup(void)
{
#if SERIAL_EN
  mpeser.begin();
  mpeser.startAnnounce(sketch, String(version));
#if DEBUG || _MEM
  cmdIo.print(F("Free RAM: "));
  cmdIo.println(freeRam());
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
#endif //_NRF24

#if _DS
  bool ds_reset = digitalRead(7);
  if (ds_search || ds_reset) {
    if (ds_reset) {
      cmdIo.println("DS-reset triggered");
    }
    int ds_search = 0;
    while (findDS18B20s(ds_search) == 1) ;
    updateDSCount(ds_search);
    return;
  }
#endif

  if (debug_ticks()) {
#ifdef _DBG_LED
    blink(_DBG_LED, 1, 50);
#endif
  }

  if (cmdIo.available()) {
    parser.poll();
    return;
  }

  char task = scheduler.poll();
  if (-1 < task && task < SCHED_IDLE) {
    runScheduler(task);
    return;
  }

    /* Sleep */
    serialFlush();
//    task = scheduler.pollWaiting();
//    if (-1 < task && task < SCHED_IDLE) {
//      runScheduler(task);
//    }
}

/* *** }}} */
