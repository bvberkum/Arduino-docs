/*
*/

char inData[20];
char inChar = -1;
int index = 0;


void setup() {
	Serial.begin(9600);
	Serial.println("serialRead");
}

void loop() 
{
	while (Serial.available() > 0) {
		if (index < 19) {
			inChar = Serial.read();
			inData[index] = inChar;
			index++;
			inData[index] = '\0';
		}
		else {
			Serial.println("SerialCommandOverflow");
			while (Serial.available() != 0) {
				Serial.read(); }
		}
		delay(10);
	}
	if (inData[0] != 0) {
		Serial.println(inData);
		for (int i;i<19;i++) {
			inData[i]=0;
		}
		index = 0;
		Serial.println("SerialACK");
	}
	delay (1000); // wait 1 second
}


