/*
 FiniteStateMachine.pde
 Sample for the FiniteStateMachine Tutorial at http://www.mathertel.de/Arduino/FiniteStateMachine.aspx

 Copyright (c) by Matthias Hertel, http://www.mathertel.de
 This work is licensed under a BSD style license. See http://www.mathertel.de/License.aspx
 More information on: http://www.mathertel.de/Arduino

 The circuit:
 * See results on pin 13 (build-in LED in Arduino).
 * 03.03.2011 created by Matthias Hertel
 * 01.12.2011 extension changed to work with the Arduino 1.0 environment
*/

#include "Morse.h"

// Setup the morse machine with LED on pin 13.  
Morse morse(13);

// setup code here, to run once:
void setup() {
} // setup
  
// main code here, to run repeatedly: 
void loop() {
  static int n = 0;
  
  // keep watching the push button:
  morse.tick();

  // simulate an external event
  n = n + 1;
  if (n == 300) {
    digitalWrite(13, HIGH);
    morse.sendCode(".- .-. -.. ..- .. -. ---");
  } // if

  // You can implement other code in here or just wait a while 

  delay(10);
} // loop


