int start = 5;
int pins = 3;

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
  Serial.println("Atmega328P Blink");
  for (int i=start;i<start+pins;i++) {
	  pinMode(i, OUTPUT);
	  digitalWrite(i, LOW);
  }
}

void loop() 
{
  for (int i=start;i<start+pins;i++) {
	  Serial.print("At pin ");
	  Serial.println(i);
	  blink(i,2,75);
	  delay(100);
  }
  delay(1000);
}

