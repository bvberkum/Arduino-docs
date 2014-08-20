/**
 * Creates a running light on my LedBarOct plug.
 */
#define SERIAL 0


//Pin connected to ST_CP of 74HC595
int latchPin = 1;
//Pin connected to SH_CP of 74HC595
int clockPin = 2;
//Pin connected to DS of 74HC595
int dataPin = 4;

int outputEnable = 0;

// Testing off 5 outputs (MSB, so starting at)
int maxPos = 9;


void shift(byte q)
{
	digitalWrite(latchPin, LOW);
	shiftOut(dataPin, clockPin, MSBFIRST, q);
	digitalWrite(latchPin, HIGH);
}

void setup() 
{   
	pinMode(outputEnable, OUTPUT);
	digitalWrite(outputEnable, LOW);
	//set pins to output so you can control the shift register
	pinMode(latchPin, OUTPUT);
	pinMode(clockPin, OUTPUT);
	pinMode(dataPin, OUTPUT);
}
void loop() 
{
	for (int pos = 0; pos <= maxPos; pos++) 
	{
		shift(1 << pos);
		delay(500);
	}
}
