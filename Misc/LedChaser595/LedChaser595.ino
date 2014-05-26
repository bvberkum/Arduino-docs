/* LucidTronix Shift Register
 * LED chaser w/ 74HC595 
 * Tutorial at:
 * http://www.lucidtronix.com/tutorials/2
 */
 
int latchPin = 1; //Pin connected to ST_CP of 74HC595
int clockPin = 2; //Pin connected to SH_CP of 74HC595
int dataPin = 1; //Pin connected to DS of 74HC595

void setup() {
  //set pins to output so you can control the shift register
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
}

void loop() {
  for (int i = 0; i < 8; i++) {
    // latchPin low so LEDs don't change 
    // while you are sending in bits:
    byte curb = 1 << i;
    digitalWrite(latchPin, LOW); 
    shiftOut(dataPin, clockPin,  curb); 
    digitalWrite(latchPin, HIGH);
    delay(500);
  }
}

// Shifts 8 bits out MSB first, on rising clock edge
void shiftOut(int myDataPin, int myClockPin, byte myDataOut) {
  int i=0;
  int pinState;
  pinMode(myClockPin, OUTPUT);
  pinMode(myDataPin, OUTPUT);
  digitalWrite(myDataPin, 0);
  digitalWrite(myClockPin, 0); 
  for (i=7; i>=0; i--)  {
    digitalWrite(myClockPin, 0);
    if ( myDataOut & (1<<i) ) {
      pinState= 1;
    }
    else {	
      pinState= 0;
    }
    digitalWrite(myDataPin, pinState); //write the bit
    digitalWrite(myClockPin, 1); //shift bits rising clock pin 
    digitalWrite(myDataPin, 0);//0 data to prevent bleed through
  }
  //stop shifting
  digitalWrite(myClockPin, 0);
}
