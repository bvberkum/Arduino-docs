/*
  AnalogReadSerial
 Reads an analog input on pin 0, prints the result to the serial monitor 
 
 This example code is in the public domain.
 */

#include <Ports.h>
#include <RF12.h>
#include <avr/sleep.h>

void setup() {
  Serial.begin(9600);
}

static void lowPower (byte mode) {
  // prepare to go into power down mode
  set_sleep_mode(mode);
  // disable the ADC
  byte prrSave = PRR, adcsraSave = ADCSRA;
  ADCSRA &= ~ bit(ADEN);
  PRR &= ~ bit(PRADC);
  // zzzzz...
  sleep_mode();
  // re-enable the ADC
  PRR = prrSave;
  ADCSRA = adcsraSave;
}

static byte loseSomeTime (word msecs) {
  // only slow down for periods longer than twice the watchdog granularity
  if (msecs >= 32) {
    for (word ticks = msecs / 16; ticks > 0; --ticks) {
      lowPower(SLEEP_MODE_PWR_DOWN); // now completely power down
      // adjust the milli ticks, since we will have missed several
      extern volatile unsigned long timer0_millis;
      timer0_millis += 16L;
    }
    return 1;
  }
  return 0;
}

static MilliTimer sleepTimer;   // poll the sensor once every so often
static MilliTimer aliveTimer;   // without change, force sending every 60 s
static byte radioIsOn = 1;      // track whether the RFM12B is on

static byte periodicSleep (word msecs) {
  // switch to idle mode while waiting for the next event
  lowPower(SLEEP_MODE_IDLE);
  // keep the easy tranmission mechanism going
  if (radioIsOn && rf12_easyPoll() == 0) {
    rf12_sleep(0); // turn the radio off
    radioIsOn = 0;
  }
  // if we will wait for quite some time, go into total power down mode
  if (!radioIsOn) {
    // see http://news.jeelabs.org/2009/12/18/battery-life-estimation/
    if (loseSomeTime(sleepTimer.remaining()))
      sleepTimer.set(1); // really did a power down, trigger right now
  }
  // return true if the time has come to do something meaningful
  return sleepTimer.poll(msecs);
}

void loop() {
  //if (periodicSleep(1000)) {

    int sensorValue = analogRead(A0);
    Serial.println(sensorValue);
  //}
}

