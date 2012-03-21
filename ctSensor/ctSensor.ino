int inputPin = A0;
int ledPin = 13;      // LED

// Define the number of samples to keep track of.  The higher the number,
// the more the readings will be smoothed, but the slower the output will
// respond to the input.  Using a constant rather than a normal variable lets
// use this value to determine the size of the readings array.
const int numReadings = 10;

int readings[numReadings];      // the readings from the analog input
int index = 0;                  // the index of the current reading
int total = 0;                  // the running total
int average = 0;                // the average


void setup()
{
	// initialize serial communication with computer:
	Serial.begin(9600);
	//    Serial.begin(57600);
	Serial.print("\n[ctSensor]");
	// initialize all the readings to 0:
	for (int thisReading = 0; thisReading < numReadings; thisReading++)
	readings[thisReading] = 0;
	// declare the ledPin as an OUTPUT:
	pinMode(ledPin, OUTPUT);

}

void loop () {

	// subtract the last reading:
	total = total - readings[index];
	// read from the sensor:
	readings[index] = analogRead(inputPin);
	// add the reading to the total:
	total = total + readings[index];
	// advance to the next position in the array:
	index = index + 1;

	// if we're at the end of the array...
	if (index >= numReadings)
		// ...wrap around to the beginning:
		index = 0;

	// calculate the average:
	average = total / numReadings;
	// send it to the computer as ASCII digits
	Serial.println(average);

	// turn the ledPin on
	//	digitalWrite(ledPin, HIGH);
	//	// stop the program for <average> milliseconds:
	//	delay(average);
	//	// turn the ledPin off:
	//	digitalWrite(ledPin, LOW);
	//	// stop the program for for <average> milliseconds:
	//	delay(average);

	delay(250);
}
