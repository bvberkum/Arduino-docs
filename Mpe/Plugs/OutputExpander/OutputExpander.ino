/**
 * 
 */
#define SERIAL 0


//Pin connected to ST_CP of 74HC595
int latchPin = 1;
//Pin connected to SH_CP of 74HC595
int clockPin = 2;
//Pin connected to DS of 74HC595
int dataPin = 4;

int outputEnable = 0;

// Testing on 5 pins (MSB, so starting at 
int maxPins = 9;

void shift(int q)
{
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, 1 << q);
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
int x = 0;
void loop() 
{/*
  for (int pinNumber = 0; pinNumber <= maxPins; pinNumber++) 
  {
    shift(pinNumber);
#if SERIAL
    Serial.println(pinNumber);
#endif  
    delay(120);
  }
  delay(800);
*/
//delay(100);
  delay(x);
  digitalWrite(latchPin, LOW);


  shiftOut(dataPin, clockPin, MSBFIRST, 0b11001100);
  
  delay(10);
  digitalWrite(latchPin, HIGH);

  
  x += 10;
  if (x==0xf) x = 0;
}
