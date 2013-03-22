/*
 Morse.cpp
 Sample for the FiniteStateMachine Tutorial at http://www.mathertel.de/Arduino/FiniteStateMachine.aspx

 Copyright (c) by Matthias Hertel, http://www.mathertel.de
 This work is licensed under a BSD style license. See http://www.mathertel.de/License.aspx
 More information on: http://www.mathertel.de/Arduino

 * 03.03.2011 created by Matthias Hertel
 * 01.12.2011 include file changed to work with the Arduino 1.0 environment
*/

#include "Morse.h"

// ----- Initialization -----

Morse::Morse(int pin)
{
  _pin = pin;
  _unitTicks = DefaultUnitTicks;        // # millisec after single click is assumed.

  pinMode(pin, OUTPUT);      // sets the MenuPin as input
  digitalWrite(pin, LOW);   // turn on pulldown resistor

  _state = 0; // starting with state 0: waiting for a code to be sent.
} // Morse


void Morse::setUnitTicks(int ticks) { 
  _unitTicks = ticks;
} // setUnitTicks


void Morse::sendCode(char *msg)
{
  if (_state == 0) {
    _nextCode = msg;
    _state = 1;
  } 
}

void Morse::tick(void)
{
  // Detect the input information 
  int buttonLevel = digitalRead(_pin); // current button signal.
  unsigned long now = millis(); // current (relative) time in msecs.

  // Implementation of the state machine
  if (_state == 0) { // waiting for menu pin being pressed.

  } else if (_state == 1) { // check next character
    if ((_nextCode == 0L) || (*_nextCode == '\0')) {
      _state = 0;
      
    } else if (*_nextCode == '.') {
      // start sending a dot.
      _state = 2;
      _startTime = now;
      digitalWrite(_pin, HIGH);
      
    } else if (*_nextCode == '-') {
      // start sending a dash.
      _state = 3;
      _startTime = now;
      digitalWrite(_pin, HIGH);

    } else if (*_nextCode == ' ') {
      // start sending a gap.
      _state = 4;
      _startTime = now;
    } // if
    
  } else if (_state == 2) { // wait for end of a dot.
    if (now > _startTime + 1 * _unitTicks) {
      // stop
      _state = 9;
      _startTime = now;
      digitalWrite(_pin, LOW);
    } // if
    
  } else if (_state == 3) { // wait for end of a dash.
    if (now > _startTime + 3 * _unitTicks) {
      // stop
      _state = 9;
      _startTime = now;
      digitalWrite(_pin, LOW);
    } // if

  } else if (_state == 4) { // wait for end of a gap.
    if (now > _startTime + 3 * _unitTicks)
      _state = 10;

  } else if (_state == 9) { // wait a single gap.
    if (now > _startTime + 1 * _unitTicks) {
      _state = 10;
    } // if

  } else if (_state == 10) { // next element.
    _nextCode += 1;
    _state = 1;

  } // if  
} // Morse.tick()


// end.

