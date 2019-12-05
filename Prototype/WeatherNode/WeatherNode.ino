/*
 */




/* *** Globals and sketch configuration *** */
#define SERIAL_EN         1 /* Enable serial */
#define DEBUG             1 /* Enable trace statements */
#define DEBUG_MEASURE     1

#define _MEM              1  // Report free memory
#define LDR_PORT          0  // defined if LDR is connected to a port's AIO pin
#define _DHT              0
#define DHT_HIGH          0  // enable for DHT22/AM2302, low for DHT11
#define _DS               0
#define DEBUG_DS          0
#define _AM2321           1  // AOSONG AM2321 temp./rhum
#define _BMP085           1  // Bosch BMP085/BMP180 pressure/temp.
#define _LCD              0
#define _LCD84x48         0
#define _RTC              0
#define _RFM12B           0
#define _RFM12LOBAT       0  // Use JeeNode lowbat measurement
#define _NRF24            0

#define REPORT_EVERY      1  // report every N measurement cycles
#define SMOOTH            5  // smoothing factor used for running averages
#define MEASURE_PERIOD    30 // how often to measure, in tenths of seconds
//#define TEMP_OFFSET     -57
//#define TEMP_K          1.0
#define DS_EEPROM_OFFSET  96
#define ANNOUNCE_START  0
#define CONFIG_VERSION "wn1"
// byte 15 seems to be bad at dccduino
#define CONFIG_EEPROM_START 64
#define NRF24_CHANNEL       90



#include <Wire.h>
//#include <SoftwareSerial.h>
#include <EEPROM.h>
#include <JeeLib.h>
#include <OneWire.h>
//#include <SPI.h>
#if _DHT
#include <Adafruit_Sensor.h>
//#include <RF24.h>
//#include <RF24Network.h>
#include <DHT.h> // Adafruit DHT
#endif
#include <DotmpeLib.h>
#include <mpelib.h>
//#include "printf.h"




const String sketch = "WeatherNode";
const int version = 0;

char node[] = "wn";

// determined upon handshake
char node_id[7];


/* IO pins */
//              RXD      0
//              TXD      1
//              INT0     2
#       define CSN      8  // NRF24
#       define CE       9  // NRF24
//              MOSI     11
#if _DHT
static const byte DHT_PIN = 7;
#endif
#       define _DBG_LED 12 // MISO
#if _DS
static const byte DS_PIN = 13; // SCK
#endif
//             SCK      13    NRF24


MpeSerial mpeser (57600);


/* *** InputParser *** {{{ */

Stream& cmdIo = Serial;
extern const InputParser::Commands cmdTab[] PROGMEM;
const InputParser parser (50, cmdTab);


/* *** /InputParser }}} *** */

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

#ifdef _DHT
  int rhum        :7;  // rhumdity: 0..100 (4 or 5% resolution?)
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

// has to be defined because we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }


/* *** /Scheduled tasks *** }}} */

/* *** EEPROM config *** {{{ */


struct Config {
  char node[3];
  int node_id;
  int version;
  char config_id[4];
  signed int temp_offset;
  float temp_k;
} static_config = {
  /* default values */
  { node[0], node[1], node[2], }, 0, version,
  CONFIG_VERSION,
  TEMP_OFFSET, TEMP_K
};

Config config;

bool loadConfig(Config &c)
{
  unsigned int w = sizeof(c);

  if (
      //EEPROM.read(CONFIG_EEPROM_START + w - 1) == c.config_id[3] &&
      //EEPROM.read(CONFIG_EEPROM_START + w - 2) == c.config_id[2] &&
      EEPROM.read(CONFIG_EEPROM_START + w - 3) == c.node[1] &&
      EEPROM.read(CONFIG_EEPROM_START + w - 4) == c.node[0]
  ) {

    for (unsigned int t=0; t<w; t++)
    {
      *((char*)&c + t) = EEPROM.read(CONFIG_EEPROM_START + t);
    }

    return true;
  } else {
#if SERIAL_EN && DEBUG
    Serial.println("No valid data in eeprom");
#endif
    return false;
  }
}

void writeConfig(Config &c)
{
#if SERIAL_EN && DEBUG
  Serial.print("Writing ");
  Serial.print(sizeof(c));
  Serial.println(" bytes to EEPROM");
#endif
  for (unsigned int t=0; t<sizeof(c); t++) {

    EEPROM.write(CONFIG_EEPROM_START + t, *((char*)&c + t));
#if SERIAL_EN && DEBUG
    // verify
    if (EEPROM.read(CONFIG_EEPROM_START + t) != *((byte*)&c + t))
    {
      // error writing to EEPROM
      Serial.println("Error writing "+ String(t)+" to EEPROM");
      Serial.println( String(EEPROM.read(CONFIG_EEPROM_START + t))
        +" "+
        String(*((byte*)&c + t)) );
    }
#endif
  }
}



/* *** /EEPROM config *** }}} */

/* *** Peripheral devices *** {{{ */

#if SHT11_PORT
#endif

#if LDR_PORT
Port ldr (LDR_PORT);
#endif

/* *** PIR support *** {{{ */
#if PIR_PORT
#endif
/* *** /PIR support }}} *** */

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
#endif //_DHT

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
#endif //_LCD84x48

#if _DS
/* Dallas OneWire bus with registration for DS18B20 temperature sensors */

OneWire ds(DS_PIN);

uint8_t ds_count = 0;
uint8_t ds_search = 0;
volatile int ds_value[0];

enum { DS_OK, DS_ERR_CRC };

#endif // DS

#if _RFM12B
/* HopeRF RFM12B 868Mhz digital radio */


#endif //_RFM12B

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
#endif //_RTC

#if _HMC5883L
/* Digital magnetometer I2C module */


#endif // HMC5883L


/* *** /Peripheral devices *** }}} */

/* *** Peripheral hardware routines *** {{{ */


#if LDR_PORT
#endif

/* *** PIR support *** {{{ */
#if PIR_PORT
#endif // PIR_PORT
/* *** /PIR support *** }}} */


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

  //Sleepy::loseSomeTime(800);
  //delay(800);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.

  present = ds.reset();
  ds.select(addr);
  ds.write(0xBE);         // Read Scratchpad

#if SERIAL_EN && DEBUG_DS
  Serial.print(F("Data="));
  Serial.print(present, HEX);
  Serial.print(" ");
#endif
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
#if SERIAL_EN && DEBUG_DS
    Serial.print(i);
    Serial.print(':');
    Serial.print(data[i], HEX);
    Serial.print(" ");
#endif
  }

  uint8_t crc8 = OneWire::crc8( data, 8);

#if SERIAL_EN && DEBUG_DS
  Serial.print(F(" CRC="));
  Serial.print( crc8, HEX);
  Serial.println();
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
  Serial.print("Reading Address=");
  for (int i=0;i<8;i++) {
    Serial.print(i);
    Serial.print(':');
    Serial.print(addr[i], HEX);
    Serial.print(' ');
  }
  Serial.println();
#endif

  int result = ds_readdata(addr, data);

  if (result != DS_OK) {
#if SERIAL_EN
    Serial.println(F("CRC error in ds_readdata"));
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
  Serial.print("Reading ");
  Serial.print(ds_count);
  Serial.println(" DS18B20 sensors");
  for (int q=0; q<ds_count;q++) {
    Serial.print("Mem Address=");
    int l = DS_EEPROM_OFFSET + 1 + ( q * 8 );
    int r[8];
    for (int i=0;i<8;i++) {
      r[i] = EEPROM.read(l+i);
      Serial.print(i);
      Serial.print(':');
      Serial.print(r[i], HEX);
      Serial.print(' ');
    }
    Serial.println();
  }
#endif
}

static int findDS18B20s(int &ds_search) {
  byte i;
  byte addr[8];

  if (!ds.search(addr)) {
    ds.reset_search();
#if SERIAL_EN && DEBUG_DS
    Serial.println("No more addresses.");
    Serial.print("Found ");
    Serial.print(ds_search);
    Serial.println(" addresses.");
    Serial.print("Previous search found ");
    Serial.print(readDSCount());
    Serial.println(" addresses.");
#endif
    return 0;
  }

  if ( OneWire::crc8( addr, 7 ) != addr[7]) {
#if SERIAL_EN
    Serial.println("CRC is not valid!");
#endif
    return 2;
  }

#if SERIAL_EN && ( DEBUG || DEBUG_DS )
  Serial.print("New Address=");
  for( i = 0; i < 8; i++) {
    Serial.print(i);
    Serial.print(':');
    Serial.print(addr[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
#endif

  ds_search++;
  Serial.println(ds_search);

  writeDSAddr(( ds_search-1 ), addr);

  return 1;
}


#endif //_DS

#if _RFM12B
/* HopeRF RFM12B 868Mhz digital radio routines */

// wait a few milliseconds for proper ACK to me, return true if indeed received
static bool waitForAck() {
    MilliTimer ackTimer;
    while (!ackTimer.poll(ACK_TIME)) {
        if (rf12_recvDone() && rf12_crc == 0 &&
                // see http://talk.jeelabs.net/topic/811#post-4712
                rf12_hdr == (RF12_HDR_DST | RF12_HDR_CTL | rf12_id))
            return 1;
        set_sleep_mode(SLEEP_MODE_SCHED_IDLE);
        sleep_mode();
    }
    return 0;
}

#endif //_RFM12B

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
#endif //_RTC

#if _HMC5883L
/* Digital magnetometer I2C routines */

/* This function will initialise the module and only needs to be run once
   after the module is first powered up or reset */
void Init_HMC5803L(void)
{
  /* Set the module to 8x averaging and 15Hz measurement rate */
  Wire.beginTransmission(HMC5803L_Address);
  Wire.write(0x00);
  Wire.write(0x70);

  /* Set a gain of 5 */
  Wire.write(0x01);
  Wire.write(0xA0);
  Wire.endTransmission();
}

/* This function will read once from one of the 3 axis data registers
   and return the 16 bit signed result. */
int HMC5803L_Read(byte Axis)
{
  int Result;

  /* Initiate a single measurement */
  Wire.beginTransmission(HMC5803L_Address);
  Wire.write(0x02);
  Wire.write(0x01);
  Wire.endTransmission();
  delay(6);

  /* Move modules the resiger pointer to one of the axis data registers */
  Wire.beginTransmission(HMC5803L_Address);
  Wire.write(Axis);
  Wire.endTransmission();

  /* Read the data from registers (there are two 8 bit registers for each axis) */
  Wire.requestFrom(HMC5803L_Address, 2);
  Result = Wire.read() << 8;
  Result |= Wire.read();

  return Result;
}
#endif // HMC5883L


/* *** /Peripheral hardware routines *** }}} */

/* *** UI *** {{{ */

/* *** /UI *** }}} */

/* *** UART commands *** {{{ */

#if SERIAL_EN

void help_sercmd(void) {
  cmdIo.println("n: print Node ID");
  cmdIo.println("t: internal temperature");
  cmdIo.println("T: set offset");
  cmdIo.println("o: temperature config");
  cmdIo.println("a: announce");
  cmdIo.println("r: report now");
  cmdIo.println("M: measure now");
  cmdIo.println("E: erase EEPROM!");
  cmdIo.println("x: reset");
  cmdIo.println("?/h: this help");
}

// forward declarations
void doReset(void);
bool doAnnounce(void);
void doReport(void);
void doMeasure(void);

void reset_sercmd() {
  doReset();
}

void announce_sercmd() {
  doAnnounce();
}

void report_sercmd() {
  doReport();
}

void measure_sercmd() {
  doMeasure();
}

void config_sercmd() {
  cmdIo.print("c ");
  cmdIo.print(config.node);
  cmdIo.print(" ");
  cmdIo.print(config.node_id);
  cmdIo.print(" ");
  cmdIo.print(config.version);
  cmdIo.print(" ");
  cmdIo.print(config.config_id);
  cmdIo.println();
}

void ctempconfig_sercmd(void) {
  Serial.print("Offset: ");
  Serial.println(config.temp_offset);
  Serial.print("K: ");
  Serial.println(config.temp_k/10);
  Serial.print("Raw: ");
  float temp_k = (float) config.temp_k / 10;
  int ctemp = internalTemp(config.temp_offset, temp_k);
  Serial.println(ctemp);
}

void ctempoffset_sercmd(void) {
  char c;
  parser >> c;
  int v = c;
  if( v > 127 ) {
    v -= 256;
  }
  config.temp_offset = v;
  Serial.print("New offset: ");
  Serial.println(config.temp_offset);
  writeConfig(config);
}

void ctemp_sercmd(void) {
  double t = ( internalTemp(config.temp_offset, config.temp_k) + config.temp_offset ) * config.temp_k ;
  Serial.println( t );
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
  // See Prototype/Node
  sprintf(node_id, "%s%i", static_config.node, static_config.node_id);
  if (config.temp_k == 0.0)
    config.temp_k = 1.0;
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
  rf24_start();
#endif // NRF24

#if _RTC
  rtc_init();
#endif

#if _DHT
  dht.begin();
#if DEBUG
  Serial.println("Initialized DHT");
  float t = dht.readTemperature();
  Serial.println(t);
  float rh = dht.readHumidity();
  Serial.println(rh);
#endif
#endif //_DHT

#if _AM2321
ac.uid();
#endif //_AM2321

#if _BMP085
  if(!bmp.begin())
  {
#if SERIAL_EN
    Serial.print("Missing BMP085");
#endif
  }
#endif // _BMP085

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
  //doConfig();

#if _DS
  int ds_search = 0;
  while (findDS18B20s(ds_search) == 1) ;
  updateDSCount(ds_search);
#endif

#ifdef _DBG_LED
  pinMode(_DBG_LED, OUTPUT);
#endif
  tick = 0;

#if _NRF24
  rf24_init();
#endif //_NRF24

  reportCount = REPORT_EVERY;     // report right away for easy debugging
  scheduler.timer(ANNOUNCE, ANNOUNCE_START); // get the measurement loop going
#if SERIAL_EN && DEBUG
  Serial.println("Reinitialized");
#endif
}

bool doAnnounce(void)
{
/* see SensorNode */
#if SERIAL_EN && DEBUG
  cmdIo.print("\n[");
  cmdIo.print(sketch);
  cmdIo.print(".");
  cmdIo.print(version);
  cmdIo.println("]");
#endif // SERIAL_EN && DEBUG
  cmdIo.print(node_id);
  cmdIo.print(' ');
#if LDR_PORT
  Serial.print("light:8 ");
#endif
#if PIR
  Serial.print("moved:1 ");
#endif
#if _DHT
  Serial.print(F("rhum:7 "));
  Serial.print(F("temp:10 "));
#endif
  Serial.print(F("ctemp:10 "));
#if _AM2321
  Serial.print(F("am2321rhum:7 "));
  Serial.print(F("am2321temp:9 "));
#endif //_AM2321
#if _BMP085
  Serial.print(F("bmp085pressure:12 "));
  Serial.print(F("bmp085temp:9 "));
#endif //_BMP085
#if _DS
  int ds_count = readDSCount();
  for ( int i = 0; i < ds_count; i++) {
    Serial.print(F("dstemp:7 "));
  }
#endif
#if _HMC5883L
#endif //_HMC5883L
#if _MEM
#if SERIAL_EN
  Serial.print(F("memfree:16 "));
#endif
#endif
#if _RFM12LOBAT
#if SERIAL_EN
  Serial.print(F("lobat:1 "));
  serialFlush();
#endif //if SERIAL_EN
#endif //_RFM12LOBAT
  Serial.println();
  return false;
}


// readout all the sensors and other values
void doMeasure(void)
{
  byte firstTime = payload.ctemp == 0; // special case to init running avg

  int ctemp = internalTemp(config.temp_offset, (float)config.temp_k/10);
  payload.ctemp = smoothedAverage(payload.ctemp, ctemp, firstTime);
#if SERIAL_EN && DEBUG_MEASURE
  Serial.print("Payload size: ");
  Serial.println(sizeof(payload));
  Serial.print("AVR T new/avg ");
  Serial.print(ctemp);
  Serial.print('/');
  Serial.println(payload.ctemp);
#endif

#if SHT11_PORT
#endif //SHT11_PORT

#if _AM2321
  int am2321 = ac.read();
  if (am2321 == 1) {
    payload.am2321temp = 10 + ac.temperature;
    payload.am2321rhum = round(ac.humidity/10);
#if SERIAL_EN && DEBUG_MEASURE
    Serial.print("AM2321 Raw rh/t: ");
    Serial.print(ac.humidity);
    Serial.print(' ');
    Serial.print(ac.temperature);
    Serial.println();
#endif
  } else {
#if SERIAL_EN
    Serial.println("Error reading AM2321");
#endif
  }
#if SERIAL_EN && DEBUG_MEASURE
  Serial.print("AM2321 Measure rh/t: ");
  Serial.print( payload.am2321rhum );
  Serial.print(' ');
  Serial.print( ( (float) payload.am2321temp - 10 ) / 10 ) ;
  Serial.println();
#endif
#endif //_AM2321

#if _BMP085
  sensors_event_t event;
  bmp.getEvent(&event);
  if (event.pressure) {
    payload.bmp085pressure = ( (event.pressure - 990 ) * 100 ) ;
#if SERIAL_EN && DEBUG_MEASURE
    Serial.print("BMP085 Measure pressure-raw/-ranged: ");
    Serial.print( event.pressure );
    Serial.print(' ');
    Serial.print( payload.bmp085pressure );
    Serial.println();
#endif
  } else {
#if SERIAL_EN
    Serial.println("Failed rading BMP085");
#endif
  }
  float bmp085temperature;
  bmp.getTemperature(&bmp085temperature);
  payload.bmp085temp = ( bmp085temperature + 10 ) * 10;
#if SERIAL_EN && DEBUG_MEASURE
    Serial.print("BMP085 Measure temp-raw/-ranged: ");
    Serial.print(bmp085temperature);
    Serial.print(' ');
    Serial.println( payload.bmp085temp );
#endif
#if SERIAL_EN && DEBUG_MEASURE
  Serial.print("BMP085 Pressure/temp: ");
  Serial.print( (float) 990 + ( (float) payload.bmp085pressure / 100 ) );
  Serial.print(' ');
  Serial.print( (float) ( (float) payload.bmp085temp / 10 ) - 10 );
  Serial.println();
#endif
#endif //_BMP085

#if PIR_PORT
  payload.moved = pir.state();
#endif
#if _DHT
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (isnan(h)) {
#if SERIAL_EN | DEBUG
    Serial.println(F("Failed to read DHT11 humidity"));
#endif
  } else {
    int rh = h * 10;
    payload.rhum = smoothedAverage(payload.rhum, rh, firstTime);
#if SERIAL_EN && DEBUG_MEASURE
    Serial.print(F("DHT RH new/avg "));
    Serial.print(rh);
    Serial.print(' ');
    Serial.println(payload.rhum);
#endif
  }
  if (isnan(t)) {
#if SERIAL_EN | DEBUG
    Serial.println(F("Failed to read DHT11 temperature"));
#endif
  } else {
    payload.temp = smoothedAverage(payload.temp, (int)(t * 10), firstTime);
#if SERIAL_EN && DEBUG_MEASURE
    Serial.print(F("DHT T new/avg "));
    Serial.print(t);
    Serial.print(' ');
    Serial.println(payload.temp);
#endif
  }
#endif //_DHT

#if LDR_PORT
  ldr.digiWrite2(1);  // enable AIO pull-up
  byte light = ~ ldr.anaRead() >> 2;
  ldr.digiWrite2(0);  // disable pull-up to reduce current draw
  payload.light = smoothedAverage(payload.light, light, firstTime);
#if SERIAL_EN && DEBUG_MEASURE
  Serial.print(F("LDR new/avg "));
  Serial.print(light);
  Serial.print(' ');
  Serial.println(payload.light);
#endif
#endif //_LDR_PORT

#if _DS
  int ds_count = readDSCount();
  byte addr[8];
  for ( int i = 0; i < ds_count; i++) {
    readDSAddr(i, addr);
    ds_value[i] = readDS18B20(addr);
#if SERIAL_EN && DEBUG_MEASURE
    Serial.print(F("DS18B20 #"));
    Serial.print(i+1);
    Serial.print('/');
    Serial.print(ds_count);
    Serial.print(' ');
    Serial.println( (float) ds_value[i] / 100 );
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
  Serial.print("MEM free ");
  Serial.println(payload.memfree);
#endif
#endif //_MEM

#if _RFM12LOBAT
  payload.lobat = rf12_lowbat();
#if SERIAL_EN && DEBUG_MEASURE
  if (payload.lobat) {
    Serial.println("Low battery");
  }
#endif
#endif //_RFM12LOBAT
}


// periodic report, i.e. send out a packet and optionally report on serial port
void doReport(void)
{
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
#endif // NRF24

#if SERIAL_EN
  /* Report over serial, same fields and order as announced */
  Serial.print(node_id);
#if LDR_PORT
  Serial.print(' ');
  Serial.print((int) payload.light);
#endif
#if PIR_PORT
  Serial.print((int) payload.moved);
  Serial.print(' ');
#endif
#if _DHT
  Serial.print((int) payload.rhum);
  Serial.print(' ');
  Serial.print((int) payload.temp);
  Serial.print(' ');
#endif
  Serial.print((int) payload.ctemp);
#if _AM2321
  Serial.print(' ');
  Serial.print((int) payload.am2321temp);
  Serial.print(' ');
  Serial.print((int) payload.am2321rhum);
#endif //_AM2321
#if _BMP085
  Serial.print(' ');
  Serial.print((int) payload.bmp085temp);
  Serial.print(' ');
  Serial.print((int) payload.bmp085pressure);
#endif //_BMP085
#if _DS
  int ds_count = readDSCount();
  for (int i=0;i<ds_count;i++) {
    Serial.print((int) ds_value[i]);
    Serial.print(' ');
  }
#endif
#if _HMC5883L
  Serial.print(' ');
  Serial.print(payload.magnx);
  Serial.print(' ');
  Serial.print(payload.magny);
  Serial.print(' ');
  Serial.print(payload.magnz);
#endif //_HMC5883L
#if _MEM
  Serial.print(' ');
  Serial.print((int) payload.memfree);
#endif //_MEM
#if _RFM12LOBAT
  Serial.print(' ');
  Serial.print((int) payload.lobat);
#endif //_RFM12LOBAT

  Serial.println();
#endif //SERIAL_EN
}

#if PIR_PORT

#endif // PIR_PORT


void runScheduler(char task)
{
  switch (task) {

    case ANNOUNCE:
#if SERIAL_EN
      debugline("ANNOUNCE");
#endif
      //Serial.print(strlen(node_id));
      //Serial.println();
      //if (strlen(node_id) > 0) {
      if (doAnnounce()) {
        //Serial.print("Node: ");
        //Serial.println(node_id);
        scheduler.timer(MEASURE, 0);
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
      Serial.print("!");
      serialFlush();
      break;

    default:
      Serial.print("0x");
      Serial.print(task, HEX);
      Serial.println(" ?");
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
  { 'o', 0, ctempconfig_sercmd },
  { 'T', 1, ctempoffset_sercmd },
  { 't', 0, ctemp_sercmd },
  { 'a', 0, announce_sercmd },
  { 'r', 0, report_sercmd },
  { 'M', 0, measure_sercmd },
  { 'E', 0, eraseEEPROM },
  { 'x', 0, reset_sercmd },
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
#endif //_NRF24

#if _DS
  bool ds_reset = digitalRead(7);
  if (ds_search || ds_reset) {
    if (ds_reset) {
      Serial.println("DS-reset triggered");
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
