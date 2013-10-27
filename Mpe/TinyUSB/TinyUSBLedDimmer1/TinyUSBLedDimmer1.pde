/**
 * 
 * Incomplete, need to add USB code.
 * 
 * - BC547 with base on D1 (PB0)
 * - Two bright white LEDs hooked between emitter and 20 ohm + GND.
 * - Another BC547 on D4 to enable 5V over a 100k pot + 100k resistor devider.
 * - A3 measures the divider and sets dc value, allowing adjustment of
 *   brightness by trimmer.
 * 
 * ATTiny25/45/85
 *                                   t85
 *                               +----v----+
 *         (RST/dW/ADC0)  D5 PB5 | 1     8 | VCC
 *   (ADC3/XTAL1/PCINT3)  D3 PB3 | 2     7 | PB2 D2 (ADC1/SCK/SCL/PCINT2)
 *   (ADC2/XTAL2/PCINT4) *D4 PB4 | 3     6 | PB1 *D1   (MISO/PCINT1)
 *                           GND | 4     5 | PB0 *D0   (MOSI/SDA/AREF/PCINT0)
 *                               +---------+
 *
 * - Marked '*' has PWM?
 * - Other site remarks PWM is possible on the OC pins D0, D1, D3 and D4, but D1
 *   and D3 would be inverted.
 *   http://forum.arduino.cc//index.php?PHPSESSID=7smf5ck710q42rtai61m659hh2&topic=156118.msg1169902#msg1169902
 *
 * Todo:
 * - noise is low but noticable at low levels and potmeter is not that
 *   smooth, should add software smoothing.

 * With LED off, this takes about 12mA, in sleep mode more like 0.02mA.
 * Ofcourse compared to the LED this is nothing, but low quiescence is just a good
 * engineering choice.
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <util/delay.h>

#define M_DELAY_MS 4
#define REPORT_DELAY_MS 400
#define WATCHDOG_SLEEP 7
// 0=16ms, 1=32ms,2=64ms,3=128ms,4=250ms,5=500ms
// 6=1 sec,7=2 sec, 8=4 sec, 9= 8sec

#define M_PIN 3
#define L_PIN 0
#define R_PIN 4

int top = 0xff; // max val of DAC
//int top = 0x3ff; // max val of ADC
int dc = top;
volatile boolean f_wdt = 0;
volatile int mode = 2;

void setup_watchdog(int ii);
void system_sleep();

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif


void setup() {
	pinMode(M_PIN, INPUT);
	pinMode(L_PIN, OUTPUT);
	pinMode(R_PIN, OUTPUT);
	wdt_enable(WATCHDOG_SLEEP);
	delay(500);
}

void loop() {
	digitalWrite(R_PIN, HIGH); // enable resistor divider
 	delay(M_DELAY_MS); // tiny delay to stabalize
	int dc = analogRead(M_PIN)/4; // read ADC and scale to DAC range
	if (dc < 2) { // lower threshold 
		dc = 0; // switches LED off
		//digitalWrite(L_PIN, LOW);
		digitalWrite(R_PIN, LOW);
		//pinMode(L_PIN, INPUT);
		pinMode(R_PIN, INPUT);
		system_sleep();
		//pinMode(L_PIN, OUTPUT);
		pinMode(R_PIN, OUTPUT);
	} else {
		digitalWrite(R_PIN, LOW); // disable resistor divider
	}
	analogWrite(L_PIN, dc); // set new level for LED

/* LED only testing: linear ramp down:
	//if ((int)dc >= 128) {
	if ((int)dc == 0) {
		delay(2500);
		dc = top;
	}
	if (dc == top-1) {
//		delay(500);
	}
	if (dc == top-1) {
//		delay(500);
	}
	dc -= 1;
	delay(10);
return;
*/

//	if (mode == 0) {
//		//WDTCR |= (1<<WDCE) | (1<<WDE); // Watchdog change enable and Watchdog enable
//		//MCUSR &= ~(1<<WDRF); // set Watchdog Reset Flag to 0?
//		//WDTCR |= (1<<WDIE); // Wd Interrupt Enable
//		return;
//	} else if (mode == 1) {
//		//cli();
//		//wdt_reset();
//		//sei();
//		mode = 2;       
//		return;
//	}
	wdt_reset();
	if (f_wdt) {
		f_wdt = 0;
	} else {
		delay(REPORT_DELAY_MS);
	}
}

// set system into the sleep state 
// system wakes up when wtchdog is timed out__
void system_sleep() {
	cbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter OFF

	set_sleep_mode(SLEEP_MODE_PWR_DOWN); // sleep mode is set here
	sleep_enable();

	sleep_mode();                        // System sleeps here

	sleep_disable();                     // System continues execution here when watchdog timed out 
	sbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter ON
}

// 0=16ms, 1=32ms,2=64ms,3=128ms,4=250ms,5=500ms
// 6=1 sec,7=2 sec, 8=4 sec, 9= 8sec
void setup_watchdog(int ii) {
	byte bb; // our new Wd WTCR value
	//int ww; // huh?
	if (ii > 9 ) ii=9; // cap value to B1001
	bb=ii & 7; // cut off lower 3 bits
	if (ii > 7) bb|= (1<<5); // set WDP3 if 4 bits
	bb|= (1<<WDCE); // add Wd Change Enable
	//ww=bb;
	//MCUSR &= ~(1<<WDRF); // set Watchdog Reset Flag to 0?
	// start timed sequence
//	WDTCR |= (1<<WDCE) | (1<<WDE); // Watchdog change enable and Watchdog enable
	// set new watchdog timeout value
	WDTCR = bb; //
//	WDTCR |= _BV(WDIE); // Wd Interrupt Enable
}

// Watchdog Interrupt Service / is executed when watchdog timed
// out
ISR(WDT_vect) {
	f_wdt = 1;
//	WDTCR |= (1<<WDIE); // Wd Interrupt Enable
}
