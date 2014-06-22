/**
 * ATTiny25/45/85
 *                                  t85
 *                              +----v----+
 *         (RST/dW/ADC0) D5 PB5 | 1     8 | VCC
 *   (ADC3/XTAL1/PCINT3) D3 PB3 | 2     7 | PB2 D2  (ADC1/SCK/SCL/PCINT2)
 *   (ADC2/XTAL2/PCINT4) D4 PB4 | 3     6 | PB1 *D1 (MISO/PCINT1)
 *                          GND | 4     5 | PB0 *D0 (MOSI/SDA/AREF/PCINT0)
 *                              +---------+
 *
 * - Marked '*' has PWM?
 * - Other site remarks
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>

#define IN_PIN (1<<PB3)
#define OUT_PIN (1<<PB4)


void setup() {
	pinMode(0, OUTPUT);
	pinMode(1, OUTPUT);
	pinMode(2, OUTPUT);
	//pinMode(3, OUTPUT);
	pinMode(3, INPUT);
	pinMode(4, OUTPUT);
//	digitalWrite(A3, HIGH);
//	DDRB |= (1 << 0);
//	TCCR1B |= ((1 << CS10) | (1 << CS11));
//	digitalWrite(3, LOW);
}

int top = 0xff;
//int top = 0x3ff;
int dc = top;

void loop() {
	digitalWrite(4, HIGH);
 	delay(2);
	int dc = analogRead(3)/4;
	analogWrite(0, dc);
	digitalWrite(4, LOW);
 	delay(100);
	return;

	//if ((int)dc >= 128) {
	if ((int)dc == 0) {
		delay(2500);
		dc = top;
	}
	if (dc == top-1) {
//		delay(500);
	}
	dc -= 1;
	delay(10);

//	analogWrite(A1, (int)dc);
//	if (TCNT1 >= 15624)
//	{
//		PORTB ^= LED2_PIN;//(1 << 0); // toggle led
//		TCNT1 = 0;
//	}
}

