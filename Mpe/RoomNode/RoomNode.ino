/* Mpe - Arduino 1.0.1

// New version of the Room Node, derived from rooms.pde
// 2010-10-19 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php

// see http://jeelabs.org/2010/10/20/new-roomnode-code/
// and http://jeelabs.org/2010/10/21/reporting-motion/

// The complexity in the code below comes from the fact that newly detected PIR
// motion needs to be reported as soon as possible, but only once, while all the
// other sensor values are being collected and averaged in a more regular cycle.
*/

#include <DHT.h> // Adafruit DHT
#include <DotmpeLib.h>
#include <JeeLib.h>
#include <OneWire.h>
#include <PortsSHT11.h>
#include <Wire.h>
#include <avr/sleep.h>
#include <util/atomic.h>

/** Globals and sketch configuration  */
#define SERIAL_EN       1 /* Enable serial */
#define DEBUG           1 /* Enable trace statements */

#define MEASURE_PERIOD  600 // how often to measure, in tenths of seconds
#define REPORT_EVERY    5   // report every N measurement cycles
#define SMOOTH          3   // smoothing factor used for running averages
#define RETRY_PERIOD    10  // how soon to retry if ACK didn't come in
#define RETRY_LIMIT     5   // maximum number of times to retry
#define ACK_TIME        10  // number of milliseconds to wait for an ack

#define MAXLENLINE      79
#define SRAM_SIZE       0x800 // atmega328, for debugging
// set the sync mode to 2 if the fuses are still the Arduino default
// mode 3 (full powerdown) can only be used with 258 CK startup fuses
#define RADIO_SYNC_MODE 2

#define _MEM            1   // Report free memory
#define _RFM12B         1
#define _RFM12LOBAT    1
#define _NRF24          1
#define _HMC5883L       1
#define _DHT            1
#define DHT_PIN         7   // DIO for DHTxx
#define _DS             1
#define DS_PIN          A5
#define DEBUG_DS        1
//#define FIND_DS    1
#define SHT11_PORT      1   // defined if SHT11 is connected to a port
#define LDR_PORT        4   // defined if LDR is connected to a port's AIO pin
#define PIR_PORT        4   // defined if PIR is connected to a port's DIO pin


String sketch = "RF24Test";
String version = "0";

String node_id = "rm-1";

MpeSerial mpeser (57600);

int tick = 0;
int pos = 0;

/* IO pins */
const byte ledPin = 13; // XXX shared with nrf24 SCK
#if _NRF24
const byte rf24_ce = 9;
const byte rf24_csn = 8;
#endif


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

PIR pir (PIR_PORT);

// the PIR signal comes in via a pin-change interrupt
ISR(PCINT2_vect) { pir.poll(); }

#endif
/* *** /PIR support }}} *** */

#if _DHT

#endif //_DHT

#if _NRF24
#endif //_NRF24
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

static byte reportCount;    // count up until next report, i.e. packet send
static byte myNodeID;       // node ID used for this unit

// This defines the structure of the packets which get sent out by wireless:

struct {
#if LDR_PORT
  byte light      :8;     // light sensor: 0..255
#endif
#if PIR_PORT
  byte moved      :1;  // motion detector: 0..1
#endif
  byte humi  :7;  // humidity: 0..100
  int temp   :10; // temperature: -500..+500 (tenths)
  int ctemp       :10; // atmega temperature: -500..+500 (tenths)
#if _HMC5883L
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


/** AVR routines */

int freeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

int usedRam () {
  return SRAM_SIZE - freeRam();
}


/** ATmega routines */

double internalTemp(void)
{
  unsigned int wADC;
  double t;

  // The internal temperature has to be used
  // with the internal reference of 1.1V.
  // Channel 8 can not be selected with
  // the analogRead function yet.

  // Set the internal reference and mux.
  ADMUX = (_BV(REFS1) | _BV(REFS0) | _BV(MUX3));
  ADCSRA |= _BV(ADEN);  // enable the ADC

  delay(20);            // wait for voltages to become stable.

  ADCSRA |= _BV(ADSC);  // Start the ADC

  // Detect end-of-conversion
  while (bit_is_set(ADCSRA,ADSC));

  // Reading register "ADCW" takes care of how to read ADCL and ADCH.
  wADC = ADCW;

  // The offset of 324.31 could be wrong. It is just an indication.
  t = (wADC - 311 ) / 1.22;

  // The returned temperature is in degrees Celcius.
  return (t);
}


/** Generic routines */

static void serialFlush () {
#if SERIAL_EN
#if ARDUINO >= 100
  Serial.flush();
#endif
  delay(2); // make sure tx buf is empty before going back to sleep
#endif
}

void blink(int led, int count, int length) {
  for (int i=0;i<count;i++) {
    digitalWrite (led, HIGH);
    delay(length);
    digitalWrite (led, LOW);
    delay(length);
  }
}

void debug_ticks(void)
{
#if DEBUG
  blink(ledPin, 1, 15);
#if SERIAL_EN
  Serial.print('.');
  if (pos > MAXLENLINE) {
    pos = 0;
    Serial.println();
  }
  pos++;
#endif
#endif
}

// utility code to perform simple smoothing as a running average
static int smoothedAverage(int prev, int next, byte firstTime =0) {
  if (firstTime)
    return next;
  return ((SMOOTH - 1) * prev + next + SMOOTH / 2) / SMOOTH;
}


#if PIR_PORT

    #define PIR_HOLD_TIME   30  // hold PIR value this many seconds after change
    #define PIR_PULLUP      1   // set to one to pull-up the PIR input pin
    #define PIR_FLIP        0   // 0 or 1, to match PIR reporting high or low

    class PIR : public Port {
        volatile byte value, changed;
        volatile uint32_t lastOn;
    public:
        PIR (byte portnum)
            : Port (portnum), value (0), changed (0), lastOn (0) {}

        // this code is called from the pin-change interrupt handler
        void poll() {
            // see http://talk.jeelabs.net/topic/811#post-4734 for PIR_FLIP
            byte pin = digiRead() ^ PIR_FLIP;
            // if the pin just went on, then set the changed flag to report it
            if (pin) {
                if (!state())
                    changed = 1;
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
                        f = 1 ^ PIR_FLIP;
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
#endif // PIR_PORT

// spend a little time in power down mode while the SHT11 does a measurement
static void shtDelay () {
    Sleepy::loseSomeTime(32); // must wait at least 20 ms
}


#if _RFM12B
/* HopeRF RFM12B 868Mhz digital radio routines */

// wait a few milliseconds for proper ACK to me, return true if indeed received
static byte waitForAck() {
    MilliTimer ackTimer;
    while (!ackTimer.poll(ACK_TIME)) {
        if (rf12_recvDone() && rf12_crc == 0 &&
                // see http://talk.jeelabs.net/topic/811#post-4712
                rf12_hdr == (RF12_HDR_DST | RF12_HDR_CTL | myNodeID))
            return 1;
        set_sleep_mode(SLEEP_MODE_IDLE);
        sleep_mode();
    }
    return 0;
}
#endif //_RFM12B

#if _NRF24
/* Nordic nRF24L01+ routines */

#endif //_NRF24
#if _DS
#endif // _DS
#if _HMC5883L
/* Digital magnetometer I2C module */

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
#endif //_HMC5883L


/* Initialization routines */

void doConfig(void)
{
}

void initLibs()
{
#if _RFM12B
#if SERIAL_EN && DEBUG
  myNodeID = rf12_config();
  serialFlush();
#else
  myNodeID = rf12_config(0); // don't report info on the serial port
#endif

  rf12_sleep(RF12_SLEEP); // power down
#endif // _RFM12B

#if PIR_PORT
  pir.digiWrite(PIR_PULLUP);
#ifdef PCMSK2
  bitSet(PCMSK2, PIR_PORT + 3);
  bitSet(PCICR, PCIE2);
#else
  //XXX TINY!
#endif
#endif // PIR_PORT

#if _NRF24
#endif //_NRF24

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

void doReset(void)
{
  tick = 0;

#if _NRF24
  rf24_init();
#endif //_NRF24

  reportCount = REPORT_EVERY;     // report right away for easy debugging
  scheduler.timer(MEASURE, 0);    // start the measurement loop going
}

/* Run-time handlers */

bool doAnnounce()
{
#if SERIAL_EN
  Serial.print(node_id);
  Serial.print(" ");
  Serial.print(F(" ctemp"));
#endif
#if _MEM
#if SERIAL_EN
  Serial.print(F(" memfree"));
#endif
#endif
}


// readout all the sensors and other values
void doMeasure(void)
{
  byte firstTime = payload.ctemp == 0; // special case to init running avg

  int ctemp = internalTemp(config.temp_offset, config.temp_k);
  payload.ctemp = smoothedAverage(payload.ctemp, ctemp, firstTime);
#if SERIAL_EN && DEBUG_MEASURE
  Serial.println();
  Serial.print("AVR T new/avg ");
  Serial.print(ctemp);
  Serial.print(' ');
  Serial.println(payload.ctemp);
#endif

#if SHT11_PORT
#ifndef __AVR_ATtiny84__
  sht11.measure(SHT11::HUMI, shtDelay);
  sht11.measure(SHT11::TEMP, shtDelay);
  float h, t;
  sht11.calculate(h, t);
  int humi = h + 0.5, temp = 10 * t + 0.5;
#else
  //XXX TINY!
  int humi = 50, temp = 25;
#endif
  payload.humi = smoothedAverage(payload.humi, humi, firstTime);
  payload.temp = smoothedAverage(payload.temp, temp, firstTime);
#endif //SHT11_PORT
#if PIR_PORT
  payload.moved = pir.state();
#endif
#if LDR_PORT
  ldr.digiWrite2(1);  // enable AIO pull-up
  byte light = ~ ldr.anaRead() >> 2;
  ldr.digiWrite2(0);  // disable pull-up to reduce current draw
  payload.light = smoothedAverage(payload.light, light, firstTime);
#endif //_LDR
#if _DHT
#endif //_DHT

#if _DS
#endif //_DS

#if _HMC5883L
  payload.magnx = smoothedAverage(payload.magnx, HMC5803L_Read(X), firstTime);
  payload.magny = smoothedAverage(payload.magny, HMC5803L_Read(Y), firstTime);
  payload.magnz = smoothedAverage(payload.magnz, HMC5803L_Read(Z), firstTime);
#endif //_HMC5883L

#if _MEM
  payload.memfree = freeRam();
#endif //_MEM
#if _RFM12LOBAT
  payload.lobat = rf12_lowbat();
#endif
}

// periodic report, i.e. send out a packet and optionally report on serial port
void doReport(void)
{
#if _RFM12B
  rf12_sleep(RF12_WAKEUP);
  while (!rf12_canSend())
    rf12_recvDone();
  rf12_sendStart(0, &payload, sizeof payload, RADIO_SYNC_MODE);
  rf12_sleep(RF12_SLEEP);
#endif

#if SERIAL_EN
  /* Report over serial, same fields and order as announced */
  Serial.print(node_id);
#if LDR_PORT
  Serial.print((int) payload.light);
  Serial.print(' ');
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
#if _DS
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

// send packet and wait for ack when there is a motion trigger
void doTrigger(void)
{
#if DEBUG
  Serial.print("PIR ");
  Serial.print((int) payload.moved);
  serialFlush();
#endif

#if _RFM12B
  for (byte i = 0; i < RETRY_LIMIT; ++i) {
    rf12_sleep(RF12_WAKEUP);
    while (!rf12_canSend())
      rf12_recvDone();
    rf12_sendStart(RF12_HDR_ACK, &payload, sizeof payload, RADIO_SYNC_MODE);
    byte acked = waitForAck();
    rf12_sleep(RF12_SLEEP);

    if (acked) {
#if DEBUG
      Serial.print(" ack ");
      Serial.println((int) i);
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
  Serial.println(" no ack!");
  serialFlush();
#endif
}
#endif // PIR_PORT

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
      serialFlush();
      break;

    case REPORT:
      doReport();
      serialFlush();
      break;

#if DEBUG
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



/* *** Main *** {{{ */


void setup(void)
{
#if SERIAL_EN
  mpeser.begin();
  mpeser.startAnnounce(sketch, String(version));
  doAnnounce();
  serialFlush();

#if DEBUG
  Serial.print(F("SRAM used: "));
  Serial.println(usedRam());
#endif
#endif

  initLibs();

  doReset();
}

void loop(void)
{
  debug_ticks();

#if PIR_PORT
  if (pir.triggered()) {
    payload.moved = pir.state();
    doTrigger();
  }
#endif

  char task = scheduler.pollWaiting();
  if (task == 0xFF) return; // -1
  if (task == 0xFE) return; // -2
  runScheduler(task);
}
