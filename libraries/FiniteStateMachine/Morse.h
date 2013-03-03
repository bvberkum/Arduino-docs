/*
 Morse.h
 Sample for the FiniteStateMachine Tutorial at http://www.mathertel.de/Arduino/FiniteStateMachine.aspx

 Copyright (c) by Matthias Hertel, http://www.mathertel.de
 This work is licensed under a BSD style license. See http://www.mathertel.de/License.aspx
 More information on: http://www.mathertel.de/Arduino

 * 03.03.2011 created by Matthias Hertel
 * 01.12.2011 include file changed to work with the Arduino 1.0 environment
*/

#ifndef Morse_h
#define Morse_h

#include "Arduino.h"

// ----- Callback function types -----

extern "C" {
  typedef void (*callbackFunction)(void);
}


class Morse
{
public:
  // ----- Constants -----
  static const int DefaultLedPin =  13;  // Output pin for the morse code. 
  static const int DefaultUnitTicks = 120; // slow enough for beginners

  // ----- Constructors -----
  Morse(int pin);

  // ----- Runtime Parameters -----

  // set # millisec for one morse unit.
  void setUnitTicks(int ticks);

  // ----- State machine functions -----

  // start sending this message like ".- .-. -.. ..- .. -. ---"
  void sendCode(char *msg);

  // call this function every some milliseconds.
  void tick(void);

private:
  int _pin;
  int _unitTicks;

  // These variables that hold information across the upcomming tick calls.
  // They are declared tatic so the only get initialized once on program start and not
  // every time the tick function is called.
  int _state;
  unsigned long _startTime; // will be set for several states

  char *_nextCode;
};

#endif



