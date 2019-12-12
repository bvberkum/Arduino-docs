/*

RF24Node

- Low power scheduled node with UI loop
  and serial UI.

TODO: get NRF24 client stuff running

2015-01-15 - Started based on LogReader84x48,
    integrating RF24Network helloworld_tx example.
2015-01-17 - Simple RF24Network send working together
    with Lcd84x48 sketch.

 */



/* *** Globals and sketch configuration *** */
#define SERIAL_EN         1 /* Enable serial */
#define DEBUG             1 /* Enable trace statements */

#define _MEM              1  // Report free memory
#define _DHT              0
#define DHT_HIGH          0   // enable for DHT22/AM2302, low for DHT11
#define _DS               0
#define _LCD              0
#define _LCD84x48         1
#define _NRF24            1

#define REPORT_EVERY      2  // report every N measurement cycles
#define SMOOTH            5  // smoothing factor used for running averages
#define MEASURE_PERIOD    30 // how often to measure, in tenths of seconds
#define UI_SCHED_IDLE     4000  // tenths of seconds idle time, ...
#define UI_STDBY          8000  // ms
#define BL_INVERTED       0xFF
#define NRF24_CHANNEL     90
#define MAXLENLINE        79
#define ANNOUNCE_START    0



//#include <SoftwareSerial.h>
#include <EEPROM.h>
#include <JeeLib.h>
#include <OneWire.h>
#include <PCD8544.h>
#include <SPI.h>
#if _DHT
#include <Adafruit_Sensor.h>
#include <DHT.h> // Adafruit DHT
#endif
#include <RF24.h>
#include <RF24Network.h>
#include <DotmpeLib.h>
#include <mpelib.h>




const String sketch = "X-RF24Node";
const int version = 0;

char node[] = "nrf";

volatile bool ui_irq;
bool ui;


// determined upon handshake
char node_id[7];


/* IO pins */
//              RXD      0
//              TXD      1
//              INT0     2     UI IRQ
#       define SCLK     3  // LCD84x48
#       define SDIN     4  // LCD84x48
#       define DC       5  // LCD84x48
#       define RESET    6  // LCD84x48
#       define SCE      7  // LCD84x48
#       define CSN      8  // NRF24
#       define CE       9  // NRF24
#       define BL       10 // PWM Backlight
//              MOSI     11
//              MISO     12
//             SCK      13    NRF24
//#       define _DBG_LED 13 // SCK

//#if _DHT
//static const byte DHT_PIN = 7;
//#endif

MpeSerial mpeser (57600);

MilliTimer idle, stdby;

/* *** InputParser *** {{{ */

Stream& cmdIo = Serial;
extern const InputParser::Commands cmdTab[] PROGMEM;
const InputParser parser (50, cmdTab);


/* *** /InputParser }}} *** */

/* *** Report variables *** {{{ */


static byte reportCount;    // count up until next report, i.e. packet send

/* Data reported by this sketch */
struct {
  int ctemp       :10; // atmega temperature: -500..+500 (tenths)
#if _MEM
  int memfree     :16;
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
    return false;
}

void writeConfig(Config &c)
{
}



/* *** /EEPROM config *** }}} */

/* *** Scheduled tasks *** {{{ */


// has to be defined because we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

static bool schedRunning(void)
{
  for (int i=0;i<TASK_END;i++) {
    if (schedbuf[i] != 0 && schedbuf[i] != 0xFFFF) {
      return true;
    }
  }
  return false;
}

/* *** /Scheduled tasks *** }}} */

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
//DHT dht(DHT_PIN, DHTTYPE); // Adafruit DHT
//DHTxx dht (DHT_PIN); // JeeLib DHT
//#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT22   // DHT 22  (AM2302)
#if DHT_HIGH
DHT dht (DHT_PIN, DHT21);
//DHT dht (DHT_PIN, DHT22);
// AM2301
// DHT21, AM2302? AM2301?
#else
DHT dht(DHT_PIN, DHT11);
#endif
#endif // DHT

#if _DHT2
DHT dht2 (DHT2_PIN, DHT11);
#endif

#if _LCD84x48
/* Nokkia 5110 display */

// The dimensions of the LCD (in pixels)...
static const byte LCD_WIDTH = 84;
static const byte LCD_HEIGHT = 48;

// The number of lines for the temperature chart...
static const byte CHART_HEIGHT = 3;

// A custom "degrees" symbol...
static const byte DEGREES_CHAR = 1;
static const byte degrees_glyph[] = { 0x00, 0x07, 0x05, 0x07, 0x00 };

// A bitmap graphic (10x2) of a thermometer...
static const byte THERMO_WIDTH = 10;
static const byte THERMO_HEIGHT = 2;
static const byte thermometer[] = { 0x00, 0x00, 0x48, 0xfe, 0x01, 0xfe, 0x00, 0x02, 0x05, 0x02,
  0x00, 0x00, 0x62, 0xff, 0xfe, 0xff, 0x60, 0x00, 0x00, 0x00};

static PCD8544 lcd84x48(SCLK, SDIN, DC, RESET, SCE);


#endif // LCD84x48

#if _DS
/* Dallas OneWire bus with registration for DS18B20 temperature sensors */



#endif // DS

#if _RFM12B
/* HopeRF RFM12B 868Mhz digital radio */


#endif // RFM12B

#if _NRF24
/* nRF24L01+: nordic 2.4Ghz digital radio */

// Set up nRF24L01 radio on SPI bus plus two extra pins
RF24 radio(CE, CSN);

// Network uses that radio
RF24Network/*Debug*/ network(radio);

// Address of our node
const uint16_t this_node = 1;

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

void lcd_start()
{
  lcd84x48.begin(LCD_WIDTH, LCD_HEIGHT);

  // Register the custom symbol...
  lcd84x48.createChar(DEGREES_CHAR, degrees_glyph);

  lcd84x48.send( LOW, 0x21 );  // LCD Extended Commands on
  lcd84x48.send( LOW, 0x07 );  // Set Temp coefficent. //0x04--0x07
    lcd84x48.send( LOW, 0x12 );  // LCD bias mode 1:48. //0x13
    lcd84x48.send( LOW, 0x0C );  // LCD in normal mode.
  lcd84x48.send( LOW, 0x20 );  // LCD Extended Commands toggle off
}

void lcd_printWelcome(void)
{
  lcd84x48.setCursor(6, 0);
  lcd84x48.print(node);
  lcd84x48.setCursor(0, 5);
  lcd84x48.print(sketch);
}

void lcd_printTicks(void)
{
  lcd84x48.setCursor(10, 2);
  lcd84x48.print("tick ");
  lcd84x48.print(tick);
  lcd84x48.setCursor(10, 3);
  lcd84x48.print("idle ");
  lcd84x48.print(idle.remaining()/100);
  lcd84x48.setCursor(10, 4);
  lcd84x48.print("stdby ");
  lcd84x48.print(stdby.remaining()/100);
}


#endif // LCD84x48

#if _DS
/* Dallas DS18B20 thermometer routines */

static int ds_readdata(uint8_t addr[8], uint8_t data[12]) {
}

static int ds_conv_temp_c(uint8_t data[8], int SignBit) {
}

static int readDS18B20(uint8_t addr[8]) {

}

static uint8_t readDSCount() {
}

static void updateDSCount(uint8_t new_count) {
}

static void writeDSAddr(uint8_t addr[8]) {
}

static void readDSAddr(int a, uint8_t addr[8]) {
}

static void printDSAddrs(void) {
}

static void findDS18B20s(void) {
}


#endif // DS

#if _RFM12B
/* HopeRF RFM12B 868Mhz digital radio routines */


#endif //_RFM12B

#if _NRF24
/* Nordic nRF24L01+ radio routines */

void rf24_init()
{
}

void rf24_start()
{
}


#endif // NRF24 funcs

#if _RTC
#endif // RTC

#if _HMC5883L
/* Digital magnetometer I2C routines */

void Init_HMC5803L(void)
{
}

int HMC5803L_Read(byte Axis)
{
  int Result;

  return Result;
}
#endif // HMC5883L


/* *** /Peripheral hardware routines *** }}} */

/* *** UI *** {{{ */

//ISR(INT0_vect)
void irq0()
{
  ui_irq = true;
  //Sleepy::watchdogInterrupts(0);
}

byte SAMPLES = 0x1f;
uint8_t keysamples = SAMPLES;
static uint8_t keyactive = 4;
static uint8_t keypins[4] = {
  8, 11, 10, 2
};
uint8_t keysample[4] = { 0, 0, 0, 0 };
bool keys[4];

void readKeys(void) {
  for (int i=0;i<4;i++) {
    if (digitalRead(keypins[i])) {
      keysample[i]++;
    }
  }
}

bool keyActive(void) {
  keysamples = SAMPLES;
  for (int i=0;i<4;i++) {
    keys[i] = (keysample[i] > keyactive);
    keysample[i] = 0;
  }
  for (int i=0;i<4;i++) {
    if (keys[i])
      return true;
  }
  return false;
}

void printKeys(void) {
  for (int i=0;i<4;i++) {
    Serial.print(keys[i]);
    Serial.print(" ");
  }
  Serial.println();
}


/* *** /UI *** }}} */

/* *** UART commands *** {{{ */

#if SERIAL_EN

void helpCmd(void) {
  cmdIo.println("n: print Node ID");
  cmdIo.println("t: internal temperature");
  cmdIo.println("T: set offset");
  cmdIo.println("o: temperature config");
  cmdIo.println("r: report");
  cmdIo.println("M: measure");
  cmdIo.println("E: erase EEPROM!");
  cmdIo.println("x: reset");
  cmdIo.println("?/h: this help");
  idle.set(UI_SCHED_IDLE);
}

// forward declarations
void doReset(void);
void doReport(void);
void doMeasure(void);

void reset_sercmd() {
  doReset();
}

void reportCmd() {
  doReport();
  idle.set(UI_SCHED_IDLE);
}

void stdbyCmd() {
  ui = false;
  digitalWrite(BL, LOW ^ BL_INVERTED);
}

static void measureCmd() {
  doMeasure();
  idle.set(UI_SCHED_IDLE);
}

static void configCmd() {
  cmdIo.print("c ");
  cmdIo.print(config.node);
  cmdIo.print(" ");
  cmdIo.print(config.node_id);
  cmdIo.print(" ");
  cmdIo.print(config.version);
  cmdIo.println();
}

void tempConfigCmd(void) {
  Serial.print("Offset: ");
  Serial.println(config.temp_offset);
  Serial.print("K: ");
  Serial.println(config.temp_k/10);
  Serial.print("Raw: ");
  float temp_k = (float) config.temp_k / 10;
  int ctemp = internalTemp(config.temp_offset, temp_k);
  Serial.println(ctemp);
}

void tempOffsetCmd(void) {
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

void tempCmd(void) {
  float temp_k = (float) config.temp_k / 10;
  double ctemp = internalTemp(config.temp_offset, temp_k);
  Serial.println(ctemp);
}

static void eraseEEPROMCmd() {
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

/* Wire init {{{ */

// See I2CPIR

/* Wire init }}} */

/* Wire commands {{{ */

/* Wire commands }}} */;

/* Wire handling {{{ */
/* Wire Handling }}} */

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
}


/* *** /Initialization routines *** }}} */

/* *** Run-time handlers *** {{{ */

void doReset(void)
{
  doConfig();

#ifdef _DBG_LED
  pinMode(_DBG_LED, OUTPUT);
#endif
  pinMode(BL, OUTPUT);
  digitalWrite(BL, LOW ^ BL_INVERTED);
  attachInterrupt(INT0, irq0, RISING);
  ui_irq = true;
  tick = 0;

#if _NRF24
  rf24_init();
#endif //_NRF24

  reportCount = REPORT_EVERY; // report right away for easy debugging
  scheduler.timer(ANNOUNCE, ANNOUNCE_START); // get the measurement loop going
#if SERIAL_EN && DEBUG
  Serial.println("Reinitialized");
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

  int ctemp = internalTemp(config.temp_offset, (float)config.temp_k/10);
  payload.ctemp = smoothedAverage(payload.ctemp, ctemp, firstTime);
#if SERIAL_EN && DEBUG_MEASURE
  Serial.println();
  Serial.print("AVR T new/avg ");
  Serial.print(ctemp);
  Serial.print(' ');
  Serial.println(payload.ctemp);
#endif

#if _MEM
  payload.memfree = freeRam();
#if SERIAL_EN && DEBUG_MEASURE
  Serial.print("MEM free ");
  Serial.println(payload.memfree);
#endif
#endif //_MEM
}


// periodic report, i.e. send out a packet and optionally report on serial port
void doReport(void)
{
#if _RFM12B
#endif //_RFM12B

#if _NRF24
  RF24NetworkHeader header(/*to node*/ rf24_link_node);
  bool ok = network.write(header, &payload, sizeof(payload));
  if (ok)
    debugline("ACK");
  else
    debugline("NACK");
#endif // NRF24

#if SERIAL_EN
  Serial.print(node_id);
  Serial.print(' ');
  Serial.print((int) payload.ctemp);
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

void uiStart()
{
  idle.set(UI_SCHED_IDLE);
  if (!ui) {
    ui = true;
    lcd_start();
    lcd_printWelcome();
  }
}

void runScheduler(char task)
{
  switch (task) {

    case ANNOUNCE:
      debugline("ANNOUNCE");
      // TODO: see SensorNode
        doAnnounce();
        scheduler.timer(MEASURE, 0); //schedule next step
      serialFlush();
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
  { '?', 0, helpCmd },
  { 'h', 0, helpCmd },
  { 'c', 0, configCmd },
  { 'o', 0, tempConfigCmd },
  { 'T', 1, tempOffsetCmd },
  { 't', 0, tempCmd },
  { 'r', 0, reportCmd },
  { 's', 0, stdbyCmd },
  { 'M', 0, measureCmd },
  { 'E', 0, eraseEEPROMCmd },
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
  if (ui_irq) {
    debugline("Irq");
    ui_irq = false;
    uiStart();
    analogWrite(BL, 0xAF ^ BL_INVERTED);
  }
  debug_ticks();

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
      analogWrite(BL, 0x1F ^ BL_INVERTED);
      stdby.set(UI_STDBY);
    } else if (stdby.poll()) {
      digitalWrite(BL, LOW ^ BL_INVERTED);
      lcd84x48.clear();
      lcd84x48.stop();
      ui = false;
    } else if (!stdby.idle()) {
      // XXX toggle UI stdby Power, got to some lower power mode..
      delay(30);
    }
    lcd_printTicks();
  } else {
#ifdef _DBG_LED
    blink(_DBG_LED, 1, 25);
#endif
    serialFlush();
    task = scheduler.pollWaiting();
    if (-1 < task && task < SCHED_IDLE) {
      runScheduler(task);
    }
  }
}

/* *** }}} */
