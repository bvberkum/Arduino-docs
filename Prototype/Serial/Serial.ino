/*
  Serial extends AtmegaEEPROM, AtmegaTemp

  - boilerplate for ATmega node with Serial interface
  - uses Node event loop which is based on JeeLib scheduler.


 */



/* *** Globals and sketch configuration *** */
#define SERIAL_EN         1 /* Enable serial */
#define DEBUG             1 /* Enable trace statements */

#define _MEM              1 // Report free memory

#define UI_SCHED_IDLE     4000 // tenths of seconds idle time, ...
#define UI_STDBY          8000 // ms
#define MAXLENLINE        79
#define ANNOUNCE_START    0
#define CONFIG_VERSION   "nx1"
#define CONFIG_EEPROM_START 0



//#include <SoftwareSerial.h>
#include <Wire.h>
#include <EEPROM.h>
//#include <util/crc16.h>
#include <JeeLib.h>
#include <SPI.h>
#include <RF24.h>
#include <RF24Network.h>
#include <DotmpeLib.h>
#include <mpelib.h>



const String sketch = "X-Serial";
const int version = 0;

char node[] = "rf24n";
char node_id[7];

volatile bool ui_irq;
bool ui;


/* IO pins */
//              RXD      0
//              TXD      1
//              INT0     2     UI IRQ
#       define BL       10 // PWM Backlight
//              MOSI     11
//              MISO     12
#       define _DBG_LED 13 // SCK
//#       define          A0
//#       define          A1
//#       define          A2
//#       define          A3
//#       define          A4
//#       define          A5


MpeSerial mpeser (57600);

MilliTimer idle, stdby;

/* *** InputParser *** {{{ */

Stream& cmdIo = Serial;
extern const InputParser::Commands cmdTab[] PROGMEM;
byte* buffer = (byte*) malloc(50);
const InputParser parser (buffer, 50, cmdTab);//, virtSerial);


/* *** /InputParser *** }}} */

/* *** Report variables *** {{{ */




/* *** /Report variables *** }}} */

/* *** Scheduled tasks *** {{{ */

enum {
  ANNOUNCE,
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
} static_config = {
  /* default values */
  { node[0], node[1], 0, 0 }, 0, version,
  CONFIG_VERSION, TEMP_OFFSET, TEMP_K,
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
#endif

#if _DHT
/* DHTxx: Digital temperature/humidity (Adafruit) */
#endif // DHT

#if _AM2321
#endif

#if _LCD84x48
/* Nokkia 5110 display */
#endif // LCD84x48

#if _DS
/* Dallas OneWire bus with registration for DS18B20 temperature sensors */

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

#endif // HMC5883L


/* *** /Peripheral devices *** }}} */

/* *** Generic routines *** {{{ */

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


#endif // NRF24 funcs

#if _RTC
#endif // RTC

#if _HMC5883L
/* Digital magnetometer I2C routines */


#endif // HMC5883L


/* *** /Peripheral hardware routines *** }}} */

/* *** UI *** {{{ */


//ISR(INT0_vect)
void irq0()
{
  ui_irq = true;
  //Sleepy::watchdogInterrupts(0);
}

/* *** /UI *** }}} */

/* *** UART commands *** {{{ */

#if SERIAL_EN

void help_sercmd(void) {
  cmdIo.println("n: print Node ID");
  cmdIo.println("c: print config");
//  cmdIo.println("v: print version");
//  cmdIo.println("N: set Node (3 byte char)");
//  cmdIo.println("C: set Node ID (1 byte int)");
  cmdIo.println("m: print free and used memory");
  cmdIo.println("t: internal temperature");
  cmdIo.println("T: set offset");
  cmdIo.println("o: temperature config");
  cmdIo.println("r: report");
  cmdIo.println("M: measure");
//  cmdIo.println("W: load/save EEPROM");
  cmdIo.println("E: erase EEPROM!");
  cmdIo.println("x: reset");
  cmdIo.println("?/h: this help");
  idle.set(UI_SCHED_IDLE);
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
  cmdIo.println(internalTemp(0, 0));
}

void ser_tempOffset(void) {
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

void ctemp_srcmd(void) {
  double t = ( internalTemp(0, 0) + config.temp_offset ) * config.temp_k ;
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
}


/* *** /Initialization routines *** }}} */

/* *** Run-time handlers *** {{{ */

void doReset(void)
{
  doConfig();

#ifdef _DBG_LED
  pinMode(_DBG_LED, OUTPUT);
#endif
  attachInterrupt(INT0, irq0, RISING);
  ui_irq = true;
  tick = 0;
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

  return false;
}


// readout all the sensors and other values
void doMeasure(void)
{
  // none, see  SensorNode
}


// periodic report, i.e. send out a packet and optionally report on serial port
void doReport(void)
{
  // none, see RadioNode
}

void uiStart()
{
  idle.set(UI_SCHED_IDLE);
  if (!ui) {
    ui = true;
  }
}

void runScheduler(char task)
{
  switch (task) {

    case ANNOUNCE:
        doAnnounce();
        //scheduler.timer(ANNOUNCE, 100);
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
  { 'm', 0, memstat_serscmd },
  { 'o', 0, ctempconfig_sercmd },
  { 'T', 1, ser_tempOffset },
  { 't', 0, ctemp_srcmd },
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
#ifdef _DBG_LED
  blink(_DBG_LED, 3, 10);
#endif
  if (ui_irq) {
    debugline("Irq");
    ui_irq = false;
    uiStart();
  }
  debug_ticks();

  if (cmdIo.available()) {
    parser.poll();
    return;
  }

  char task = scheduler.poll();
  if (-1 < task && task < SCHED_IDLE) {
    runScheduler(task);
    return;
  }

  if (ui) {
    if (idle.poll()) {
      debugline("Idle");
      stdby.set(UI_STDBY);
    } else if (stdby.poll()) {
      debugline("StdBy");
      ui = false;
    } else if (!stdby.idle()) {
      // XXX toggle UI stdby Power, got to some lower power mode..
      delay(30);
    }
  } else {
#ifdef _DBG_LED
    blink(_DBG_LED, 1, 25);
#endif
    serialFlush();
    task = scheduler.pollWaiting();
    if (-1 < task && task < SCHED_IDLE) {
      runScheduler(task);
    } else {
      Sleepy::loseSomeTime(1000);
    }
  }
}

/* *** }}} */
// Derive: AtmegaTemp
// Derive: AtmegaEEPROM
