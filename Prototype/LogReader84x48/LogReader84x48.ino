/*

SD Card log reader with 84x48 display.

Current targets:
  - Display menu
  - SD card file log
  - Serial mpe-bus slave version

2014-06-15 - Started
2014-06-22 - Created Lcd84x48 based on this.

 */


/* *** Globals and sketch configuration *** */
#define SERIAL_EN         1 /* Enable serial */
#define DEBUG             1 /* Enable trace statements */

#define _MEM              1 // Report free memory
#define _LCD84x48         1

#define REPORT_EVERY      5 // report every N measurement cycles
#define SMOOTH            5 // smoothing factor used for running averages
#define MEASURE_PERIOD    100 // how often to measure, in tenths of seconds
#define UI_SCHED_IDLE     4000 // tenths of seconds idle time, ...
#define UI_STDBY          8000 // ms
#define MAXLENLINE        79
#define BL_INVERTED       0xFF



#include <JeeLib.h>
#include <OneWire.h>
#include <PCD8544.h>
#include <DotmpeLib.h>
#include <mpelib.h>



const String sketch = "X-LogReader84x48";
const int version = 0;

char node[] = "lr";


volatile bool ui_irq;
bool ui;

byte bl_level = 0xff;

int out = 20;

/* IO pins */
//              RXD      0
//              TXD      1
//              INT0     2     UI IRQ
#       define SCLK     3  // LCD84x48
#       define SDIN     4  // LCD84x48
#       define DC       5  // LCD84x48
#       define RESET    6  // LCD84x48
#       define SCE      7  // LCD84x48
#       define BL       10 // PWM Backlight
//              MOSI     11
//              MISO     12
#       define _DBG_LED 13 // SCK


MpeSerial mpeser (57600);

MilliTimer idle, stdby;

/* *** InputParser *** {{{ */

extern const InputParser::Commands cmdTab[] PROGMEM;
byte* buffer = (byte*) malloc(50);
const InputParser parser (buffer, 50, cmdTab);


/* *** /InputParser *** }}} */

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


/* *** /EEPROM config *** }}} */

/* *** Scheduler routines *** {{{ */

// has to be defined because we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

bool schedRunning()
{
  for (int i=0;i<TASK_END;i++) {/*
    Serial.print(i);
    Serial.print(' ');
    Serial.print(schedbuf[i]);
    Serial.println(); */
    if (schedbuf[i] != 0 && schedbuf[i] != 0xFFFF) {
      return true;
    }
  }
  return false;
}

/* *** /Scheduler routines *** }}} */

/* *** Peripheral devices *** {{{ */

#if LDR_PORT
Port ldr (LDR_PORT);
#endif

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

#if _RFM12B
/* HopeRF RFM12B 868Mhz digital radio */

#endif //_RFM12B

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


#endif // DS

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

/* *** Initialization routines *** {{{ */

void initConfig(void)
{
  // See Prototype/Node
}

void doConfig(void)
{
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

#if _LCD84x48
  pinMode(BL, OUTPUT);
  digitalWrite(BL, LOW ^ BL_INVERTED);
  attachInterrupt(INT0, irq0, RISING);
#endif // LCD84x48

  ui_irq = true;
  tick = 0;

  reportCount = REPORT_EVERY; // report right away for easy debugging
  scheduler.timer(MEASURE, 0); // get the measurement loop going
}

bool doAnnounce()
{
  return false;
}


// readout all the sensors and other values
void doMeasure(void)
{
  byte firstTime = payload.ctemp == 0; // special case to init running avg

  int ctemp = internalTemp(0, 0);
  payload.ctemp = smoothedAverage(payload.ctemp, ctemp, firstTime);

#if _MEM
  payload.memfree = freeRam();
#if SERIAL_EN && DEBUG_MEASURE
  Serial.print("MEM free ");
  Serial.println(payload.memfree);
#endif
#endif

#if SERIAL_EN
  serialFlush();
#endif
}

// periodic report, i.e. send out a packet and optionally report on serial port
void doReport(void)
{
#if SERIAL_EN
  Serial.print(node);
  Serial.print(" ");
  Serial.print((int) payload.ctemp);
  Serial.print(' ');
#if _MEM
  Serial.print((int) payload.memfree);
  Serial.print(' ');
#endif
  Serial.println();
  serialFlush();
#endif//SERIAL_EN
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

    case MEASURE:
      debugline("MEASURE");
      scheduler.timer(MEASURE, MEASURE_PERIOD);
      doMeasure();

      if (++reportCount >= REPORT_EVERY) {
        reportCount = 0;
        scheduler.timer(REPORT, 0);
      }
      serialFlush();
#ifdef _DBG_LED
      blink(_DBG_LED, 2, 25);
#endif
      break;

    case REPORT:
      debugline("REPORT");
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

void helpCmd() {
  Serial.println("Help!");
  idle.set(UI_SCHED_IDLE);
}

void valueCmd () {
  int v;
  parser >> v;
  Serial.print("value = ");
  Serial.println(v);
  idle.set(UI_SCHED_IDLE);
}

void reportCmd () {
  doReport();
  idle.set(UI_SCHED_IDLE);
}

void measureCmd() {
  doMeasure();
  idle.set(UI_SCHED_IDLE);
}

void stdbyCmd() {
  ui = false;
  digitalWrite(BL, LOW ^ BL_INVERTED);
}

const InputParser::Commands cmdTab[] = {
  { 'h', 0, helpCmd },
  { 'v', 2, valueCmd },
  { 'm', 0, measureCmd },
  { 'r', 0, reportCmd },
  { 's', 0, stdbyCmd },
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
#ifdef _DBG_LED
  blink(_DBG_LED, 1, 15);
#endif
  if (ui_irq) {
    debugline("Irq");
    ui_irq = false;
    uiStart();
    analogWrite(BL, 0xAF ^ BL_INVERTED);
  }
  debug_ticks();
  char task = scheduler.poll();
  if (-1 < task && task < SCHED_IDLE) {
    runScheduler(task);
  }
  if (ui) {
    parser.poll();
    if (idle.poll()) {
      debugline("Idle");
      analogWrite(BL, 0x1F ^ BL_INVERTED);
      stdby.set(UI_STDBY);
    } else if (stdby.poll()) {
      debugline("StdBy");
      digitalWrite(BL, LOW ^ BL_INVERTED);
      lcd84x48.clear();
      lcd84x48.stop();
      ui = false;
    }
    lcd_printTicks();
  } else {
#ifdef _DBG_LED
    blink(_DBG_LED, 1, 15);
#endif
    debugline("Sleep");
    serialFlush();
    char task = scheduler.pollWaiting();
    debugline("Wakeup");
    if (-1 < task && task < SCHED_IDLE) {
      runScheduler(task);
    }
  }
}

/* *** }}} */
