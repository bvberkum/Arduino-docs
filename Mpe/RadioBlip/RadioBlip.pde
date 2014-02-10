/// @dir radioBlip
/// Send out a radio packet every minute, consuming as little power as possible.
// 2010-08-29 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php

#include <JeeLib.h>

struct {
	char msgtype;
	long payload;
} payload;
static int sleeptime = 60000;

// this must be added since we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

static void serialFlush () {
#if ARDUINO >= 100
	Serial.flush();
#endif  
	delay(2); // make sure tx buf is empty before going back to sleep
}

void setup() {
    Serial.begin(57600);
    Serial.println("\n[radioBlip/0.1]");
    serialFlush();
    serialFlush();
//    pinMode(8, INPUT);
//	if (digitalRead(8)) {
//		Serial.println("Locking");
//		serialFlush();
//		while (true);
//	}
//    cli();
//    CLKPR = bit(CLKPCE);
//#if defined(__AVR_ATtiny84__)
//    CLKPR = 0; // div 1, i.e. speed up to 8 MHz
//#else
//    CLKPR = 1; // div 2, i.e. slow down to 8 MHz
//#endif
//    sei();

#if defined(__AVR_ATtiny84__)
    // power up the radio on JMv3
    bitSet(DDRB, 0);
    bitClear(PORTB, 0);
#endif

    rf12_initialize(6, RF12_868MHZ, 5);
    // see http://tools.jeelabs.org/rfm12b
    //rf12_control(0xC040); // set low-battery level to 2.2V i.s.o. 3.1V
	  rf12_control(0x949C); // Receiver Control: LNA -20, RX @ 200Mhz, DRSSI-97
	  rf12_control(0x9850); // Transmission Control: Pos, 90kHz
	  rf12_control(0xC606); // Data Rate 6
    Serial.println("initialized");
    serialFlush();
    serialFlush();
    payload.msgtype = 0;
}

void loop() {
    ++payload.payload;
    Serial.print('t');
    serialFlush();
    
    rf12_sendNow(0, &payload, sizeof payload);
    // set the sync mode to 2 if the fuses are still the Arduino default
    // mode 3 (full powerdown) can only be used with 258 CK startup fuses
    Serial.print('w');
    serialFlush();
    rf12_sendWait(2);
    
    Serial.print('s');
    serialFlush();
    serialFlush();
    rf12_sleep(RF12_SLEEP);
    Sleepy::loseSomeTime(sleeptime);
    rf12_sleep(RF12_WAKEUP);
    Serial.println();
}

