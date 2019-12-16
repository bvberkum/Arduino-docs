/*
CarrierCase

- Generally based on roomNode, probably can be made compatible with other environmental sensor nodes.
- Two moisture probes.
- Low power scheduled node. No UI yet.

- Later to have shift register and analog switchting for mre inputs, perhaps.
  Make SDA/SCL free for I2C or other bus?

TODO: DallasTempBus
TODO: other wire, relay

Current pin assignments:

Ard Atm Jee  Sign Mapping
=== === ==== ==== ===========
D0  PD0      TXD  SER/DEBUG
D1  PD1      RXD  SER/DEBUG
D2  PD2      INT0 JeePort IRQ
D3  PD3      INT1 RFM12B IRQ
D4  PD4 DIO1      PIR
D5  PD5 DIO2
D6  PD6 DIO3
D7  PD7 DIO4      DHT11
D8  PB0           LED yellow
D9  PB1           LED orange
D10 PB2      SS   RFM12B SEL
D11 PB3      MOSI RFM12B SDI
D12 PB4      MISO RFM12B DSO
D13 PB5      SCK  RFM12B SCK
A0  PC1 AIO1
A1  PC1 AIO2
A2  PC2 AIO3
A1  PC3 AIO4      LDR
A4  PC4      SDA
A5  PC5      SCL
    PC6      RST  Reset
=== === ==== ==== ===========


Todo:

Ard Atm Jee  Sign Mapping
=== === ==== ==== ===========
D4  PD4 DIO1      MP-1 RIGHT
D5  PD5 DIO2      MP-1 LEFT
D6  PD6 DIO3      MP-2 RIGHT
D7  PD7 DIO4      MP-2 LEFT
D8  PB0           MP-1 LED
D9  PB1           MP-2 LED
A0  PC1 AIO1      MP1 measure
A1  PC1 AIO2      MP2 measure
A2  PC2 AIO3
A1  PC3 AIO4      LDR
A4  PC4      SDA  Dallas bus
A5  PC5      SCL  DHT out
=== === ==== ==== ===========

ToDo
  - Announce payload config

    spec:
    detect:
      PIR
        motion
      LDR

 */

#define DEBUG_DHT 1
#define _EEPROMEX_DEBUG 1  // Enables logging of maximum of writes and out-of-memory

/* *** Globals and sketch configuration *** */
#define SERIAL_EN         1 /* Enable serial */
#define DEBUG             1 /* Enable trace statements */

#define _DHT              0
#define LDR_PORT          0 // defined if LDR is connected to a port's AIO pin
#define _DS               0
#define DEBUG_DS          1
//#define FIND_DS    1
#define DHT_PIN           0 // DIO for DHTxx
#define DHT_HIGH          1 // enable for DHT22/AM2302, low for DHT11
#define PIR_PORT          0
#define _HMC5883L         1
#define _LCD84x48         0
#define _RFM12B           0
#define _RFM12LOBAT       1 // Use JeeNode lowbat measurement
#define _NRF24            0

#define REPORT_EVERY      5 // report every N measurement cycles
#define SMOOTH            5 // smoothing factor used for running averages
#define MEASURE_PERIOD    50 // how often to measure, in tenths of seconds
#define RETRY_PERIOD      10 // how soon to retry if ACK didn't come in
#define RETRY_LIMIT       2 // maximum number of times to retry
#define ACK_TIME          10 // number of milliseconds to wait for an ack
#define UI_SCHED_IDLE     4000 // tenths of seconds idle time, ...
#define UI_STDBY          8000 // ms
#define MAXLENLINE        79
#define DS_EEPROM_OFFSET  96
#define CONFIG_VERSION "cc1"
#define CONFIG_EEPROM_START 50
//#define SRAM_SIZE       0x800 // atmega328, for debugging
// set the sync mode to 2 if the fuses are still the Arduino default
// mode 3 (full powerdown) can only be used with 258 CK startup fuses
#define RADIO_SYNC_MODE 2





#include <stdlib.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <util/atomic.h>

#include <Wire.h>
#include <OneWire.h>
#include <EEPROM.h>
// include EEPROMEx.h
#include <JeeLib.h>
#include <SPI.h>
#include <RF24.h>
#include <RF24Network.h>
#if _DHT
#include <Adafruit_Sensor.h>
#include <DHT.h> // Adafruit DHT
#endif // DHT
#include <DotmpeLib.h>
#include <mpelib.h>
#include <EmBencode.h>




const String sketch = "CarrierCase";
const int version = 0;

char node[] = "cc-1";
char node_id[7];


/* IO pins */
//              RXD      0
//              TXD      1
//              INT0     2
#       define _DBG_LED2 8 // B0, yellow
#       define _DBG_LED 9  // B1, orange
#       define BL       10 // PWM Backlight
//              MOSI     11
//              MISO     12
//              SCK 13
#define DS_PIN          A5


MpeSerial mpeser (57600);

//MilliTimer idle, stdby;

/* *** InputParser *** {{{ */

Stream& cmdIo = Serial;
extern const InputParser::Commands cmdTab[] PROGMEM;
byte* buffer = (byte*) malloc(50);
const InputParser parser (buffer, 50, cmdTab);


/* *** /InputParser *** }}} */

/* *** Report variables *** {{{ */


enum { ANNOUNCE_MSG, REPORT_MSG };

static byte reportCount;    // count up until next report, i.e. packet send
static byte rf12_id;       // node ID used for this unit

/* Data reported by this sketch */
struct {
  int msgtype     :8; // XXX: incorporated, but not the right place..
#if LDR_PORT
  byte light      :8;     // light sensor: 0..255
#endif
#if PIR_PORT
  byte moved      :1;  // motion detector: 0..1
#endif

#ifdef _DHT
  int rhum        :7;  // rhumdity: 0..100 (4 or 5% resolution?)
#if DHT_HIGH
/* DHT22/AM2302: 20% to 100% @ 2% rhum, -40C to 80C @ ~0.5 temp */
  int temp    :10; // -500..+500 (int value of tenths avg)
#else
/* DHT11: 20% to 80% @ 5% rhum, 0C to 50C @ ~2C temp */
  int temp    :10; // -500..+500 (tenths, .5 resolution)
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
  byte lobat      :1;  // supply voltage dropped under 3.1V: 0..1
#endif
} payload;


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

// See Prototype Node or SensorNode
const long maxAllowedWrites = 100000; /* if evenly distributed, the ATmega328 EEPROM
should have at least 100,000 writes */
const int memBase          = 0;
//const int memCeiling       = EEPROMSizeATmega328;

struct Config {
  char node[4]; // Alphanumeric Id (for sketch)
  int node_id; // Id suffix integer (for unique run-time Id)
  int version;
  char config_id[4];
  signed int temp_offset;
  float temp_k;
} static_config = {
  /* default values */
  { node[0], node[1], 0, 0 }, 0, version,
  CONFIG_VERSION, 0, 1.0,
};

Config config;

bool loadConfig(Config &c)
{
  unsigned int w = sizeof(c);

  if (
      //EEPROM.read(CONFIG_EEPROM_START + w - 1) == c.config_id[3] &&
      //EEPROM.read(CONFIG_EEPROM_START + w - 2) == c.config_id[2] &&
      EEPROM.read(CONFIG_EEPROM_START + w - 3) == c.config_id[1] &&
      EEPROM.read(CONFIG_EEPROM_START + w - 4) == c.config_id[0]
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
#if SERIAL_EN && DEBUG
    // verify
    if (EEPROM.read(CONFIG_EEPROM_START + t) != *((char*)&c + t))
    {
      // error writing to EEPROM
      cmdIo.println("Error writing "+ String(t)+" to EEPROM");
    }
#endif
  }
#if _RFM12B
  eeprom_write_byte(RF12_EEPROM_ADDR, 4);
  eeprom_write_byte(RF12_EEPROM_ADDR+1, 5);
  //  eeprom_write_byte(RF12_EEPROM_ADDR+2, );
  // JeeLib RF12_EEPROM_ADDR is at 20
  int nodeId = eeprom_read_byte(RF12_EEPROM_ADDR);
  int group = eeprom_read_byte(RF12_EEPROM_ADDR + 1);

  cmdIo.print("nodeId: ");
  cmdIo.println(nodeId);
  cmdIo.print("group: ");
  cmdIo.println(group);
  serialFlush();
#endif //_RFM12B
}


/* *** /EEPROM config *** }}} */

/* *** Scheduler routines *** {{{ */

// has to be defined because we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

/* *** /Scheduler routines *** }}} */

/* *** Peripheral devices *** {{{ */

#if SHT11_PORT
SHT11 sht11 (SHT11_PORT);
#endif

#if LDR_PORT
Port ldr (LDR_PORT);
#endif

/* *** PIR *** {{{ */
#if PIR_PORT

#define PIR_INVERTED    0   // 0 or 1, to match PIR reporting high or low
#define PIR_HOLD_TIME   30  // hold PIR value this many seconds after change

/// Interface to a Passive Infrared motion sensor.
class PIR : public Port {
  volatile byte value, changed;
  volatile uint32_t lastOn;
public:
  PIR (byte portnum)
    : Port (portnum), value (0), changed (0), lastOn (0) {}

  // this code is called from the pin-change interrupt handler
  void poll() {
    // see http://talk.jeelabs.net/topic/811#post-4734 for PIR_INVERTED
    byte pin = digiRead() ^ PIR_INVERTED;
    // if the pin just went on, then set the changed flag to report it
    if (pin) {
      if (!state()) {
        changed = 1;
      }
      lastOn = millis();
    }
    value = pin;
  }

  // state is true if curr value is still on or if it was on recently
  byte state() const {
    byte f = value;
    if (lastOn > 0)
      ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        if (millis() - lastOn < 1000 * PIR_HOLD_TIME)
          f = 1;
      }
    return f;
  }

  // return true if there is new motion to report
  byte triggered() {
    byte f = changed;
    changed = 0;
    return f;
  }
};

PIR pir (PIR_PORT);

// the PIR signal comes in via a pin-change interrupt
ISR(PCINT2_vect) { pir.poll(); }

#endif
/* *** /PIR *** }}} */

#if _DHT
/* DHTxx: Digital temperature/humidity (Adafruit) */
//DHTxx dht (DHT_PIN); // JeeLib DHT

#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT22   // DHT 22  (AM2302)

DHT dht (DHT_PIN, DHTTYPE); // DHT lib
#endif // DHT

#if _AM2321
#endif

#if _LCD84x48
/* Nokkia 5110 display */
#endif // LCD84x48

#if _DS
/* Dallas OneWire bus with registration for DS18B20 temperature sensors */

OneWire ds(DS_PIN);

uint8_t ds_count = 0;
uint8_t ds_search = 0;
volatile int ds_value[ds_count];
volatile int ds_value[8]; // take on 8 DS sensors in report

enum { DS_OK, DS_ERR_CRC };

#endif // DS

#if _RFM12B
/* HopeRF RFM12B 868Mhz digital radio */


#endif // RFM12B

#if _NRF24
/* nRF24L01+: nordic 2.4Ghz digital radio */


#endif // NRF24

#if _RTC
/* DS1307: Real Time Clock over I2C */
#endif // RTC

#if _HMC5883L
/* Digital magnetometer I2C module */

/* The I2C address of the module */
#define HMC5803L_Address 0x1E

/* Register address for the X Y and Z data */
#define X 3
#define Y 7
#define Z 5

#endif //_HMC5883L

/* Report variables */


/* *** /Peripheral devices *** }}} */

// spend a little time in power down mode while the SHT11 does a measurement
static void lpDelay () {
  Sleepy::loseSomeTime(32); // must wait at least 20 ms
}


//String serialBuffer = "";         // a string to hold incoming data

//void serialEvent() {
//  while (cmdIo.available()) {
//    // get the new byte:
//    char inchar = (char)cmdIo.read();
//    // add it to the serialBuffer:
//    if (inchar == '\n') {
//      serialBuffer = "";
//    }
//    else if (serialBuffer == "new ") {
//      node_id += inchar;
//    }
//    else {
//      serialBuffer += inchar;
//    }
//  }
//}


/* embenc for storage and xfer proto */

uint8_t* embencBuff;
int embencBuffLen = 0;

// dynamic allocation for embencBuff
void embenc_push(char ch) {
  embencBuffLen += 1;
  uint8_t* np = (uint8_t *) realloc( embencBuff, sizeof(uint8_t) * embencBuffLen);
  if( np != NULL ) {
    embencBuff = np;
    embencBuff[embencBuffLen] = ch;
  } else {
    cmdIo.println(F("Out of Memory"));
  }
}

void EmBencode::PushChar(char ch) {
  cmdIo.write(ch);
  embenc_push(ch);
}

void freeEmbencBuff() {
  embencBuff = (uint8_t *) realloc(embencBuff, 0); // free??
  embencBuffLen = 0;
}


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

  serialFlush();
  Sleepy::loseSomeTime(800);
  //delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.

  present = ds.reset();
  ds.select(addr);
  ds.write(0xBE);         // Read Scratchpad

#if SERIAL_EN && DEBUG_DS
  cmdIo.print(F("Data="));
  cmdIo.print(present,HEX);
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

// FIXME: returns 8500 value at times, drained parasitic power?
static int readDS18B20(uint8_t addr[8]) {
  byte data[12];
  int SignBit;

  int result = ds_readdata(addr, data);

  if (result != DS_OK) {
#if SERIAL_EN
    cmdIo.println(F("CRC error in ds_readdata"));
    serialFlush();
#endif
    return 0;
  }

  int Tc_100 = ds_conv_temp_c(data, SignBit);

  if (SignBit) {
    return 0 - Tc_100;
  } else {
    return Tc_100;
  }
}

static uint8_t readDSCount() {
  uint8_t ds_count = EEPROM.read(3);
  if (ds_count == 0xFF) return 0;
  return ds_count;
}

static void updateDSCount(uint8_t new_count) {
  if (new_count != ds_count) {
    EEPROM.write(3, new_count);
    ds_count = new_count;
  }
}

static void writeDSAddr(uint8_t addr[8]) {
  int l = 4 + ( ( ds_search-1 ) * 8 );
  for (int i=0;i<8;i++) {
    EEPROM.write(l+i, addr[i]);
  }
}

static void readDSAddr(int a, uint8_t addr[8]) {
  int l = 4 + ( a * 8 );
  for (int i=0;i<8;i++) {
    addr[i] = EEPROM.read(l+i);
  }
}

static void printDSAddrs(void) {
  for (int q=0;q<ds_count;q++) {
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
}

static void findDS18B20s(void) {
  byte i;
  byte addr[8];

  if (!ds.search(addr)) {
#if SERIAL_EN && DEBUG_DS
    cmdIo.println("No more addresses.");
#endif
    ds.reset_search();
    if (ds_search != ds_count) {
#if DEBUG || DEBUG_DS
      cmdIo.print("Found ");
      cmdIo.print(ds_search );
      cmdIo.println(" addresses.");
      cmdIo.print("Previous search found ");
      cmdIo.print(ds_count);
      cmdIo.println(" addresses.");
#endif
    }
    updateDSCount(ds_search);
    ds_search = 0;
    return;
  }

  if ( OneWire::crc8( addr, 7) != addr[7]) {
#if DEBUG || DEBUG_DS
    cmdIo.println("CRC is not valid!");
#endif
    return;
  }

  ds_search++;

#if DEBUG || DEBUG_DS
  cmdIo.print("New Address=");
  for( i = 0; i < 8; i++) {
    cmdIo.print(i);
    cmdIo.print(':');
    cmdIo.print(addr[i], HEX);
    cmdIo.print(" ");
  }
  cmdIo.println();
#endif

  writeDSAddr(addr);
}


#endif // DS

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

// XXX: just an example, not actually used
static void sendSomeData () {
  EmBencode encoder;
  // send a simple string
  encoder.push("abcde");
  // send a number of bytes, could be binary
  encoder.push("123", 3);
  // send an integer
  encoder.push(12345);
  // send a list with an int, a nested list, and an int
  encoder.startList();
    encoder.push(987);
    encoder.startList();
      encoder.push(654);
    encoder.endList();
    encoder.push(321);
  encoder.endList();
  // send a large integer
  encoder.push(999999999);
  // send a dictionary with two entries
  encoder.startDict();
    encoder.push("one");
    encoder.push(11);
    encoder.push("two");
    encoder.push(22);
  encoder.endDict();
  // send one last string
  encoder.push("bye!");
}

#endif // NRF24 funcs

#if _RTC
#endif // RTC

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
  cmdIo.println("o: print temp. cfg.");
  cmdIo.println("E: erase EEPROM!");
  cmdIo.println("r: report now");
  cmdIo.println("m: measure now");
  cmdIo.println("x: reset");
  cmdIo.println("?/h: this help");
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
  cmdIo.print("Offset: ");
  cmdIo.println(config.temp_offset);
  cmdIo.print("K: ");
  cmdIo.println(config.temp_k);
  cmdIo.print("Raw: ");
  cmdIo.println(internalTemp(config.temp_offset, config.temp_k));
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
  double t = ( internalTemp(config.temp_offset, config.temp_k) + config.temp_offset ) * config.temp_k ;
  cmdIo.println( t );
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
  sprintf(node_id, "%s%i", config.node, config.node_id);
}

void doConfig(void)
{
  /* load valid config or reset default config */
  if (!loadConfig(config)) {
    writeConfig(config);
  }
  initConfig();

#if _RFM12B
  // JeeLib RF12_EEPROM_ADDR is at 20
  int nodeId = eeprom_read_byte(RF12_EEPROM_ADDR);
  int group = eeprom_read_byte(RF12_EEPROM_ADDR + 1);
#endif //_RFM12B

  return;

  // Read embencoded from eeprom
  int specByteCnt= eeprom_read_byte(RF12_EEPROM_ADDR + 2);
  uint8_t specRaw [specByteCnt];
  eeprom_read_block(
      (void*)specRaw,
      RF12_EEPROM_ADDR + 3,
      specByteCnt
    );

  char embuf [200];
  EmBdecode decoder (embuf, sizeof embuf);
  uint8_t bytes;
  int i = 0;
  // XXX: fix this, catch longer than scenario, check for remaining raw spec
  while (!bytes) {
    bytes = decoder.process(specRaw[i]);
    i += 1;
  }

  if (!bytes) {
    cmdIo.println("Unable to decode");
  } else {
    cmdIo.print(" => ");
    cmdIo.print((int) bytes);
    cmdIo.println(" bytes");
    for (;;) {
      uint8_t token = decoder.nextToken();
      if (token == EmBdecode::T_END)
        break;
      switch (token) {
        case EmBdecode::T_STRING:
          cmdIo.print(" string: ");
          cmdIo.println(decoder.asString());
          break;
        case EmBdecode::T_NUMBER:
          cmdIo.print(" number: ");
          cmdIo.println(decoder.asNumber());
          break;
        case EmBdecode::T_DICT:
          cmdIo.println(" > dict");
          break;
        case EmBdecode::T_LIST:
          cmdIo.println(" > list");
          break;
        case EmBdecode::T_POP:
          cmdIo.println(" < pop");
          break;
      }
    }
    decoder.reset();
  }
}

void initLibs(void)
{
#if _NRF24
  radio_init();
  rf24_start();
#endif // NRF24

#if _RTC
#endif //_RTC

#if _DHT
  dht.begin();
#if DEBUG
  cmdIo.println("Initialized DHT");
  float t = dht.readTemperature();
  cmdIo.println(t);
  float rh = dht.readHumidity();
  cmdIo.println(rh);
#endif
#endif //_DHT

#if _AM2321
#endif //_AM2321

#if _BMP085
  if(!bmp.begin())
  {
#if SERIAL_EN
    cmdIo.print("Missing BMP085");
#endif
  }
#endif // _BMP085

#if _HMC5883L
  /* Initialise the Wire library */
  Wire.begin();

  /* Initialise the module */
  Init_HMC5803L();
#endif //_HMC5883L

#if SERIAL_EN && DEBUG
  //printf_begin();
#endif
}


/* *** /Initialization routines *** }}} */

/* *** Run-time handlers *** {{{ */

void doReset(void)
{
  doConfig();

#if _DS
  int ds_search = 0;
  while (findDS18B20s(ds_search) == 1) ;
  updateDSCount(ds_search);
#endif

#ifdef _DBG_LED2
  pinMode(_DBG_LED2, OUTPUT);
  blink(_DBG_LED2, 3, 100);
#endif

#ifdef _DBG_LED
  pinMode(_DBG_LED, OUTPUT);
#endif
  tick = 0;
  reportCount = REPORT_EVERY;     // report right away for easy debugging
  scheduler.timer(MEASURE, 0);    // start the measurement loop going
}

/* After init, before going to regular scheduling first confer config with
 * central node */
bool doAnnounce(void)
{
#if SERIAL_EN

  //mpeser.startAnnounce(sketch, version);

#if DEBUG
  cmdIo.print("\n[");
  cmdIo.print(sketch);
  cmdIo.print(".");
  cmdIo.print(version);
  cmdIo.println("]");
#endif // DEBUG
  cmdIo.print(node_id);
  cmdIo.print(' ');
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
#if _AM2321
  cmdIo.print(F("am2321rhum:7 "));
  cmdIo.print(F("am2321temp:9 "));
#endif //_AM2321
#if _BMP085
  cmdIo.print(F("bmp085pressure:12 "));
  cmdIo.print(F("bmp085temp:9 "));
#endif //_BMP085
#if _DS
  ds_count = readDSCount();
  for ( int i = 0; i < ds_count; i++) {
    cmdIo.print(F("dstemp:7 "));
    //cmdIo.print(" ds-");
    //cmdIo.print(i+1);
  }
#endif

#endif //SERIAL_EN


  embenc_push(ANNOUNCE_MSG);

  EmBencode payload_spec;

  payload_spec.startList();
#if LDR_PORT
  payload_spec.startList();
  // name, may be unique but can be part number and as specific or generic as possible
  payload_spec.push("ldr2250");
  // bits in payload
  payload_spec.push(8);
  // XXX decoder params, optional?
  payload_spec.push("int(0-255)");
  payload_spec.endList();
#if SERIAL_EN
  cmdIo.print(F(" ldr"));
#endif
#endif
#if PIR_PORT
  payload_spec.startList();
  payload_spec.push("moved");
  payload_spec.push(1);
  payload_spec.endList();
#if SERIAL_EN
  cmdIo.print("moved:1 ");
#endif
#endif
#if _DHT
  payload_spec.startList();
  payload_spec.push("dht11-rhum");
  payload_spec.push(7);
  payload_spec.endList();
#if SERIAL_EN
  cmdIo.print(F(" dht11-rhum dht11-temp"));
#endif
#endif // _DHT
  payload_spec.startList();
  payload_spec.push("ctemp");
  payload_spec.push(10);
  payload_spec.endList();
#if SERIAL_EN
  cmdIo.print(F(" ctemp"));
#endif
#if _MEM
  payload_spec.startList();
  payload_spec.push("memfree");
  payload_spec.push(16);
  payload_spec.endList();
#if SERIAL_EN
  cmdIo.print(F("memfree:16 "));
#endif
#endif
  payload_spec.startList();
  payload_spec.push("lobat");
  payload_spec.endList();
#if SERIAL_EN
  cmdIo.print(F("lobat:1 "));
  serialFlush();
#endif //if SERIAL_EN
  payload_spec.endList();

  cmdIo.println();
  for (int i=0; i<embencBuffLen; i++) {
    if (i > 0) cmdIo.print(' ');
    cmdIo.print((int)embencBuff[i]);
  }
  cmdIo.println();
  cmdIo.print(F("Sending"));
  cmdIo.print((int)embencBuffLen);
  cmdIo.println(F("bytes"));

  if (embencBuffLen > 66) {
  // FIXME: need to send in chunks
  }

#if _RFM12B
  rf12_sleep(RF12_WAKEUP);
  rf12_sendNow(
    (rf12_id & RF12_HDR_MASK) | RF12_HDR_ACK,
    &embencBuff,
    sizeof embencBuff);
  rf12_sendWait(RADIO_SYNC_MODE);
  bool acked = waitForAck();

  acked = 1;

  freeEmbencBuff();

  if (acked) {
//    cmdIo.println("ACK");
//  } else {
//    cmdIo.println("NACK");
  }
  cmdIo.print(F("Free RAM: "));
  cmdIo.println(freeRam());

  rf12_sleep(RF12_SLEEP);
  return acked;
#endif //_RFM12B
#if _DS
      // report a new node or reinitialize node with central link node
//      for ( int x=0; x<ds_count; x++) {
//        cmdIo.print("SEN ");
//        cmdIo.print(node_id);
//        cmdIo.print(" ds-");
//        cmdIo.print(x);
//        for ( int y= 0; y<9; y++) {           // we need 9 bytes
//          cmdIo.print(" ");
//          cmdIo.print(ds_addr[x][y], HEX);
//        }
//        cmdIo.println("");
//      }
#endif
  return false;
}


// readout all the sensors and other values
void doMeasure(void)
{
  byte firstTime = payload.ctemp == 0; // special case to init running avg

  int ctemp = internalTemp(config.temp_offset, config.temp_k);
  payload.ctemp = smoothedAverage(payload.ctemp, ctemp, firstTime);
#if SERIAL_EN && DEBUG_MEASURE
  cmdIo.print("Payload size: ");
  cmdIo.println(sizeof(payload));
  cmdIo.print("AVR T new/avg ");
  cmdIo.print(ctemp);
  cmdIo.print(' ');
  cmdIo.println(payload.ctemp);
#endif

#if SHT11_PORT
#endif //SHT11_PORT

#if _AM2321
#endif //_AM2321

#if _BMP085
  sensors_event_t event;
  bmp.getEvent(&event);
  if (event.pressure) {
    payload.bmp085pressure = ( (event.pressure - 990 ) * 100 ) ;
#if SERIAL_EN && DEBUG_MEASURE
    cmdIo.print("BMP085 Measure pressure-raw/-ranged: ");
    cmdIo.print( event.pressure );
    cmdIo.print(' ');
    cmdIo.print( payload.bmp085pressure );
    cmdIo.println();
#endif
  } else {
#if SERIAL_EN
    cmdIo.println("Failed rading BMP085");
#endif
  }
  float bmp085temperature;
  bmp.getTemperature(&bmp085temperature);
  payload.bmp085temp = ( bmp085temperature + 10 ) * 10;
#if SERIAL_EN && DEBUG_MEASURE
    cmdIo.print("BMP085 Measure temp-raw/-ranged: ");
    cmdIo.print(bmp085temperature);
    cmdIo.print(' ');
    cmdIo.println( payload.bmp085temp );
#endif
#if SERIAL_EN && DEBUG_MEASURE
  cmdIo.print("BMP085 Pressure/temp: ");
  cmdIo.print( (float) 990 + ( (float) payload.bmp085pressure / 100 ) );
  cmdIo.print(' ');
  cmdIo.print( (float) ( (float) payload.bmp085temp / 10 ) - 10 );
  cmdIo.println();
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
    cmdIo.println(F("Failed to read DHT11 humidity"));
#endif
  } else {
    payload.rhum = smoothedAverage(payload.rhum, round(h), firstTime);
#if SERIAL_EN && DEBUG_MEASURE
    cmdIo.print(F("DHT RH new/avg "));
    cmdIo.print(h);
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
  //ldr.digiWrite2(1);  // enable AIO pull-up
  byte light = ~ ldr.anaRead() >> 2;
  ldr.digiWrite2(0);  // disable pull-up to reduce current draw
  payload.light = smoothedAverage(payload.light, light, firstTime);
#endif //LDR_PORT

#if _DS
  uint8_t addr[8];
  for ( int i = 0; i < ds_count; i++) {
    readDSAddr(i, addr);
    ds_value[i] = readDS18B20(addr);
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
  cmdIo.print(node_id);
#if LDR_PORT
  cmdIo.print(' ');
  cmdIo.print((int) payload.light);
#endif
#if PIR_PORT
  cmdIo.print((int) payload.moved);
  cmdIo.print(' ');
#endif
#if _DHT
  cmdIo.print((int) payload.rhum);
  cmdIo.print(' ');
  cmdIo.print((int) payload.temp);
  cmdIo.print(' ');
#endif
  cmdIo.print((int) payload.ctemp);
#if _AM2321
  cmdIo.print(' ');
  cmdIo.print((int) payload.am2321temp);
  cmdIo.print(' ');
  cmdIo.print((int) payload.am2321rhum);
#endif //_AM2321
#if _BMP085
  cmdIo.print(' ');
  cmdIo.print((int) payload.bmp085temp);
  cmdIo.print(' ');
  cmdIo.print((int) payload.bmp085pressure);
#endif //_BMP085
#if _DS
  int ds_count = readDSCount();
  for (int i=0;i<ds_count;i++) {
    cmdIo.print((int) ds_value[i]);
    cmdIo.print(' ');
  }
#endif
#if _HMC5883L
  cmdIo.print(' ');
  cmdIo.print(payload.magnx);
  cmdIo.print(' ');
  cmdIo.print(payload.magny);
  cmdIo.print(' ');
  cmdIo.print(payload.magnz);
#endif //_HMC5883L
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
}

#if PIR_PORT

// send packet and wait for ack when there is a motion trigger
void doTrigger(void)
{
#if DEBUG
  cmdIo.print("PIR ");
  cmdIo.print((int) payload.moved);
  cmdIo.print(' ');
  cmdIo.print((int) pir.lastOn);
  cmdIo.println(' ');
  serialFlush();
#endif

#if _RFM12B
  for (byte i = 0; i < RETRY_LIMIT; ++i) {
    rf12_sleep(RF12_WAKEUP);
    rf12_sendNow(RF12_HDR_ACK, &payload, sizeof payload);
    rf12_sendWait(RADIO_SYNC_MODE);
    byte acked = waitForAck();
    rf12_sleep(RF12_SLEEP);

    if (acked) {
#if DEBUG
      cmdIo.print(" ack ");
      cmdIo.println((int) i);
      serialFlush();
#endif
      // reset scheduling to start a fresh measurement cycle
      scheduler.timer(MEASURE, MEASURE_PERIOD);
      return;
    }

    delay(RETRY_PERIOD * 100);
  }
#endif
  scheduler.timer(MEASURE, MEASURE_PERIOD);
#if DEBUG
  cmdIo.println(" no ack!");
  serialFlush();
#endif
}
#endif // PIR_PORT

// Add UI or not..
//void uiStart()
//{
//  idle.set(UI_SCHED_IDLE);
//  if (!ui) {
//    ui = true;
//  }
//}

void runScheduler(char task)
{
  switch (task) {

    case ANNOUNCE:
#if SERIAL_EN
      debugline("ANNOUNCE");
#endif
      if (doAnnounce()) {
        scheduler.timer(MEASURE, 0);
      } else {
        scheduler.timer(ANNOUNCE, 100);
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
      blink(_DBG_LED, 1, 50);
#endif
      break;

    case REPORT:
      payload.msgtype = REPORT_MSG;
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
  { 'o', 0, ctempconfig_sercmd },
  { 'T', 1, ctempoffset_sercmd },
  { 't', 0, ctemp_sercmd },
  { 'a', 0, doAnnounce },
  { 'r', 0, doReport },
  { 'm', 0, doMeasure },
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
//#if DEBUG
//  byte rf12_show = 1;
//#else
//  byte rf12_show = 0;
//#endif
#endif

  //writeConfig();
  //return;

  // Reported on serial?
  //node_id = "CarrierCase.0-2-RF12-5-23";
  //<sketch>.0-<count>-RF12....
  rf12_id = 23;
  rf12_initialize(rf12_id, RF12_868MHZ, 5);
  //rf12_config(rf12_show);

  rf12_sleep(RF12_SLEEP); // power down

#if PIR_PORT
  // PIR pull down and interrupt
  pir.digiWrite(0);
  // PCMSK2 = PCIE2 = PCINT16-23 = D0-D7
  bitSet(PCMSK2,  3 + PIR_PORT); // DIO1
  bitSet(PCICR, PCIE2); // enable PCMSK2 for PCINT at DIO1-4 (D4-7)
#endif

  /* warn out of bound if _EEPROMEX_DEBUG */
  //EEPROM.setMemPool(memBase, memCeiling);
  /* warn if _EEPROMEX_DEBUG */
  //EEPROM.setMaxAllowedWrites(maxAllowedWrites);

  /* read first config setting from memory, if empty start first-run init */

//  uint8_t nid = EEPROM.read(0);
//  if (nid == 0x0) {
//#if DEBUG
//    cmdIo.println("");
//#endif
//  }
  cmdIo.println();
  serialFlush();

  initLibs();

  doReset();
}

void loop(void)
{

  //measure_sercmd();
  //doReport();
  //delay(15000);
  //return;

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

#if PIR_PORT
  if (pir.triggered()) {
    payload.moved = pir.state();
    doTrigger();
  }
#endif

  parser.poll();

  char task = scheduler.poll();
  if (-1 < task && task < SCHED_IDLE) {
    runScheduler(task);
    //return;
  }

    /* Sleep */
    serialFlush();
    task = scheduler.pollWaiting();
    if (-1 < task && task < SCHED_IDLE) {
      runScheduler(task);
    }
}

/* *** }}} */
