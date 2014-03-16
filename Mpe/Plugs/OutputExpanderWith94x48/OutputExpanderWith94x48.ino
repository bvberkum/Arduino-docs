/**
 * 
 */
#define LCD_94x48 0
#define SERIAL 0

//#if !defined(__AVR_ATtiny85__)
/*

 # include <JeeLib.h>
 # include <avr/sleep.h>
 */
// has to be defined because we're using the watchdog for low-power waiting
//ISR(WDT_vect) { Sleepy::watchdogEvent(); }
//#endif

#if LCD_94x48

#include <PCD8544.h>
static const byte LCD_WIDTH = 84;
static const byte LCD_HEIGHT = 48;

static PCD8544 lcd(3, 4, 9, 6, 7); /* SCLK, SDIN, DC, RESET, SCE */
#endif


//Pin connected to ST_CP of 74HC595
int latchPin = 1;
//Pin connected to SH_CP of 74HC595
int clockPin = 2;
//Pin connected to DS of 74HC595
int dataPin = 4;

int outputEnable = 0;

// Testing on 5 pins (MSB, so starting at 
int maxPins = 8;

void shift(int q)
{
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, 1 << q);
  digitalWrite(latchPin, HIGH);
}

void setup() 
{
  /*
#if SERIAL
   Serial.begin( 57600 );
   #endif
   #if LCD_94x48
   lcd.begin(LCD_WIDTH, LCD_HEIGHT);
   lcd.setCursor(0, 0);
   lcd.print("OutputExpander");
   #endif
   */
   
  pinMode(outputEnable, OUTPUT);
  digitalWrite(outputEnable, LOW);
  //set pins to output so you can control the shift register
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
}

void loop() 
{
  for (int pinNumber = 0; pinNumber < maxPins; pinNumber++) 
  {
    shift(pinNumber);
    
#if LCD_94x48
     		lcd.setCursor(0, 1);
     		lcd.print(pinNumber);
     #endif
     #if SERIAL
     		Serial.println(pinNumber);
     #endif
     
    delay(200);
    //Sleepy::loseSomeTime(300);
  }
  //Sleepy::loseSomeTime(300);
}


