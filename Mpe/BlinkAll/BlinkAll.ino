int first_led = 5;
int last_led = 7;

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
  Serial.println("Atmega328p Blink All");
  // Set up the LED output pins
  for (int p=first_led; p<=last_led;p++) {
    pinMode(p, OUTPUT);
    digitalWrite(p, LOW);
  }
}

void loop() 
{
  for (int p=first_led; p<=last_led;p++) {
    Serial.print("pin ");
    Serial.println(p);
    blink(p,10,50);
    delay(500);
  }
  delay(1000);
} 
