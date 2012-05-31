void setup () {
	Serial.begin(57600);
	Serial.println("");
	Serial.println("simple IO");

	pinMode(A0, OUTPUT);
	pinMode(A1, OUTPUT);
	pinMode(A2, OUTPUT);
	pinMode(A3, OUTPUT);
}

void loop () {
	int di1 = digitalRead(4);
	int di2 = digitalRead(5);
	int di3 = digitalRead(6);
	int di4 = digitalRead(7);

	digitalWrite(A0, di1);
	digitalWrite(A1, di2);
	digitalWrite(A2, di3);
	digitalWrite(A3, di4);

	//Serial.print("v:");
	//Serial.print(di1);
	//Serial.print(di2);
	//Serial.print(di3);
	//Serial.print(di4);
	//Serial.println();

	//Serial.print('.');
	delay(100);
}
