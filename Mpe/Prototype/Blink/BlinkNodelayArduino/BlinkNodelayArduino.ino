/*
   BlinkNodelayArduino - use millis for non-blocking blink
*/
int blinkPin = 2;
int blinkCount = 1;
int blinkDelay = 1000;
long delay_loop = 0;
int blinkActive = HIGH;

int ledState = LOW;             // ledState used to set the LED
long previousMillis = 0;        // will store last time LED was updated
int blinkCounter = -1;


void blinkNodelay(int led, int count) {
	unsigned long currentMillis = millis();
	if (blinkCounter == -1) {
		blinkCounter = count;
	}
	if (blinkCounter > -1 && currentMillis - previousMillis > blinkDelay) {
		previousMillis = currentMillis;
		if (ledState == blinkActive) {
			blinkCounter--;
			ledState = ! blinkActive;
		} else { 
			ledState = blinkActive;
		}
		digitalWrite(led, ledState);
	}
}

void setup()
{
	pinMode(blinkPin, OUTPUT);
	digitalWrite(blinkPin, ! blinkActive);
}
void loop()
{
	blinkNodelay(blinkPin, blinkCount);
	if (blinkCounter == -1) {
		delay(delay_loop); // no millis, just delay
//		blinkCounter = blinkCount;
	}
}
