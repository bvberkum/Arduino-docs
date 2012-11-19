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
  Serial.println("Blink digital pin 8 five times");
  pinMode(8, OUTPUT);
  digitalWrite(8, LOW);
}

void loop() 
{
  blink(8,5,80);
  delay(1000);
}
