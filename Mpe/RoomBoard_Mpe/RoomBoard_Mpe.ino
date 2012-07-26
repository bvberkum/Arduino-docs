#include <Ports.h>
#include <RF12.h>
#include <avr/sleep.h>
#include 

#define DHTTYPE DHT11   // DHT 11

Port port (4);

void setup () {
  Serial.begin(57600);
}

void loop () {
  port.digiWrite2(1); // upp-up AIO
  byte light = port.anaRead() >> 2;
  port.digiWrite(0); // reduce power-draw in bright light
  Serial.println(light);
  delay(100);
}
