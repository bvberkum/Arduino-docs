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
  Serial.begin(57600);
  Serial.println("Atmega16A Blink");
  Serial.println("Blink digital pin 8 five times");
  pinMode(5, OUTPUT);
  digitalWrite(5, LOW);
  pinMode(6, OUTPUT);
  digitalWrite(6, LOW);
  pinMode(7, OUTPUT);
  digitalWrite(7, LOW);
}

void loop() 
{
  blink(5,5,80);
  blink(6,5,80);
  blink(7,5,80);
  delay(1000);
}

