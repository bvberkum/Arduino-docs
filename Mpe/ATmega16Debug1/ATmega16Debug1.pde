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
	Serial.println("Atmega16A Debug");
	Serial.println("Blink pin 8 five times, BL_PROG blinks pin 9 continuously");

	// Set up the LED output pins
	pinMode (8, OUTPUT);
	pinMode (9, OUTPUT);
        pinMode (23, INPUT);
        digitalWrite (23, HIGH); // pull up
	digitalWrite (8, LOW);
	digitalWrite (9, LOW);
}

void loop() 
{
	blink(8,5,80);
        while (digitalRead(23) == LOW) { 
          // blink while switch to ground is pressed
	  blink(9,5,80);
        }
	delay(1000);
}

