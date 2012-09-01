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
	Serial.begin(9600);
	Serial.println("Atmega16A Blink");
	Serial.println("Pins 0, 1, 8, 9");
	// Set up the LED output pins
	pinMode (0, OUTPUT);
        pinMode (1, OUTPUT);
	pinMode (8, OUTPUT);
	pinMode (9, OUTPUT);
	//pinMode (2, OUTPUT);
	//pinMode (3, OUTPUT);
	//pinMode (4, OUTPUT);
	//pinMode (5, OUTPUT);
	//pinMode (6, OUTPUT);
	//pinMode (7, OUTPUT);
	//pinMode (8, OUTPUT);
	//pinMode (9, OUTPUT);
	//pinMode (10, OUTPUT);
	//pinMode (11, OUTPUT);
	//pinMode (12, OUTPUT);
	//pinMode (13, OUTPUT);

	digitalWrite (0, LOW);
	digitalWrite (1, LOW);
	digitalWrite (8, LOW);
	digitalWrite (9, LOW);
	//digitalWrite (2, LOW);
	//digitalWrite (3, LOW);
	//digitalWrite (4, LOW);
	//digitalWrite (5, LOW);
	//digitalWrite (6, LOW);
	//digitalWrite (7, LOW);
	//digitalWrite (8, LOW);
	//digitalWrite (9, LOW);
	//digitalWrite (10, LOW);
	//digitalWrite (11, LOW);
	//digitalWrite (12, LOW);
	//digitalWrite (13, LOW);
}

void loop() 
{
	blink(0,5,80);
	blink(1,5,80);
	blink(8,5,80);
	blink(9,5,80);
	//blink(2,5,80);
	//blink(3,5,80);
	//blink(4,5,80);
	//blink(5,5,80);
	//blink(6,5,80);
	//blink(7,5,80);
	//blink(8,5,80);
	//blink(9,5,80);
	//blink(10,5,80);
	//blink(11,5,80);
	//blink(12,5,80);
	//blink(13,5,80);
	delay(1000);
}

