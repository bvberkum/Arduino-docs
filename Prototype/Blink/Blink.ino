/**
 * DIP:
 *                            m328
 *                        +----v----+
 *          (RST)     PC6 | 1    28 | PC5 A5  (ADC5/SCL)
 *          (TXD)  D0 PD0 | 2    27 | PC4 A4  (ADC4/SDA)
 *          (RXD)  D1 PD1 | 3    26 | PC3 A3  (ADC3)
 *         (INT0)  D2 PD2 | 4    25 | PC2 A2  (ADC2)
 *         (INT1) *D3 PD3 | 5    24 | PC1 A1  (ADC1)
 *       (XCK/T0)  D4 PD4 | 6    23 | PC0 A0  (ADC0)
 *                    VCC | 7    22 | GND
 *                    GND | 8    21 | AREF
 *  (XTAL1/TOSC1)     PB6 | 9    20 | AVCC
 *  (XTAL2/TOSC2)     PB7 | 10   19 | PB5 D13 (SCK)
 *           (T1) *D5 PD5 | 11   18 | PB4 D12 (MISO)
 *         (AIN0) *D6 PD6 | 12   17 | PB3 D11* (MOSI/OC2)
 *         (AIN1)  D7 PD7 | 13   16 | PB2 D10* (SS/OC1B)
 *         (ICP1)  D8 PB0 | 14   15 | PB1 D9 * (OC1A)
 *                        +---------+
 *
 * ATTiny25/45/85
 *                                     t85
 *                                 +----v----+
 *         (RST/dW/ADC0) D5/A0 PB5 | 1     8 | VCC
 *   (ADC3/XTAL1/PCINT3) D3/A3 PB3 | 2     7 | PB2 D2/A1 (ADC1/SCK/SCL/PCINT2)
 *   (ADC2/XTAL2/PCINT4) D4/A2 PB4 | 3     6 | PB1 *D1   (MISO/PCINT1)
 *                             GND | 4     5 | PB0 *D0   (MOSI/SDA/AREF/PCINT0)
 *                                 +---------+
 *
 * - Marked '*' has PWM.
 */
int blinkPin = 3;
int blinkCount = 2;
int delay_low = 150;
int delay_high = 10;
long delay_loop = 1000;

void blink(int led, int count) {
  unsigned long currentMillis = millis();
  for (int i=0;i<count;i++) {
    digitalWrite(led, HIGH);
    delay(delay_high);
    digitalWrite(led, LOW);
    delay(delay_low);
  }
}

void setup() 
{
#if !defined(__AVR_ATtiny85__)
  Serial.begin(57600);
  Serial.println("BlinkNoDelay");
#endif
  pinMode(blinkPin, OUTPUT);
  digitalWrite(blinkPin, LOW);
//  delay(5);
//  blink(pin, 5);
//  delay(1500);
}

void loop() 
{
  blink(blinkPin, blinkCount);
  delay(delay_loop);
}

/*
	TQFP:
           PD2 1 0 PC6 5 4 3 2  
           32      29        25 
          .-------------------. 
	PD3 1 |O                  | 24 PC1
	PD4 2 |                   | 23 PC0
	GND 3 |                   | 22 ADC7 (*)
	VCC 4 |       m8          | 21 GND
	GND 5 |                   | 20 AREF
	VCC 6 |                   | 19 ADC6 (*)
	PB6 7 |                   | 18 AVCC
	PB7 8 |                   | 17 PB5
		  '-------------------' 
	       9       12        16 
	       PD5 6 7 PB0 1 2 3 4  

	- ports/signals same as PDIP, except: 
	* denotes extra ADC pins on TQFP */
