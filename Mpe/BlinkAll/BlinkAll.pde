void blink(int led, int count, int length) {
  for (int i=0;i<count;i++) {
    digitalWrite (led, HIGH);
    delay(length);
    digitalWrite (led, LOW);
    delay(length);
  }
}

void setup() 
{
  Serial.begin(38400);
  Serial.println("Atmega16A Blink");
  Serial.println("Blink digital pins 1-23");
  // Set up the LED output pins
  for (int p=0; p<24;p++) {
    pinMode(p, OUTPUT);
    digitalWrite(p, LOW);
  }
}

void loop() 
{
  for (int p=0; p<24;p++) {
    blink(p,5,80);
  }
  delay(1000);
}
