/*
Cassette328P

An atmega328p TQFP chip with various peripherals:

- Leds (green, yellow, red) at 5, 6 and 7 (PD on the m328p, 5 and 6 have PWM)
- Nordic nRF24L01+ is at ISP plus D8 and D9 (PB0 and PB1), RF24 IRW is not used?
- LCD at hardware I2C port with address 0x20
- RTC at I2C 0x68
- DHT11 at A1 (PC1)

* Voltage is 3.3v and speed 16Mhz (ie. out of spec), external ceramic resonator.
  Hope this will keep working as such.
* Arduino library is used while I'm learning from chip datasheets,
  perhaps where needed there is time for more efficient and low-power
  implementation.

Plans
  - Hookup a low-pass filter and play with PWM, sine generation at D10 (m328p: PB2).

Research
  - Perhaps enable DEBUG/SERIAL_EN modes internally, wonder what it does with the sketch size, no experience there.
    Could use free ADC pin to measure level for verbosity, grey encoding would be cool witht he proper visuals.
  - Optionally enable LCD when present, also, make control panel to go with display, using attiny85 as I2C slave.
    Some buttons for menu. Perhaps touchscreen controller, remote reset.

Free pins
  - D3  PD3 PWM
  - D4  PD4
  - D10 PB2 PWM
  - D20 A6 ADC6

 */



/* *** Globals and sketch configuration *** */
#define SERIAL_EN         1 /* Enable serial */
#define DEBUG             0 /* Enable trace statements */
#define DEBUG_MEASURE     0

#define _MEM              0 // Report free memory
//#define LDR_PORT        0 // defined if LDR is connected to a port's AIO pin
#define _DHT              1
#define DHT_HIGH          0
#define _DS               0
#define _LCD              0
#define _LCD84x48         0
#define _RTC              0
#define _RFM12B           0
#define _RFM12LOBAT       0
#define _NRF24            1
#define _DBG_LED          0

#define REPORT_EVERY      5   // report every N measurement cycles
#define SMOOTH            5   // smoothing factor used for running averages
#define MEASURE_PERIOD    10  // how often to measure, in tenths of seconds
#define RETRY_PERIOD      10  // how soon to retry if ACK didn't come in
#define RETRY_LIMIT       5   // maximum number of times to retry
#define ACK_TIME          10  // number of milliseconds to wait for an ack
#define RADIO_SYNC_MODE   2

//#define TEMP_OFFSET     -57
//#define TEMP_K          1.0
#define ANNOUNCE_START    0
#define UI_SCHED_IDLE         4000  // tenths of seconds idle time, ...
#define UI_STDBY          8000  // ms
#define CONFIG_VERSION "cs1"
#define CONFIG_EEPROM_START 0
#define NRF24_CHANNEL     90



#include <EEPROM.h>
#include <JeeLib.h>
#include <Wire.h>
#include <SPI.h>
#include <RF24.h>
#include <RF24Network.h>
//include <LiquidCrystalTmp_I2C.h>
//#include <SimpleFIFO.h> //code.google.com/p/alexanderbrevig/downloads/list
#include <DHT.h> // Adafruit DHT
#include <DotmpeLib.h>
#include <mpelib.h>
#include "printf.h"




const String sketch = "Cassette328P";
const int version = 0;

char node[] = "cs";
char node_id[7];

volatile bool ui_irq;
bool ui;


/* IO pins */
//              RXD      0
//              TXD      1
//              INT0     2
static const int LED_RED       =  5;
static const int LED_YELLOW    =  6;
static const int LED_GREEN     =  7;
#       define CSN       8  // NRF24
#       define CE        9  // NRF24
#if _DHT
static const byte DHT_PIN = 15;
#endif
//#       define _DBG_LED 13 // SCK


MpeSerial mpeser (57600);

MilliTimer idle, stdby;

/* *** InputParser *** {{{ */

Stream& cmdIo = Serial;
extern const InputParser::Commands cmdTab[] PROGMEM;
const InputParser parser (50, cmdTab);


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
#if _MEM
  int memfree     :16;
#endif
#if _RFM12LOBAT
  byte lobat      :1; // supply voltage dropped under 3.1V: 0..1
#endif
//#if _RTC
// XXX: datetime?
//#endif
} payload;

/* Roles supported by this sketch */
typedef enum {
  role_reporter = 1,  /* start enum at 1, 0 is reserved for invalid */
  role_logger
} Role;

const char* role_friendly_name[] = {
  "invalid",
  "Reporter",
  "Logger"
};

Role role;// = role_logger;


/* *** /Report variables *** }}} */

/* *** Scheduled tasks *** {{{ */

enum {
  ANNOUNCE,
  DISCOVERY,
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

struct Config {
  char node[4]; // Alphanumeric Id (for sketch)
  int node_id; // Id suffix integer (for unique run-time Id)
  int version;
  char config_id[4];
  signed int temp_offset;
  float temp_k;
  bool display;
  bool dht;
  bool rtc;
  int rf24id;
  uint64_t rf24addr;
  uint64_t rf24routes[1];
} static_config = {
  /* default values */
  { node[0], node[1], }, 0, version,
  CONFIG_VERSION, TEMP_OFFSET, TEMP_K,
  false, true, true,
  1, 0xF0F0F0F0D2LL, { 0xF0F0F0F0E1LL }
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

#if _LCD84x48
/* Nokkia 5110 display */
#endif //_LCD84x48

#if _DS
/* Dallas OneWire bus with registration for DS18B20 temperature sensors */

OneWire ds(DS_PIN);

uint8_t ds_count = 0;
uint8_t ds_search = 0;


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
#define DS1307_I2C_ADDRESS 0x68
#endif // RTC

#if _HMC5883L
/* Digital magnetometer I2C module */

#endif // HMC5883L


/* *** /Peripheral devices *** }}} */

/* *** Generic routines *** {{{ */

// Convert normal decimal numbers to binary coded decimal
byte decToBcd(byte val)
{
  return ( (val/10*16) + (val%10) );
}

// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val)
{
  return ( (val/16*10) + (val%16) );
}

/* *** /Generic routines *** }}} */

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
//uint8_t bell[8]  = {0x4,0xe,0xe,0xe,0x1f,0x0,0x4};

/* I2C LCD 1602 parameters */
#define I2C_ADDR    0x20  // Define I2C Address where the PCF8574A is

#define BACKLIGHT_PIN     7
#define En_pin  4
#define Rw_pin  5
#define Rs_pin  6
#define D4_pin  0
#define D5_pin  1
#define D6_pin  2
#define D7_pin  3

#define  LED_OFF  0
#define  LED_ON  1


LiquidCrystalTmp_I2C  lcd(I2C_ADDR,En_pin,Rw_pin,Rs_pin,D4_pin,D5_pin,D6_pin,D7_pin);

void i2c_lcd_init()
{
  lcd.begin(16,2);
  lcd.setBacklightPin(BACKLIGHT_PIN, NEGATIVE);
  //lcd.noDisplay();
  //lcd.noBacklight();
  lcd.off();
  //lcd.setBacklight(LED_ON);
  lcd.leftToRight();
  lcd.noBlink();
  lcd.noCursor();
//lcd.createChar(0, bell);
}

void i2c_lcd_start()
{
  lcd.on();
  lcd.home();
}
#endif //_LCD

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
// Stops the DS1307, but it has the side effect of setting seconds to 0
// Probably only want to use this for testing
/*void stopDs1307()
  {
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  Wire.send(0);
  Wire.send(0x80);
  Wire.endTransmission();
  }*/

// 1) Sets the date and time on the ds1307
// 2) Starts the clock
// 3) Sets hour mode to 24 hour clock
// Assumes you're passing in valid numbers
void setDateDs1307(
    byte second,        // 0-59
    byte minute,        // 0-59
    byte hour,          // 1-23
    byte dayOfWeek,     // 1-7
    byte dayOfMonth,    // 1-28/29/30/31
    byte month,         // 1-12
    byte year)          // 0-99
{
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  Wire.write(0);
  Wire.write(decToBcd(second));    // 0 to bit 7 starts the clock
  Wire.write(decToBcd(minute));
  Wire.write(decToBcd(hour));      // If you want 12 hour am/pm you need to set
  // bit 6 (also need to change readDateDs1307)
  Wire.write(decToBcd(dayOfWeek));
  Wire.write(decToBcd(dayOfMonth));
  Wire.write(decToBcd(month));
  Wire.write(decToBcd(year));
  Wire.endTransmission();
}

// Gets the date and time from the ds1307
void getDateDs1307(
    byte *second,
    byte *minute,
    byte *hour,
    byte *dayOfWeek,
    byte *dayOfMonth,
    byte *month,
    byte *year)
{
  // Reset the register pointer
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  Wire.write(0);
  Wire.endTransmission();

  Wire.requestFrom(DS1307_I2C_ADDRESS, 7);

  // A few of these need masks because certain bits are control bits
  *second     = bcdToDec(Wire.read() & 0x7f);
  *minute     = bcdToDec(Wire.read());
  *hour       = bcdToDec(Wire.read() & 0x3f);  // Need to change this if 12 hour am/pm
  *dayOfWeek  = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month      = bcdToDec(Wire.read());
  *year       = bcdToDec(Wire.read());
}

void rtc_init()
{
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;

#if SERIAL_EN || DEBUG
  cmdIo.println("WaimanTinyRTC");
#endif
  Wire.begin();
  getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);

  if (second == 0 && minute == 0 && hour == 0 && month == 0 && year == 0) {
#if SERIAL_EN || DEBUG
    cmdIo.println("Warning: time reset");
#endif
    second = 0;
    minute = 31;
    hour = 5;
    dayOfWeek = 4;
    dayOfMonth = 18;
    month = 5;
    year = 12;
    setDateDs1307(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
  }
}

void rtc_run(void)
{
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);

#if _LCD
  if (config.display) {
    lcd.setCursor(0, 1);
    lcd.print(year, DEC);
    lcd.print("-");
    lcd.print(month, DEC);
    lcd.print("-");
    lcd.print(dayOfMonth, DEC);
    lcd.print(" ");
    lcd.print(hour, DEC);
    lcd.print(":");
    lcd.print(minute, DEC);
    lcd.print(":");
    lcd.print(second, DEC);
  }
#endif //_LCD
}
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
  cmdIo.println("n: print Node ID");
  cmdIo.println("c: print config");
  cmdIo.println("m: print free and used memory");
  cmdIo.println("t: internal temperature");
  cmdIo.println("T: set offset");
  cmdIo.println("o: temperature config");
  cmdIo.println("r: report");
  cmdIo.println("M: measure");
  cmdIo.println("S: stand-by");
  cmdIo.println("E: erase EEPROM!");
  cmdIo.println("x: reset");
  cmdIo.println("?/h: this help");
  idle.set(UI_SCHED_IDLE);
}

void nodeinfo_serscmd() {
  cmdIo.println(node_id);
}

void memstat_serscmd() {
  int free = freeRam();
  int used = usedRam();

  cmdIo.print("m ");
  cmdIo.print(free);
  cmdIo.print(' ');
  cmdIo.print(used);
  cmdIo.print(' ');
  cmdIo.println();
}

// forward declarations
void doReset(void);
void doReport(void);
void doMeasure(void);

void reset_sercmd(void) {
  doReset();
}

void report_sercmd(void) {
  doReport();
  idle.set(UI_SCHED_IDLE);
}

void measure_sercmd(void) {
  doMeasure();
  idle.set(UI_SCHED_IDLE);
}

void stdby_sercmd(void) {
  ui = false;
}

void config_sercmd(void) {
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
  cmdIo.println(internalTemp());
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
  double t = ( internalTemp() + config.temp_offset ) * config.temp_k ;
  cmdIo.println( t );
}

void eraseEEPROM(void) {
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
  sprintf(node_id, "%s%i", static_config.node, static_config.node_id);
}

void doConfig(void)
{
  /* load valid config or reset default config */
  if (!loadConfig(config)) {
    writeConfig(static_config);
  }
  initConfig();
}

void initLibs(void)
{
#if _RTC
  rtc_init();
#endif
#if _NRF24
  rf24_init();
#endif // NRF24
#if _RF12
  radio_init();
#endif
#if _LCD
  i2c_lcd_init();
  i2c_lcd_start();
  lcd.print(F("Cassette328P"));
#endif
#if _DHT
  dht.begin();
#if DEBUG
  //float t = dht.readTemperature();
  //cmdIo.println("Initialized DHT");
  //cmdIo.println(t);
#endif
#endif

#if _HMC5883L
#endif //_HMC5883L

  blink(LED_GREEN, 4, 100);
}


/* *** /Initialization routines *** }}} */

/* *** Run-time handlers *** {{{ */

void resetPayload(void)
{
#if LDR_PORT
  payload.light = 0;
#endif
#if PIR_PORT
  payload.moved = 0;
#endif
#if _DHT
  payload.rhum = 0;
  payload.temp = 0;
#endif //_DHT
  payload.ctemp = 0;
#if _MEM
  payload.memfree = 0;
#endif
#if _RFM12LOBAT
  payload.lobat = 0;
#endif
}

void doReset(void)
{
  doConfig();
  ui_irq = true;
  resetPayload();

  pinMode( LED_GREEN, OUTPUT );
  pinMode( LED_YELLOW, OUTPUT );
  pinMode( LED_RED, OUTPUT );
  digitalWrite( LED_GREEN, LOW );
  digitalWrite( LED_YELLOW, LOW );
  digitalWrite( LED_RED, LOW );
  tick = 0;
  reportCount = REPORT_EVERY;     // report right away for easy debugging
  scheduler.timer(ANNOUNCE, ANNOUNCE_START); // get the measurement loop going
}

bool doAnnounce(void)
{
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
  cmdIo.println("");
  return false;
}


// readout all the sensors and other values
void doMeasure(void)
{
  byte firstTime = payload.ctemp == 0; // special case to init running avg

  int ctemp = internalTemp(config.temp_offset, (float)config.temp_k/10);
  payload.ctemp = smoothedAverage(payload.ctemp, ctemp, firstTime);
#if SERIAL_EN && DEBUG_MEASURE
  cmdIo.print("AVR T new/avg ");
  cmdIo.print(ctemp);
  cmdIo.print(' ');
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
    payload.temp = smoothedAverage(payload.temp, t * 10, firstTime);
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

#if SHT11_PORT
#endif //SHT11_PORT
#if PIR_PORT
#endif
#if _DS
#endif //_DS
#if _HMC5883L
#endif //_HMC5883L
#if _MEM
  payload.memfree = freeRam();
#if SERIAL_EN && DEBUG_MEASURE
  cmdIo.print("MEM free ");
  cmdIo.println(payload.memfree);
#endif
#endif // MEM
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
#endif // NRF24

#if RTC
  blink(LED_YELLOW, 2, 25);
  rtc_run();
#endif // RTC
#if SERIAL_EN
  /* Report over serial, same fields and order as announced */
  cmdIo.print(node_id);
#if LDR_PORT
  cmdIo.print(' ');
  cmdIo.print((int) payload.light);
#endif
#if PIR
  cmdIo.print(' ');
  cmdIo.print((int) payload.moved);
#endif
#if _DHT
  cmdIo.print(' ');
  cmdIo.print((int) payload.rhum);
  cmdIo.print(' ');
  cmdIo.print((int) payload.temp);
#endif
  cmdIo.print(' ');
  cmdIo.print((int) payload.ctemp);
#if _DS
#endif
#if _HMC5883L
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

  return ok;
}

void runScheduler(char task)
{
  switch (task) {

    case DISCOVERY:
      // XXX
      break;

    case ANNOUNCE:
        doAnnounce();
        scheduler.timer(MEASURE, 0); //schedule next step
#if SERIAL_EN
      serialFlush();
#endif
      break;

    case MEASURE:
      blink(LED_YELLOW, 1, 200);
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
      blink(_DBG_LED, 2, 25, -1, true);
#endif
      break;

    case REPORT:
//      payload.msgtype = REPORT_MSG;
      blink(LED_YELLOW, 4, 50);
      bool ok = doReport();
      if (ok) {
        debugline("ACK");
        blink(LED_GREEN, 2, 100);
      } else {
        debugline("NACK");
        blink(LED_RED, 2, 200);
      }
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
  { 'n', 0, nodeinfo_serscmd },
  { 'm', 0, memstat_serscmd },
  { 'c', 0, config_sercmd },
  { 'o', 0, ctempconfig_sercmd },
  { 'T', 1, ctempoffset_sercmd },
  { 't', 0, ctemp_sercmd },
  { 'r', 0, report_sercmd },
  { 's', 0, stdby_sercmd },
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
  cmdIo.print(F("Free RAM: "));
  cmdIo.println(freeRam());
#endif

#if _RFM12B
#if DEBUG
  byte rf12_show = 1;
#else
  byte rf12_show = 0;
#endif
  //node= 4;
  rf12_initialize(node, RF12_868MHZ, 5);
  rf12_control(0x9485); // Receiver Control: Max LNA, 200kHz RX bw, DRSSI -73dB
  rf12_control(0x9850); // Transmission Control: Pos, 90kHz
  rf12_control(0xC606); // Data Rate 6
  rf12_sleep(RF12_SLEEP); // power down
#endif

  /* Read config from memory, or initialize with defaults */
//  if (!loadConfig(config))
//  {
//    saveRF24Config(static_config);
//    config = static_config;
//  }
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

  if (debug_ticks()) {
#ifdef _DBG_LED
    blink(_DBG_LED, 3, 10, -1, true);
#endif
  }

  if (cmdIo.available()) {
    parser.poll();
  }

  char task = scheduler.poll();
  if (-1 < task && task < SCHED_IDLE) {
    runScheduler(task);
  }

  return; // no sleep
#ifdef _DBG_LED
    blink(_DBG_LED, 1, 25, -1, true);
#endif
    serialFlush();
    task = scheduler.pollWaiting();
    if (-1 < task && task < SCHED_IDLE) {
      runScheduler(task);
    }
}

/* *** }}} */
// Derive: cmdIo
// Derive: SensorNode
