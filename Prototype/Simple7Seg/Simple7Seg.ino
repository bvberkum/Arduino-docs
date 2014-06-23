// Arduino 7 segment display hack
// Edited by .mpe
// http://www.hacktronics.com/Tutorials/arduino-and-7-segment-led.html
// License: http://www.opensource.org/licenses/mit-license.php (Go crazy)
byte seven_seg_digits[10][7] = { 
/*	  b c d e f a g */
	{ 1,1,1,1,1,1,0 },  // = 0
	{ 0,1,1,0,0,0,0 },  // = 1
	{ 1,1,0,1,1,0,1 },  // = 2
	{ 1,1,1,1,0,0,1 },  // = 3
	{ 0,1,1,0,0,1,1 },  // = 4
	{ 1,0,1,1,0,1,1 },  // = 5
	{ 1,0,1,1,1,1,1 },  // = 6
	{ 1,1,1,0,0,0,0 },  // = 7
	{ 1,1,1,1,1,1,1 },  // = 8
	{ 1,1,1,1,0,1,1 }   // = 9
};
int dp = 9;
int ca1 = 12;
int ca2 = 10;

void writeDot(byte dot) {
	digitalWrite(dp, dot);
}

void digitOne() {
	pinMode(ca1, OUTPUT);
	digitalWrite(ca1, HIGH);
	pinMode(ca2, INPUT);
	digitalWrite(ca2, HIGH);
}
void digitTwo() {
	pinMode(ca1, INPUT);
	digitalWrite(ca1, LOW);
	pinMode(ca2, OUTPUT);
	digitalWrite(ca2, HIGH);
}

void sevenSegWrite(byte digit) {
	byte pin = 2;
	for (byte segCount = 0; segCount < 7; ++segCount) {
		// write for common-anode (CA)
		digitalWrite(pin, ! seven_seg_digits[digit][segCount]);
		++pin;
	}
}

void sevenSegClear() {
	byte pin = 2;
	while (pin < 9) {
		digitalWrite(pin, HIGH);
		++pin;
	}
}

void activeDemuxPrintDigits(int count) {
	for (int x=0; x<50; x++) {
		digitTwo();
		sevenSegWrite(count % 10 ); 
		delay(6);
		writeDot(1);
		digitOne();
		sevenSegWrite(count / 10 ); 
		delay(6);
		writeDot(0);
	}
}

void setup() {
//	DDRD = 0b11111111; // pin 0-7 to output mode
	pinMode(0, OUTPUT); 
	pinMode(2, OUTPUT); // B
	pinMode(3, OUTPUT); // C
	pinMode(4, OUTPUT); // D
	pinMode(5, OUTPUT); // E
	pinMode(6, OUTPUT); // F
	pinMode(7, OUTPUT); // A
	pinMode(8, OUTPUT); // G
	pinMode(dp, OUTPUT); // DP
	pinMode(ca1, OUTPUT); // CA1
	pinMode(ca2, OUTPUT); // CA2
	digitTwo();
	writeDot(1);
	digitOne();
	sevenSegClear();
	sevenSegWrite(3);
}

int x=0;
void loop() {
//	sevenSegWrite(x++);
//	if (x==9) {
//		x=0;
//	}
//	for (int x=0; x<50; x++) {
//		digitOne();
//		delay(12);
//		digitTwo();
//		delay(12);
//	}
	for (byte count = 100; count > 0; --count) {
		activeDemuxPrintDigits(count-1);
	}
}

