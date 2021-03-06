/*

  Lcd84x48

=Features
- Using Nokia 5110 display board and Arduino pro mini clone.
- Using to about 30mA with background fully lit.
- Low-power about 60 to 70 uA in standby mode.

=ToDo
- Start using a simple statemachine to enter into different power/function
  states.
- Investigate hardware for external interrupts. Standard specs do not seem to
  allow an wake-up from power-off using serial, but look at TWI address-match.
- Watchdog wakeup could use abit of work to do better scheduling.
- LCD dims when idle mode kicks in, would like to elaborate using an LDR or
  photov. cel.

2014-06-22 Started

=Description
Using the scheduler pollWaiting there is a continious measure and report tasks
cycle running using minimal power. This follows JeeLab's roomNode low power
sensor node and JeeLib.

During low power, only INT0 or INT1, WDT, and TWI address-match events will wake
the Atmega. In this mode, loop halts at scheduler.pollWaiting where the
scheduler uses the WDT to wake up at some remote time.

One switch generates a RISING event for INTO, making the next loop enter into
high power mode with no delays. This enables waiting for other serial and button
events, starts up the LCD and the backlight.

Successive states can trim power-usage a bit in this active mode. Current code uses an
idle timer (that should be reset on ui/serial events),
which on timeout sets a second timer stdby
which finally on timeout will turn off the UI mode making next loop return to the
pollWaiting routines.

During the idle and stdby timers various schemes are possible,
depends on the used hardware etc.

*/


/* *** Globals and sketch configuration *** */
#define SERIAL_EN         1 /* Enable serial */
#define DEBUG             1 /* Enable trace statements */

#define _MEM              1 // Report free memory
#define _LCD84x48         1

#define REPORT_EVERY      5 // report every N measurement cycles
#define SMOOTH            5 // smoothing factor used for running averages
#define MEASURE_PERIOD    300 // how often to measure, in tenths of seconds
#define UI_SCHED_IDLE     3000 // tenths of seconds idle time, ...
#define UI_STDBY          8000 // ms
#define MAXLENLINE        79
#define BL_INVERTED       0xFF



#include <JeeLib.h>
#include <OneWire.h>
#include <PCD8544.h>
#include <DotmpeLib.h>
#include <mpelib.h>



const String sketch = "X-Lcd84x48";
const int version = 0;

String node = "lcd84x48-1";

volatile bool ui_irq;
bool ui;

/* IO pins */
//             RXD      1
//             TXD      2
//             INT0     2     UI IRQ
#       define SCLK     3  // LCD84x48
#       define SDIN     4  // LCD84x48
#       define DC       5  // LCD84x48
#       define RESET    6  // LCD84x48
#       define SCE      7  // LCD84x48
#       define BL       10 // PWM Backlight
//             MOSI     11    NRF24
//             MISO     12    NRF24
//             SCK      13    NRF24
//define _DBG_LED 13
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

// See Prototype Node or SensorNode

/* *** /EEPROM config *** }}} */

/* *** Scheduler routines *** {{{ */

// has to be defined because we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

void debug_task(char task) {
  if (task == -2) { // nothing running
    cmdIo.print('n');
  } else if (task == -1) { // waiting
    cmdIo.print('w');
  }
}

bool printSchedRunning(void)
{
  for (int i=0;i<TASK_END;i++) {
    cmdIo.print(i);
    cmdIo.print(' ');
    cmdIo.print(schedbuf[i]);
    cmdIo.println();
  }
}

bool schedRunning(void)
{
  for (int i=0;i<TASK_END;i++) {/*
    cmdIo.print(i);
    cmdIo.print(' ');
    cmdIo.print(schedbuf[i]);
    cmdIo.println(); */
    if (schedbuf[i] != 0 && schedbuf[i] != 0xFFFF) {
      return true;
    }
  }
  return false;
}

/* *** /Scheduler routines *** }}} */

/* *** Peripheral devices *** {{{ */

#if LDR_PORT
#endif

#if _DHT
/* DHTxx: Digital temperature/humidity (Adafruit) */
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

/* *** /UI *** }}} */

/* *** UART commands *** {{{ */


static void stdbyCmd() {
  ui = false;
#if _LCD84x48
  digitalWrite(BL, LOW ^ BL_INVERTED);
#endif // LCD84x48
}


/* *** UART commands *** }}} */

/* *** Initialization routines *** {{{ */

void doConfig(void)
{
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
#if SERIAL_EN && DEBUG
  cmdIo.println("Reinitialized");
#endif
}

bool doAnnounce(void)
{
  return false;
}


// readout all the sensors and other values
void doMeasure(void)
{
  byte firstTime = payload.ctemp == 0; // special case to init running avg

  int ctemp = internalTemp(0, 0);
  payload.ctemp = smoothedAverage(payload.ctemp, ctemp, firstTime);
#if SERIAL_EN && DEBUG_MEASURE
  cmdIo.println();
  cmdIo.print("AVR T new/avg ");
  cmdIo.print(ctemp);
  cmdIo.print(' ');
  cmdIo.println(payload.ctemp);
#endif

#if _MEM
  payload.memfree = freeRam();
#if SERIAL_EN && DEBUG_MEASURE
  cmdIo.print("MEM free ");
  cmdIo.println(payload.memfree);
#endif
#endif //_MEM
}

void doReport(void)
{
#if SERIAL_EN
  cmdIo.print(node);
  cmdIo.print(" ");
  cmdIo.print((int) payload.ctemp);
  cmdIo.print(' ');
#if _MEM
  cmdIo.print((int) payload.memfree);
  cmdIo.print(' ');
#endif
  cmdIo.println();
  serialFlush();
#endif//SERIAL_EN
}

void uiStart()
{
  idle.set(UI_SCHED_IDLE);
  if (!ui) {
    ui = true;
#if _LCD84x48
    lcd_start();
    lcd_printWelcome();
#endif // LCD84x48
  }
}

void runScheduler(char task)
{
  switch (task) {

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

static void help_sercmd() {
  cmdIo.println("Help!");
  idle.set(UI_SCHED_IDLE);
}

static void valueCmd () {
  int v;
  parser >> v;
  cmdIo.print("value = ");
  cmdIo.println(v);
  idle.set(UI_SCHED_IDLE);
}

static void doReport () {
  doReport();
  idle.set(UI_SCHED_IDLE);
}

static void doMeasure() {
  doMeasure();
  idle.set(UI_SCHED_IDLE);
}

const InputParser::Commands cmdTab[] = {
  { '?', 0, help_sercmd },
  { 'h', 0, help_sercmd },
  { 'v', 2, valueCmd },
  { 'r', 0, doReport },
  { 's', 0, stdbyCmd },
  { 'M', 0, doMeasure },
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
#ifdef _DBG_LED
  blink(_DBG_LED, 1, 15);
#endif
  if (ui_irq) {
    debugline("Irq");
    ui_irq = false;
    uiStart();
#if _LCD84x48
    analogWrite(BL, 0xAF ^ BL_INVERTED);
#endif // LCD84x48
  }
  debug_ticks();
  char task = scheduler.poll();
  debug_task(task);
  if (-1 < task && task < SCHED_IDLE) {
    runScheduler(task);
  }

  if (ui) {
    parser.poll();
    if (idle.poll()) {
      debugline("Idle");
#if _LCD84x48
      analogWrite(BL, 0x1F ^ BL_INVERTED);
#endif // LCD84x48
      stdby.set(UI_STDBY);
    } else if (stdby.poll()) {
      debugline("StdBy");
#if _LCD84x48
      digitalWrite(BL, LOW ^ BL_INVERTED);
      lcd84x48.clear();
      lcd84x48.stop();
#endif // LCD84x48
      ui = false;
    } else if (!stdby.idle()) {
      // XXX toggle UI stdby Power, got to some lower power mode..
      delay(30);
    }
#if _LCD84x48
    lcd_printTicks();
#endif // LCD84x48
  } else {
#ifdef _DBG_LED
    blink(_DBG_LED, 1, 25);
#endif
    debugline("Sleep");
    serialFlush();
    task = scheduler.pollWaiting();
    debugline("Wakeup");
    if (-1 < task && task < SCHED_IDLE) {
      runScheduler(task);
    }
  }
}

/* *** }}} */
