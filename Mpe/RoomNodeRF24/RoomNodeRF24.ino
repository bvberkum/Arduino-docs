/*

RoomNodeRF24

 */



/* *** Globals and sketch configuration *** */
#define SERIAL_EN        1 /* Enable serial */
#define DEBUG           1 /* Enable trace statements */
#define DEBUG_MEASURE   0

#define _MEM            1   // Report free memory
#define _DHT            1
#define DHT_HIGH        0   // enable for DHT22/AM2302, low for DHT11
#define _RFM12B         0
#define _NRF24          1
#define SHT11_PORT      0   // defined if SHT11 is connected to a port
#define PIR_PORT        0   // defined if PIR is connected to a port's DIO pin
#define LDR_PORT        0   // defined if LDR is connected to a port's AIO pin

#define REPORT_EVERY    5   // report every N measurement cycles
#define SMOOTH          5   // smoothing factor used for running averages
#define MEASURE_PERIOD  600 // how often to measure, in tenths of seconds
//#define TEMP_OFFSET     -65
//#define TEMP_K          1.0
#define ANNOUNCE_START  0
#define UI_SCHED_IDLE         4000  // tenths of seconds idle time, ...
#define UI_STDBY        8000  // ms
#define CONFIG_VERSION "nx1"
#define CONFIG_EEPROM_START 50
#define NRF24_CHANNEL   90



#include <EEPROM.h>
#include <JeeLib.h>
#include <SPI.h>
#include <RF24.h>
#include <RF24Network.h>
#include <DHT.h> // Adafruit DHT
#include <DotmpeLib.h>
#include <mpelib.h>
#include "printf.h"




const String sketch = "RoomNodeRF24";
const int version = 0;

char node[] = "rn24";

String node_id = "rn24-1";

volatile bool ui_irq;
bool ui;


/* IO pins */
//              RXD      0
//              TXD      1
//              INT0     2
#if _DHT
static const byte DHT_PIN = 7;
#endif
#       define CSN      10  // NRF24
#       define CE       9  // NRF24
//define _DEBUG_LED 13
//#       define _DBG_LED 13 // SCK



MpeSerial mpeser (57600);

MilliTimer idle, stdby;


/* *** InputParser *** {{{ */

Stream& cmdIo = Serial;
extern InputParser::Commands cmdTab[];
InputParser parser (50, cmdTab);


/* *** /InputParser }}} *** */

/* *** Report variables *** {{{ */


static byte reportCount;    // count up until next report, i.e. packet send

/* Data reported by this sketch */
struct {
#if LDR_PORT
  byte light      :8;     // light sensor: 0..255
#endif
#if PIR_PORT
  byte moved      :1;  // motion detector: 0..1
#endif

#if _DHT
  int rhum    :7;  // 0..100 (avg'd)
// XXX : dht temp
#if DHT_HIGH
/* DHT22/AM2302: 20% to 100% @ 2% rhum, -40C to 80C @ ~0.5 temp */
  int temp    :10; // -500..+500 (int value of tenths avg)
#else
/* DHT11: 20% to 80% @ 5% rhum, 0C to 50C @ ~2C temp */
  int temp    :10; // -500..+500 (tenths, .5 resolution)
#endif // DHT_HIGH
#endif //_DHT

  int ctemp       :10; // atmega temperature: -500..+500 (tenths)
#ifdef _HMC5883L
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
  int rf24id;
} static_config = {
  /* default values */
  { node[0], node[1], 0, }, 0, version,
  { CONFIG_VERSION[0], CONFIG_VERSION[1], CONFIG_VERSION[2], },
  TEMP_OFFSET, TEMP_K
};

Config config;

bool loadConfig(Config &c)
{
  unsigned int w = sizeof(c);

  if (
      EEPROM.read(CONFIG_EEPROM_START + w - 1) == c.config_id[3] &&
      EEPROM.read(CONFIG_EEPROM_START + w - 2) == c.config_id[2] &&
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
    Serial.println("No valid data in eeprom");
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
      Serial.println("Error writing "+ String(t)+" to EEPROM");
    }
#endif
  }
}



/* *** /EEPROM config *** }}} */

/* *** Peripheral devices *** {{{ */

#if SHT11_PORT
SHT11 sht11 (SHT11_PORT);
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
#endif // DHT

#if _LCD84x48
/* Nokkia 5110 display */
#endif // LCD84x48

#if _DS
/* Dallas OneWire bus with registration for DS18B20 temperature sensors */

OneWire ds(DS_PIN);

uint8_t ds_count = 0;
uint8_t ds_search = 0;


//uint8_t ds_addr[ds_count][8] = {
//  { 0x28, 0xCC, 0x9A, 0xF4, 0x03, 0x00, 0x00, 0x6D }, // In Atmega8TempGaurd
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
#endif //_RTC

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
  cmdIo.println("x: reset");
  cmdIo.println("E: erase EEPROM!");
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

void reset_sercmd() {
  doReset();
}

void report_sercmd() {
  doReport();
  idle.set(UI_SCHED_IDLE);
}

void measure_sercmd() {
  doMeasure();
  idle.set(UI_SCHED_IDLE);
}

void stdby_sercmd() {
  ui = false;
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
  Serial.println(config.temp_k);
  Serial.print("Raw: ");
  Serial.println(internalTemp());
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
  double t = ( internalTemp() + config.temp_offset ) * config.temp_k ;
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

void initLibs()
{
#if _NRF24
  rf24_init();
  rf24_start();
#endif // NRF24
#if _DHT
  dht.begin();
#if DEBUG
  Serial.println("Initialized DHT");
  float t = dht.readTemperature();
  Serial.println(t);
  float rh = dht.readHumidity();
  Serial.println(rh);
#endif
#endif

#if SERIAL_EN && DEBUG
  printf_begin();
#endif
}


/* *** /Initialization routines *** }}} */

/* *** Run-time handlers *** {{{ */

void doReset(void)
{
  doConfig();
  ui_irq = true;
  tick = 0;
  reportCount = REPORT_EVERY;     // report right away for easy debugging
  scheduler.timer(ANNOUNCE, ANNOUNCE_START); // get the measurement loop going
}

bool doAnnounce(void)
{
#if SERIAL_EN && DEBUG
  cmdIo.print("\n[");
  cmdIo.print(sketch);
  cmdIo.print(".");
  cmdIo.print(version);
  cmdIo.println("]");
#endif // SERIAL_EN && DEBUG
  cmdIo.println(node_id);
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
#if _MEM
#if SERIAL_EN
  Serial.print(F("memfree:16 "));
#endif
#if _RFM12LOBAT
  Serial.print(F("lobat:1 "));
#endif //_RFM12LOBAT
  return false;
}


// readout all the sensors and other values
void doMeasure(void)
{
  byte firstTime = payload.ctemp == 0; // special case to init running avg

  int ctemp = ( internalTemp() + config.temp_offset ) * config.temp_k ;
  payload.ctemp = smoothedAverage(payload.ctemp, ctemp, firstTime);
#if SERIAL_EN && DEBUG_MEASURE
  Serial.println();
  Serial.print("AVR T new/avg ");
  Serial.print(ctemp);
  Serial.print(' ');
  Serial.println(payload.ctemp);
#endif

#if _RFM12LOBAT
  payload.lobat = rf12_lowbat();
#if SERIAL_EN && DEBUG_MEASURE
  if (payload.lobat) {
    Serial.println("Low battery");
  }
#endif
#endif //_RFM12LOBAT

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
    payload.temp = smoothedAverage(payload.temp, t * 10, firstTime);
#if SERIAL_EN && DEBUG_MEASURE
    Serial.print(F("DHT T new/avg "));
    Serial.print(t);
    Serial.print(' ');
    Serial.println(payload.temp);
#endif
  }
#endif // _DHT

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
#endif // LDR_PORT

#if _MEM
  payload.memfree = freeRam();
#if SERIAL_EN && DEBUG_MEASURE
  Serial.print("MEM free ");
  Serial.println(payload.memfree);
#endif
#endif //_MEM
}


// periodic report, i.e. send out a packet and optionally report on serial port
bool doReport(void)
{
  bool ok;

#if _RFM12B
  rf12_sleep(RF12_WAKEUP);
  while (!rf12_canSend())
    rf12_recvDone();
  rf12_sendStart(0, &payload, sizeof payload, RADIO_SYNC_MODE);
  rf12_sleep(RF12_SLEEP);
#endif //_RFM12B
#if _NRF24
  //rf24_run();
  RF24NetworkHeader header(/*to node*/ rf24_link_node);
  ok = network.write(header, &payload, sizeof(payload));
#endif //_NRF24

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
  Serial.print(' ');
  Serial.print((int) payload.rhum);
  Serial.print(' ');
  Serial.print((int) payload.temp);
#endif
  Serial.print(' ');
  Serial.print((int) payload.ctemp);
#if _DS
#endif
#if _HMC5883L
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

  return ok;
}


void runScheduler(char task)
{
  switch (task) {

    case ANNOUNCE:
        doAnnounce();
        scheduler.timer(MEASURE, 0); //schedule next step
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
      break;

    case REPORT:
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

#if PIR_PORT

// send packet and wait for ack when there is a motion trigger
void doTrigger()
{
}
#endif // PIR_PORT

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
  { 'r', 0, reset_sercmd },
  { 's', 0, stdby_sercmd },
  { 'x', 0, report_sercmd },
  { 'M', 0, measure_sercmd },
  { 'E', 0, eraseEEPROM },
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
  network.update();
#endif
  if (ui_irq) {
    debugline("Irq");
    ui_irq = false;
    uiStart();
  }
  if (debug_ticks()) {
#ifdef _DBG_LED
    blink(_DBG_LED, 1, 50);
#endif
  }
  serialFlush();

  if (cmdIo.available()) {
    parser.poll();
    return;
  }

  char task = scheduler.poll();
  if (-1 < task && task < SCHED_IDLE) {
    runScheduler(task);
  }

  if (ui) {
    if (idle.poll()) {
      stdby.set(UI_STDBY);
    } else if (stdby.poll()) {
      ui = false;
    } else if (!stdby.idle()) {
      // XXX toggle UI stdby Power, got to some lower power mode..
      delay(30);
    }
  } else {
    serialFlush();
    task = scheduler.pollWaiting();
    if (-1 < task && task < SCHED_IDLE) {
      runScheduler(task);
    }
}

/* *** }}} */

