/**
 * 
 */
#define SERIAL 0

#include <JeeLib.h>
#include <avr/sleep.h>

// has to be defined because we're using the watchdog for low-power waiting
ISR(WDT_vect) { 
  Sleepy::watchdogEvent(); 
}

//Pin connected to ST_CP of 74HC595
int latchPin = 1;
//Pin connected to SH_CP of 74HC595
int clockPin = 2;
//Pin connected to DS of 74HC595
int dataPin = 4;

int outputEnable = 0;

// Testing on 5 pins (MSB, so starting at 
int maxPins = 8;

void shift(int q)
{
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, 1 << q);
  digitalWrite(latchPin, HIGH);
}

void setup() 
{
#if SERIAL
  Serial.begin( 57600 );
#endif

  pinMode(outputEnable, OUTPUT);
  digitalWrite(outputEnable, LOW);
  //set pins to output so you can control the shift register
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
}

void loop() 
{
  for (int pinNumber = 0; pinNumber < maxPins; pinNumber++) 
  {
    shift(pinNumber);

#if SERIAL
    Serial.println(pinNumber);
#endif

    //Sleepy::loseSomeTime(300);
  }
  Sleepy::loseSomeTime(300);
}



