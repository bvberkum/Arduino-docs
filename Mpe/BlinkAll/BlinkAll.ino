int first_led = 14;
int last_led = 15;

void blink(int led, int count, int length) {
  for (int i=0; i<count; i++) {
    digitalWrite (led, HIGH);
    delay(length);
    digitalWrite (led, LOW);
    delay(length);
  }
}

void setup() 
{
  Serial.begin(57600);
  Serial.println("Atmega328p Blink");
  Serial.println("Blink pins");
  // Set up the LED output pins
  for (int p=first_led; p<=last_led;p++) {
    pinMode(p, OUTPUT);
    digitalWrite(p, LOW);
  }
}

void loop() 
{
  for (int p=first_led; p<=last_led;p++) {
    Serial.println(p);
    blink(p,5,80);
  }
  delay(1000);
}
