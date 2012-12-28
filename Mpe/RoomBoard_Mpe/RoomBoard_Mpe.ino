#include <Ports.h>
#include <JeeLib.h>

  DHTxx dht (5); // connected to DIO2

void setup () {
  Serial.begin(57600);
  Serial.println("\n[dht_demo]");
}

void loop () {
  int t, h;
  if (dht.reading(t, h)) {
    Serial.print("temperature = ");
    Serial.println(t);
    Serial.print("humidity = ");
    Serial.println(h);
    Serial.println();
  }
  else
  Serial.println('.');
  delay(3000);
}
