/*
  AnalogReadSerial
 Reads an analog input on pin 0, prints the result to the serial monitor 
 
 This example code is in the public domain.
 */

#include "Arduino.h"

void setup() {
  Serial.begin(57600);
  //Serial1.begin(9600);
  Serial.println("Testing 1284");
  for( int i=0 ; i<14 ; i++ )
  {
    pinMode(i, OUTPUT);
  }
}

void loop() {
  for( int i=0 ; i<14 ; i++ )
  {
    digitalWrite(i, 1);
  }
  
  int sensorValue = analogRead(A0);
  Serial.println(sensorValue);
  delay(50);

  for( int i=0 ; i<14 ; i++ )
  {
    digitalWrite(i, 0);
  }
  
  sensorValue = analogRead(A0);
  Serial.println(sensorValue);
  
  Serial.print('.');
  delay(2100);
}

